#!/usr/bin/env python3

import argparse
import time

import grpc

import storage_pb2
import storage_pb2_grpc
import task_pb2
import task_pb2_grpc


SLEEP_TIME = 5


def try_printing_request_id(md):
    for m in md:
        if m.key == 'x-request-id':
            print('RequestID:', m.value)


def task_function(args):
    ssl_cred = grpc.ssl_channel_credentials(
        root_certificates=open(args.ca, 'rb').read() if args.ca else None,
    )
    token_cred = grpc.access_token_call_credentials(args.token)

    channel = grpc.secure_channel(
        args.host,
        grpc.composite_channel_credentials(ssl_cred, token_cred),
    )

    task_stub = task_pb2_grpc.SmartSpeechStub(channel)
    storage_stub = storage_pb2_grpc.SmartSpeechStub(channel)

    try:
        if args.cancel:
            task, call = task_stub.CancelTask.with_call(task_pb2.CancelTaskRequest(task_id=args.task_id))
        else:
            while True:
                time.sleep(SLEEP_TIME)

                task, call = task_stub.GetTask.with_call(task_pb2.GetTaskRequest(task_id=args.task_id))
                if not args.wait:
                    break

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
                    with open(args.wait, 'wb') as f:
                        for chunk in download_response:
                            f.write(chunk.file_chunk)
                    print('Output file has been downloaded')
                    return
        try_printing_request_id(call.initial_metadata())
        print('Response:', task)
    except grpc.RpcError as err:
        print('RPC error: code = {}, details = {}'.format(err.code(), err.details()))
    except Exception as exc:
        print('Exception:', exc)
    finally:
        channel.close()


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument('--host', default='smartspeech.sber.ru', help='host:port of gRPC endpoint')
    parser.add_argument('--token', required=True, default=argparse.SUPPRESS, help='access token')
    parser.add_argument('--task-id', required=True, default=argparse.SUPPRESS, help='task_id')

    parser.add_argument('--ca', help='CA certificate file name (TLS)')

    action_parser = parser.add_mutually_exclusive_group()
    action_parser.add_argument('--cancel', action='store_true', help='cancel')
    action_parser.add_argument('--wait', help='wait for readiness and download result')

    args = parser.parse_args()

    task_function(args)


if __name__ == '__main__':
    main()
