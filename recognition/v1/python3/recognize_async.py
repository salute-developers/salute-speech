#!/usr/bin/env python3
import argparse

import grpc
import time

import recognition_pb2
import recognition_pb2_grpc
import storage_pb2
import storage_pb2_grpc
import task_pb2
import task_pb2_grpc
from recognize import Arguments, CHUNK_SIZE, create_parser


SLEEP_TIME = 5


def generate_chunks(path, chunk_size=CHUNK_SIZE):
    with open(path, 'rb') as f:
        for data in iter(lambda: f.read(chunk_size), b''):
            yield storage_pb2.UploadRequest(file_chunk=data)


def recognize_async(args):
    ssl_cred = grpc.ssl_channel_credentials(
        root_certificates=open(args.ca, 'rb').read() if args.ca else None,
    )
    token_cred = grpc.access_token_call_credentials(args.token)

    channel = grpc.secure_channel(
        args.host,
        grpc.composite_channel_credentials(ssl_cred, token_cred),
    )

    recognition_stub = recognition_pb2_grpc.SmartSpeechStub(channel)
    storage_stub = storage_pb2_grpc.SmartSpeechStub(channel)
    task_stub = task_pb2_grpc.SmartSpeechStub(channel)

    try:
        upload_response = storage_stub.Upload(generate_chunks(args.file))
        print('Input file has been uploaded:', upload_response.request_file_id)

        recognition_task = recognition_stub.AsyncRecognize(
            recognition_pb2.AsyncRecognizeRequest(options=args.recognition_options, request_file_id=upload_response.request_file_id))
        print('Task has been created:', recognition_task.id)

        while True:
            time.sleep(SLEEP_TIME)

            task = task_stub.GetTask(task_pb2.GetTaskRequest(task_id=recognition_task.id))
            if task.status == task_pb2.Task.NEW:
                print('-', end='', flush=True)
                continue
            elif task.status == task_pb2.Task.RUNNING:
                print('+', end='', flush=True)
                continue
            elif task.status == task_pb2.Task.CANCELED:
                print('\nTask has been canceled')
                break
            elif task.status == task_pb2.Task.ERROR:
                print('\nTask has failed:', task.error)
                break
            elif task.status == task_pb2.Task.DONE:
                print('\nTask has finished successfully:', task.response_file_id)
                download_response = storage_stub.Download(storage_pb2.DownloadRequest(response_file_id=task.response_file_id))
                with open(args.output, 'wb') as f:
                    for chunk in download_response:
                        f.write(chunk.file_chunk)
                print('Output file has been downloaded')
                break
    except grpc.RpcError as err:
        print('RPC error: code = {}, details = {}'.format(err.code(), err.details()))
    except Exception as exc:
        print('Exception:', exc)
    finally:
        channel.close()


def main():
    parser = create_parser()

    parser.add_argument('--output', required=True, default=argparse.SUPPRESS, help='file for results')
    Arguments.NOT_RECOGNITION_OPTIONS.update({'output'})

    recognize_async(parser.parse_args(namespace=Arguments()))


if __name__ == '__main__':
    main()
