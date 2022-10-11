#!/usr/bin/env node

const fs = require('fs');
const grpc = require('@grpc/grpc-js');
const protoLoader = require('@grpc/proto-loader');

const HOST = 'smartspeech.sber.ru';
const OPTIONS = {
    language: 'ru-RU',
    model: 'general',
    hypotheses_count: 1,
    enable_partial_results: false,
    enable_multi_utterance: false,
    enable_profanity_filter: false,
    no_speech_timeout: {seconds: 7},
    max_speech_timeout: {seconds: 20},
    hints: {
        words: [],
        enable_letters: false,
        hints_eou_timeout: {seconds: 0},
    },
};
const PROTO = __dirname + '/../recognition.proto';

function recognize(accessToken, file, sampleRate) {
    const packageDefinition = protoLoader.loadSync(PROTO, {
        keepCase: true, longs: String, enums: String, defaults: true, oneofs: true, includeDirs: [__dirname + '/../../../task/v1/']});
    const proto = grpc.loadPackageDefinition(packageDefinition);

    const service = new proto['smartspeech']['recognition']['v1']['SmartSpeech'](HOST, grpc.credentials.createSsl());

    let metadata = new grpc.Metadata();
    metadata.set('authorization', 'Bearer ' + accessToken);

    const call = service['Recognize'](metadata);
    call.on('metadata', (metadata) => {
        console.info('X-Request-Id:', metadata.get('x-request-id'));
    });
    call.on('data', (response) => {
        console.info('Response:')
        if (response.results && response.results.length > 0) {
            console.info(`  Raw text: ${response.results[0].text}`);
            console.info(`  Normalized text: ${response.results[0].normalized_text}`);
            console.info(`  Is final: ${response.eou}`);
        }
    });
    call.on('error', (error) => {
        console.error(`Error: ${error.message}`);
        process.exit(1);
    });
    call.on('end', () => {
        call.end(function () {
            service.close();
            process.exit(0);
        });
    });

    const deadline = new Date(Date.now() + 3000);
    service.waitForReady(deadline, (error) => {
        if (!error) {
            const options = Object.assign({
                audio_encoding: (/\.ogg|.opus$/.test(file)) ? 'OPUS' : 'PCM_S16LE',
                sample_rate: sampleRate,
            }, OPTIONS);
            call.write({options: options});

            const reader = fs.createReadStream(file, {highWaterMark: 3200});
            reader.on('data', (chunk) => {
                call.write({audio_chunk: chunk})
            });
            reader.on('error', (error) => {
                console.error('Failed to read file:', error.message);
                process.exit(1);
            });
            reader.on('end', () => {
                call.end()
            });
        } else {
            console.error(`Failed to connect to ${HOST}:`, error);
            process.exit(1);
        }
    });
}

function main() {
    try {
        fs.lstatSync(PROTO).isFile();
    } catch (error) {
        console.error(`Failed to load proto file ${PROTO}:`, error);
        process.exit(1);
    }

    const argv = process.argv.slice(1);
    if (argv.length < 3 || argv.length > 4) {
        console.error(`usage: node ${argv[0]} <access_token> <file> [<sample_rate>]`);
        process.exit(1);
    }
    const accessToken = argv[1];
    const file = argv[2];
    const sampleRate = argv[3] || 16000;
    recognize(accessToken, file, sampleRate)
}

main()
