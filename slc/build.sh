#!/bin/bash

# This script generates and builds the SLC project for the given Matter application and board.
# If the application name ends with the string "no-led" it is assumed that project directory name
# is the name of the application with the "no-led" suffix omitted. For example lighting-app-no-led.slcp
# is assumed to reside in ./sample-apps/lighting-app/
#
# Usage:
# ./slc/build.sh <slcp path> <board>
#
# Example usage:
# ./slc/build.sh slc/sample-app/lighting-app/lighting-app-no-led-thread.slcp brd4161a
#       output in out/lighting-app-no-led-thread/brd4161a

SILABS_APP_PATH=$1
SILABS_BOARD=$2
SILABS_APP=$(basename "$SILABS_APP_PATH" .slcp)
OUTPUT_DIR="out/$SILABS_APP/$SILABS_BOARD"

echo "Building $SILABS_APP for $SILABS_BOARD in $OUTPUT_DIR"

MATTER_ROOT=$( pwd -P )
GSDK_ROOT=$MATTER_ROOT/third_party/silabs/gecko_sdk

# Ensure Matter repo is registered as SDK extension

[ -d $GSDK_ROOT/extension ] && echo "Directory $GSDK_ROOT/extension exists." || mkdir $GSDK_ROOT/extension

EXTENSION_DIR=$GSDK_ROOT/extension/matter_extension
if [ ! -L "$EXTENSION_DIR" ]; then
    ln -s ../../../../ $EXTENSION_DIR
fi

# Trust SDK and Matter extension
echo "Ensure SDK and Matter extension are trusted by SLC."
slc signature trust --sdk $GSDK_ROOT
slc signature trust --sdk $GSDK_ROOT --extension-path "$GSDK_ROOT/extension/matter_extension/"

# Make ZAP available to SLC-CLI
export STUDIO_ADAPTER_PACK_PATH="$MATTER_ROOT/third_party/zap/repo/"


while [ $# -gt 0 ]; do
    case "$1" in
    --clean)
        rm -rf $OUTPUT_DIR
        shift
        ;;
    *)
        shift
        ;;
    esac
done
# Generate project
slc generate -d $OUTPUT_DIR -p $SILABS_APP_PATH -s $GSDK_ROOT --with $SILABS_BOARD

make -C $OUTPUT_DIR -f $SILABS_APP.Makefile -j4
