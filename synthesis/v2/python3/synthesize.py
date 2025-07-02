#!/usr/bin/env python3

import argparse
import itertools

import grpc

import synthesisv2_pb2
import synthesisv2_pb2_grpc

ENCODING_PCM = 'pcm'
ENCODING_OPUS = 'opus'
ENCODING_WAV = 'wav'
ENCODING_ALAW = 'alaw'
ENCODINGS_MAP = {
    ENCODING_PCM: synthesisv2_pb2.Options.PCM_S16LE,
    ENCODING_OPUS: synthesisv2_pb2.Options.OPUS,
    ENCODING_WAV: synthesisv2_pb2.Options.WAV,
    ENCODING_ALAW: synthesisv2_pb2.Options.PCM_ALAW,
}

TYPE_TEXT = 'text'
TYPE_SSML = 'ssml'
TYPES_MAP = {
    TYPE_TEXT: synthesisv2_pb2.Text.TEXT,
    TYPE_SSML: synthesisv2_pb2.Text.SSML,
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
        grpc.composite_channel_credentials(ssl_cred, token_cred)
    )

    stub = synthesisv2_pb2_grpc.SmartSpeechStub(channel)

    con = stub.Synthesize(itertools.chain(
        (synthesisv2_pb2.SynthesisRequest(options=args.synthesis_options),
         synthesisv2_pb2.SynthesisRequest(text=args.synthesis_text),),
    ))

    try:
        with open(args.file, 'wb') as f:
            for resp in con:
                if resp.WhichOneof('response') == 'audio':
                    f.write(resp.audio.audio_chunk)
                    print('Got {} of audio'.format(resp.audio.audio_duration.ToJsonString()))
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
    NOT_SYNTHESIS_OPTIONS = {'host', 'token', 'file', 'ca'}
    SYNTHESIS_REQUEST_OPTIONS = {'audio_encoding', 'language', 'voice'}

    def __init__(self):
        super().__setattr__('synthesis_options', synthesisv2_pb2.Options())
        super().__setattr__('synthesis_text', synthesisv2_pb2.Text())

    def __setattr__(self, key, value):
        if key in self.NOT_SYNTHESIS_OPTIONS:
            super().__setattr__(key, value)
        elif key in self.SYNTHESIS_REQUEST_OPTIONS:
            setattr(self.synthesis_options, key, value)
        else:
            setattr(self.synthesis_text, key, value)


def create_parser(encodings=None):
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('--host', default='smartspeech.sber.ru', help='host:port of gRPC endpoint')
    parser.add_argument('--token', required=True, default=argparse.SUPPRESS, help='access token')
    parser.add_argument('--file', required=True, default=argparse.SUPPRESS, help='output audio file')

    parser.add_argument('--ca', help='CA certificate file name (TLS)')

    parser.add_argument('--text', required=True, default='', help='input text')

    default_encoding = ENCODING_WAV if ENCODING_WAV in encodings else encodings[0]
    parser.add_argument('--audio-encoding', default=ENCODINGS_MAP[default_encoding], type=lambda x: ENCODINGS_MAP[x], help=','.join(encodings or ENCODINGS_MAP))
    parser.add_argument('--content-type', default=TYPES_MAP[TYPE_TEXT], type=lambda x: TYPES_MAP[x], help=','.join(TYPES_MAP))
    parser.add_argument('--language', default='ru-RU', help=' ')
    parser.add_argument('--voice', default='May_24000', help=' ')

    return parser


def main():
    parser = create_parser([ENCODING_WAV, ENCODING_OPUS, ENCODING_PCM])

    synthesize(parser.parse_args(namespace=Arguments()))


if __name__ == '__main__':
    main()
