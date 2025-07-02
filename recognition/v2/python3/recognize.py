#!/usr/bin/env python3

import argparse
import itertools
import time

import grpc

import recognitionv2_pb2
import recognitionv2_pb2_grpc


CHUNK_SIZE = 2048
SLEEP_TIME = 0.1

ENCODING_PCM = 'pcm'

ENCODINGS_MAP = {
    ENCODING_PCM: recognitionv2_pb2.RecognitionOptions.PCM_S16LE,
    'opus': recognitionv2_pb2.RecognitionOptions.OPUS,
    'mp3': recognitionv2_pb2.RecognitionOptions.MP3,
    'flac': recognitionv2_pb2.RecognitionOptions.FLAC,
    'alaw': recognitionv2_pb2.RecognitionOptions.ALAW,
    'mulaw': recognitionv2_pb2.RecognitionOptions.MULAW,
}


def try_printing_request_id(md):
    for m in md:
        if m.key == 'x-request-id':
            print('RequestID:', m.value)


def generate_audio_chunks(path, chunk_size=CHUNK_SIZE, sleep_time=SLEEP_TIME):
    with open(path, 'rb') as f:
        for data in iter(lambda: f.read(chunk_size), b''):
            yield recognitionv2_pb2.RecognitionRequest(audio_chunk=data)
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

    stub = recognitionv2_pb2_grpc.SmartSpeechStub(channel)

    metadata_pairs = [(args.metadata[i], args.metadata[i+1]) for i in range(0, len(args.metadata), 2)]

    con = stub.Recognize(itertools.chain(
        (recognitionv2_pb2.RecognitionRequest(options=args.recognition_options),),
        generate_audio_chunks(args.file),
    ), metadata=metadata_pairs)

    try:
        for resp in con:
            if resp.WhichOneof('response') == 'transcription':
                transcription = resp.transcription
                if not transcription.eou:
                    print('Got partial result:')
                else:
                    print('Got end-of-utterance result:')

                for i, hyp in enumerate(transcription.results):
                    print('  Channel #{}, Hyp #{}: {}'.format(transcription.channel, i + 1, hyp.normalized_text if args.normalized_result else hyp.text))
                if args.recognition_options.speaker_separation_options.enable:
                    print('  Speaker info: id {}, confidence {}'.format(transcription.speaker_info.speaker_id, transcription.speaker_info.main_speaker_confidence))

                if transcription.eou and args.emotions_result:
                    print('  Emotions: pos={}, pos_a={}, pos_t={}, neu={}, neu_a={}, neu_t={}, neg={}, neg_a={}, neg_t={}'.format(
                        transcription.emotions_result.positive,
                        transcription.emotions_result.positive_a,
                        transcription.emotions_result.positive_t,
                        transcription.emotions_result.neutral,
                        transcription.emotions_result.neutral_a,
                        transcription.emotions_result.neutral_t,
                        transcription.emotions_result.negative,
                        transcription.emotions_result.negative_a,
                        transcription.emotions_result.negative_t
                    ))
            elif resp.WhichOneof('response') == 'insight':
                print('Insight:')
                print(resp.insight.insight_result)
            elif resp.WhichOneof('response') == 'vad':
                print('VAD: channel {}, processed_audio_time {}, utterance_detection_time {}'.format(
                    resp.vad.channel,
                    resp.vad.processed_audio_time,
                    resp.vad.utterance_detection_time
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
    OPTIONAL_BOOLS_PREFIX = 'enable_'
    NORMALIZATION_OPTIONS_PREFIX = 'normalization_options_'
    CUSTOM_WS_FLOW_CONTROL = 'custom_ws_flow_control'

    def __init__(self):
        super().__setattr__('recognition_options', recognitionv2_pb2.RecognitionOptions())

    def __setattr__(self, key, value):
        if key in self.NOT_RECOGNITION_OPTIONS:
            super().__setattr__(key, value)
        elif key.startswith(self.HINTS_PREFIX):
            key = key[len(self.HINTS_PREFIX):]
            self._set_option(self.recognition_options.hints, key, value)
        elif key.startswith(self.SPEAKER_SEPARATION_PREFIX):
            key = key[len(self.SPEAKER_SEPARATION_PREFIX):]
            self._set_option(self.recognition_options.speaker_separation_options, key, value)
        elif key.startswith(self.OPTIONAL_BOOLS_PREFIX) or key == self.CUSTOM_WS_FLOW_CONTROL:
            self._set_option(getattr(self.recognition_options, key), "enable", value)
        elif key.startswith(self.NORMALIZATION_OPTIONS_PREFIX):
            key = key[len(self.NORMALIZATION_OPTIONS_PREFIX):]
            self._set_option(getattr(self.recognition_options.normalization_options, key), "enable", value)
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
    parser.add_argument('--language', default='ru-RU', help=' ')
    parser.add_argument('--model', default='', help=' ')
    parser.add_argument('--hypotheses-count', default=1, type=int, help='min=0, max=10')
    parser.add_argument('--enable-multi-utterance', action='store_true', help=' ')
    parser.add_argument('--enable-partial-results', action='store_true', help=' ')
    parser.add_argument('--no-speech-timeout', default='7s', help='min=2s, max=20s')
    parser.add_argument('--max-speech-timeout', default='20s', help='min=500ms, max=20s')
    parser.add_argument('--hints-words', nargs='*', default=[], help=' ')
    parser.add_argument('--hints-enable-letters', action='store_true', help=' ')
    parser.add_argument('--hints-eou-timeout', default='0s', help='min=500ms, max=5s')
    parser.add_argument('--channels-count', default=1, type=int, help='max: opus-1, mp3-2, flac-8, other-8')
    parser.add_argument('--insight-models', nargs='*', default=[], help=' ')
    parser.add_argument('--speaker-separation-options-enable', action='store_true', help=' ')
    parser.add_argument('--speaker-separation-options-enable-only-main-speaker', action='store_true', help=' ')
    parser.add_argument('--speaker-separation-options-count', default=0, type=int, help='min=1, max=10 (if speaker separation enabled)')
    parser.add_argument('--normalization-options-enable', action='store_true', help=' ')
    parser.add_argument('--normalization-options-profanity-filter', action='store_true', help=' ')
    parser.add_argument('--normalization-options-punctuation', action='store_true', help=' ')
    parser.add_argument('--normalization-options-capitalization', action='store_true', help=' ')
    parser.add_argument('--normalization-options-question', action='store_true', help=' ')
    parser.add_argument('--normalization-options-force-cyrillic', action='store_true', help=' ')
    parser.add_argument('--enable-vad', action='store_true', help=' ')
    parser.add_argument('--custom-ws-flow-control', action='store_true', help=' ')
    parser.add_argument('--enable-long-utterances', action='store_true', help=' ')

    return parser


def main():
    parser = create_parser()

    recognize(parser.parse_args(namespace=Arguments()))


if __name__ == '__main__':
    main()
