#!/bin/bash

# This script copies Matter apps from the GN built Matter structure to the SLC built Matter structure
# Example usage:
# bash copy-app.sh lighting-app
MATTER_ROOT="$(dirname "$0")/.."

if [ "$#" -ne 1 ]; then
    echo "Illegal number of parameters"
    echo "Usage: $0 <matter app>"
    exit 1
fi

MATTER_APP=$1

if [ "$MATTER_APP" != "lighting-app" ] && [ "$MATTER_APP" != "light-switch-app" ] && [ "$MATTER_APP" != "thermostat" ] && [ "$MATTER_APP" != "window-app" ] && [ "$MATTER_APP" != "lock-app" ]; then
    echo "Invalid App choice"
    exit 1
fi

[ -d sample-app/$MATTER_APP/ ] && echo "Directory sample-app/$MATTER_APP/ already exists." || mkdir sample-app/$MATTER_APP/
[ -d sample-app/$MATTER_APP/common ] && echo "Directory sample-app/$MATTER_APP/common already exists." || mkdir sample-app/$MATTER_APP/common/

# Copy all files from GN application in src and include to SLC expect main.cpp
rsync -aP --exclude=main.cpp $MATTER_ROOT/examples/$MATTER_APP/efr32/src/* sample-app/$MATTER_APP/src/
rsync -aP $MATTER_ROOT/examples/$MATTER_APP/efr32/include/* sample-app/$MATTER_APP/include/

# Window app needs its own main, and doesn't need app.h
# Window app has "common" folder instead of "window-common" like the other apps.. 
if [ "$MATTER_APP" != "window-app" ]; then
    cp $MATTER_ROOT/examples/$MATTER_APP/${MATTER_APP%-*}-common/$MATTER_APP.zap sample-app/$MATTER_APP/common/$MATTER_APP.zap
    cp $MATTER_ROOT/examples/$MATTER_APP/${MATTER_APP%-*}-common/$MATTER_APP.matter sample-app/$MATTER_APP/common/$MATTER_APP.matter

    cp sample-app/common/app.h sample-app/$MATTER_APP/include/app.h
    cp sample-app/common/main.cpp sample-app/$MATTER_APP/src/main.cpp
else
    cp $MATTER_ROOT/examples/$MATTER_APP/common/$MATTER_APP.zap sample-app/$MATTER_APP/common/$MATTER_APP.zap
    cp $MATTER_ROOT/examples/$MATTER_APP/common/$MATTER_APP.matter sample-app/$MATTER_APP/common/$MATTER_APP.matter

    rsync -aP $MATTER_ROOT/examples/$MATTER_APP/common/include/ sample-app/$MATTER_APP/include/
    rsync -aP $MATTER_ROOT/examples/$MATTER_APP/common/src/ sample-app/$MATTER_APP/src/

    cp $MATTER_ROOT/examples/$MATTER_APP/efr32/src/main.cpp sample-app/$MATTER_APP/src/main.cpp
fi
