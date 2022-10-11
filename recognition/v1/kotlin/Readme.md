## Building

    $ mvn clean package

## Usage

    $ java -jar ./target/smartSpeechRecognizeApp-1.0.jar --token <jwt token> --in <path to pcm16 or opus file> --format <string with audio format "pcm" or "opus"> --sr <sample rate>
