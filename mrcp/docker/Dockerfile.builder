FROM ubuntu:21.04

ENV DEBIAN_FRONTEND=noninteractive

# install building tools
RUN apt-get update && apt-get install -y build-essential openjdk-11-jdk python3 python-is-python3 zip unzip wget autoconf libtool pkg-config

# download bazel and bootstrap bazel
RUN cd /tmp && wget https://github.com/bazelbuild/bazel/releases/download/4.2.2/bazel-4.2.2-dist.zip
RUN mkdir -p /opt/bazel/ && cd /opt/bazel
RUN unzip /tmp/bazel-4.2.2-dist.zip
RUN env EXTRA_BAZEL_ARGS="--host_javabase=@local_jdk//:jdk" bash ./compile.sh
RUN cp output/bazel /usr/local/bin/


