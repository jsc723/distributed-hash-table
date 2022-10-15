#!/bin/bash -e
DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REL=$DIR/release

PAC_NAME=dh-release

mkdir -p $REL

cp -r $DIR/bin $REL
cp -r $DIR/script $REL
#make the relative lib paths override the system lib path
patchelf --set-rpath "\$ORIGIN/lib" $REL/bin/client
patchelf --set-rpath "\$ORIGIN/lib" $REL/bin/dhserver

#make a tar
tar -cvzf $PAC_NAME.tar.gz ./release
