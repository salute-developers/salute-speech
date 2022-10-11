#!/usr/bin/env python3

import argparse

import grpc

import synthesis_pb2
import synthesis_pb2_grpc

ENCODING_PCM = 'pcm'
ENCODING_OPUS = 'opus'
ENCODING_WAV = 'wav'
ENCODINGS_MAP = {
    ENCODING_PCM: synthesis_pb2.SynthesisRequest.PCM_S16LE,
    ENCODING_OPUS: synthesis_pb2.SynthesisRequest.OPUS,
    ENCODING_WAV: synthesis_pb2.SynthesisRequest.WAV,
}

TYPE_TEXT = 'text'
TYPE_SSML = 'ssml'
TYPES_MAP = {
    TYPE_TEXT: synthesis_pb2.SynthesisRequest.TEXT,
    TYPE_SSML: synthesis_pb2.SynthesisRequest.SSML,
}


def try_printing_request_id(md):
    for m in md:
        if m.key == 'x-request-id':
            print('RequestID:', m.value)


def synthesize(args):
    ssl_cred = grpc.ssl_channel_credentials(
        root_certificates=open(args.ca, 'rb').read() if args.ca else None,
    )
    token_cred = grpc.access_token_call_credentials(args.token)

    channel = grpc.secure_channel(
        args.host,
        grpc.composite_channel_credentials(ssl_cred, token_cred),
    )

    stub = synthesis_pb2_grpc.SmartSpeechStub(channel)

    con = stub.Synthesize(args.synthesis_options)

    try:
        with open(args.file, 'wb') as f:
            for resp in con:
                f.write(resp.data)
                print('Got {} of audio'.format(resp.audio_duration.ToJsonString()))
    except grpc.RpcError as err:
        print('RPC error: code = {}, details = {}'.format(err.code(), err.details()))
    except Exception as exc:
        print('Exception:', exc)
    else:
        print('Synthesis has finished')
    finally:
        try_printing_request_id(con.initial_metadata())
        channel.close()


class Arguments:
    NOT_SYNTHESIS_OPTIONS = {'host', 'token', 'file'}

    def __init__(self):
        super().__setattr__('synthesis_options', synthesis_pb2.SynthesisRequest())

    def __setattr__(self, key, value):
        if key in self.NOT_SYNTHESIS_OPTIONS:
            super().__setattr__(key, value)
        else:
            setattr(self.synthesis_options, key, value)


def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('--host', default='smartspeech.sber.ru', help='host:port of gRPC endpoint')
    parser.add_argument('--token', required=True, default=argparse.SUPPRESS, help='access token')
    parser.add_argument('--file', required=True, default=argparse.SUPPRESS, help='audio file for recognition')

    parser.add_argument('--ca', help='CA certificate file name (TLS)')

    parser.add_argument('--text', required=True, default='')
    parser.add_argument('--audio-encoding', default=ENCODINGS_MAP[ENCODING_WAV], type=lambda x: ENCODINGS_MAP[x], help=','.join(ENCODINGS_MAP))
    parser.add_argument('--content-type', default=TYPES_MAP[TYPE_TEXT], type=lambda x: TYPES_MAP[x], help=','.join(TYPES_MAP))
    parser.add_argument('--language', default='ru-RU', help=' ')
    parser.add_argument('--voice', default='May_24000', help=' ')

    args = parser.parse_args(namespace=Arguments())

    synthesize(args)


if __name__ == '__main__':
    main()
