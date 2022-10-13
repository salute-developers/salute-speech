#!/bin/bash

cp package/one-8kHz.pcm /usr/local/unimrcp/data/
cd /usr/local/unimrcp/bin && /src/package/recog.exp
