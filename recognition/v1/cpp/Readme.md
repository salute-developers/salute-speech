# OSX
### Building
    $ brew install protobuf grpc openssl 
    
    $ mkdir build && cd build
    $ cmake ..
    $ make

### Usage
    $ GRPC_DEFAULT_SSL_ROOTS_FILE_PATH=/etc/ssl/cert.pem recognize --token "<access_token>" --file "<file>"