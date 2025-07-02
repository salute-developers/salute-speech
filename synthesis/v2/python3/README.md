# Installation

    $ pip3 install grpcio-tools
    $ python3 -m grpc_tools.protoc -I .. --python_out=. --grpc_python_out=. ../synthesisv2.proto

# Usage

    $ python3 synthesize.py [--ca /path/to/russian_trusted_root_ca] --token "<access_token>" --file "<output_file>" --text "<input_text>"
