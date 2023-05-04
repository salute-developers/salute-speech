#!/usr/bin/env python3

import argparse
import itertools
import time

import grpc

import recognition_pb2
import recognition_pb2_grpc


CHUNK_SIZE = 2048
SLEEP_TIME = 0.1

ENCODING_PCM = 'pcm'

ENCODINGS_MAP = {
    ENCODING_PCM: recognition_pb2.RecognitionOptions.PCM_S16LE,
    'opus': recognition_pb2.RecognitionOptions.OPUS,
    'mp3': recognition_pb2.RecognitionOptions.MP3,
    'flac': recognition_pb2.RecognitionOptions.FLAC,
    'alaw': recognition_pb2.RecognitionOptions.ALAW,
    'mulaw': recognition_pb2.RecognitionOptions.MULAW,
}


def try_printing_request_id(md):
    for m in md:
        if m.key == 'x-request-id':
            print('RequestID:', m.value)


def generate_audio_chunks(path, chunk_size=CHUNK_SIZE, sleep_time=SLEEP_TIME):
    with open(path, 'rb') as f:
        for data in iter(lambda: f.read(chunk_size), b''):
            yield recognition_pb2.RecognitionRequest(audio_chunk=data)
            time.sleep(sleep_time)


def recognize(args):
    ssl_cred = grpc.ssl_channel_credentials(
        root_certificates=open(args.ca, 'rb').read() if args.ca else None,
    )
    token_cred = grpc.access_token_call_credentials(args.token)

    channel = grpc.secure_channel(
        args.host,
        grpc.composite_channel_credentials(ssl_cred, token_cred)
    )

    stub = recognition_pb2_grpc.SmartSpeechStub(channel)

    metadata_pairs = [(args.metadata[i], args.metadata[i+1]) for i in range(0, len(args.metadata), 2)]

    con = stub.Recognize(itertools.chain(
        (recognition_pb2.RecognitionRequest(options=args.recognition_options),),
        generate_audio_chunks(args.file),
    ), metadata=metadata_pairs)

    try:
        for resp in con:
            if not resp.eou:
                print('Got partial result:')
            else:
                print('Got end-of-utterance result:')

            for i, hyp in enumerate(resp.results):
                print('  Hyp #{}: {}'.format(i + 1, hyp.normalized_text if args.normalized_result else hyp.text))

            if resp.eou and args.emotions_result:
                print('  Emotions: pos={}, neu={}, neg={}'.format(
                    resp.emotions_result.positive,
                    resp.emotions_result.neutral,
                    resp.emotions_result.negative,
                ))
    except grpc.RpcError as err:
        print('RPC error: code = {}, details = {}'.format(err.code(), err.details()))
    except Exception as exc:
        print('Exception:', exc)
    else:
        print('Recognition has finished')
    finally:
        try_printing_request_id(con.initial_metadata())
        channel.close()


class Arguments:
    NOT_RECOGNITION_OPTIONS = {'host', 'token', 'file', 'normalized_result', 'emotions_result', 'metadata', 'ca'}
    DURATIONS = {'no_speech_timeout', 'max_speech_timeout', 'eou_timeout'}
    REPEATED = {'words', 'insight_models'}
    HINTS_PREFIX = 'hints_'
    SPEAKER_SEPARATION_PREFIX = 'speaker_separation_options_'

    def __init__(self):
        super().__setattr__('recognition_options', recognition_pb2.RecognitionOptions())

    def __setattr__(self, key, value):
        if key in self.NOT_RECOGNITION_OPTIONS:
            super().__setattr__(key, value)
        elif key.startswith(self.HINTS_PREFIX):
            key = key[len(self.HINTS_PREFIX):]
            self._set_option(self.recognition_options.hints, key, value)
        elif key.startswith(self.SPEAKER_SEPARATION_PREFIX):
            key = key[len(self.SPEAKER_SEPARATION_PREFIX):]
            self._set_option(self.recognition_options.speaker_separation_options, key, value)
        else:
            self._set_option(self.recognition_options, key, value)

    def _set_option(self, obj, key, value):
        if key in self.DURATIONS:
            getattr(obj, key).FromJsonString(value)
        elif key in self.REPEATED:
            if value:
                getattr(obj, key).extend(value)
        else:
            setattr(obj, key, value)


def create_parser():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('--host', default='smartspeech.sber.ru', help='host:port of gRPC endpoint')
    parser.add_argument('--token', required=True, default=argparse.SUPPRESS, help='access token')
    parser.add_argument('--file', required=True, default=argparse.SUPPRESS, help='audio file for recognition')
    parser.add_argument('--metadata', nargs='*', default=[], help=' ')

    parser.add_argument('--ca', help='CA certificate file name (TLS)')

    parser.add_argument('--normalized-result', action='store_true', help='show normalized text')
    parser.add_argument('--emotions-result', action='store_true', help='show emotions result')

    parser.add_argument('--audio-encoding', default=ENCODINGS_MAP[ENCODING_PCM], type=lambda x: ENCODINGS_MAP[x], help=str(list(ENCODINGS_MAP)))
    parser.add_argument('--sample-rate', default=16000, type=int, help='PCM only')
    parser.add_argument('--model', default='', help=' ')
    parser.add_argument('--hypotheses-count', default=1, type=int, help=' ')
    parser.add_argument('--enable-profanity-filter', action='store_true', help=' ')
    parser.add_argument('--enable-multi-utterance', action='store_true', help=' ')
    parser.add_argument('--enable-partial-results', action='store_true', help=' ')
    parser.add_argument('--no-speech-timeout', default='7s', help=' ')
    parser.add_argument('--max-speech-timeout', default='20s', help=' ')
    parser.add_argument('--hints-words', nargs='*', default=[], help=' ')
    parser.add_argument('--hints-enable-letters', action='store_true', help=' ')
    parser.add_argument('--hints-eou-timeout', default='0s', help=' ')
    parser.add_argument('--channels-count', default=1, type=int, help=' ')
    parser.add_argument('--speaker-separation-options-enable', action='store_true', help=' ')
    parser.add_argument('--speaker-separation-options-enable-only-main-speaker', action='store_true', help=' ')
    parser.add_argument('--speaker-separation-options-count', default=0, type=int, help=' ')
    parser.add_argument('--force_cyrillic', action='store_true', help=' ')
    parser.add_argument('--insight-models', nargs='*', default=[], help=' ')

    return parser


def main():
    parser = create_parser()

    recognize(parser.parse_args(namespace=Arguments()))


if __name__ == '__main__':
    main()
