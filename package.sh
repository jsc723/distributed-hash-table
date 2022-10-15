#!/bin/bash -e
DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REL=$DIR/release
mkdir -p $REL

cp -r $DIR/bin $REL
cp -r $DIR/script $REL
#make the relative lib paths override the system lib path
patchelf --set-rpath "\$ORIGIN/lib" $REL/bin/client
patchelf --set-rpath "\$ORIGIN/lib" $REL/bin/dhserver
