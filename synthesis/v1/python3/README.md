# Installation

    $ pip3 install grpcio-tools
    $ python3 -m grpc_tools.protoc -I .. -I ../../../task/v1 --python_out=. --grpc_python_out=. ../synthesis.proto ../../../task/v1/{storage,task}.proto

# Usage

    $ python3 synthesize.py [--ca /path/to/russian_trusted_root_ca] --token "<access_token>" --file "<output_file>" --text "<input_text>"

    $ python3 synthesize_async.py [--ca /path/to/russian_trusted_root_ca] --audio-encoding opus --token "<access_token>" --file "<output_file>" --text "<input_file>"
