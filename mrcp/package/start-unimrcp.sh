#!/bin/bash

set -eux
set -o pipefail

cat /usr/local/unimrcp/conf/unimrcpserver.xml

# copy plugins
cp output/smartspeech-recognition-plugin.so /usr/local/unimrcp/plugin
cp output/smartspeech-synthesis-plugin.so /usr/local/unimrcp/plugin

mkdir -p /tmp/config && cd /tmp/config
cp /src/package/smartspeech-plugins-config.xml .

# prepare smartspeech config
sed -i "s/__CLIENT_ID__/$SMARTSPEECH_USER_ID/" smartspeech-plugins-config.xml
sed -i "s/__SECRET__/$SMARTSPEECH_SECRET/" smartspeech-plugins-config.xml
sed -i "s/__SCOPE__/$SMARTSPEECH_SCOPE/" smartspeech-plugins-config.xml

plugin_config=`cat smartspeech-plugins-config.xml`

# remove unimrcp demo plugins; add smartspeech plugins
xmlstarlet ed -L -d 'unimrcpserver/components/plugin-factory' -s 'unimrcpserver/components' -t elem -n plugin-factory -v "$plugin_config" /usr/local/unimrcp/conf/unimrcpserver.xml 
cat /usr/local/unimrcp/conf/unimrcpserver.xml | xmlstarlet unesc | xmlstarlet fo -R | tee /usr/local/unimrcp/conf/unimrcpserver.xml
cd /usr/local/unimrcp/bin && ./unimrcpserver -w -c ../conf/dirlayout.xml

