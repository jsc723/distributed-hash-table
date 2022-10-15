#!/bin/bash -e
#make the relative lib paths override the system lib path
patchelf --set-rpath "\$ORIGIN/lib" ../bin/client
patchelf --set-rpath "\$ORIGIN/lib" ../bin/dhserver
