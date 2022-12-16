#!/bin/bash

# This script copies the Matter extension files to the supplied directory path

MATTER_ROOT="$(dirname "$0")/.."

if [ "$#" -ne 1 ]; then
    echo "Illegal number of parameters"
    echo "Usage: $0 <destination directory>"
    exit 1
fi

DEST_DIR=$1

if [ ! -d "$DEST_DIR" ]; then
    echo "Directory $DEST_DIR does not exist"
    exit 1
fi

mkdir $DEST_DIR/matter_extension
mkdir $DEST_DIR/matter_extension/examples/
mkdir $DEST_DIR/matter_extension/examples/platform/
mkdir $DEST_DIR/matter_extension/examples/common/
mkdir $DEST_DIR/matter_extension/examples/common/QRCode/
mkdir $DEST_DIR/matter_extension/examples/common/QRCode/repo/
mkdir $DEST_DIR/matter_extension/third_party/
mkdir $DEST_DIR/matter_extension/third_party/silabs/
mkdir $DEST_DIR/matter_extension/third_party/silabs/wiseconnect-wifi-bt-sdk/
mkdir $DEST_DIR/matter_extension/examples/providers/
mkdir $DEST_DIR/matter_extension/zzz_generated

cp -R $MATTER_ROOT/src $DEST_DIR/matter_extension
cp -R $MATTER_ROOT/slc $DEST_DIR/matter_extension
cp -R $MATTER_ROOT/examples/platform/efr32 $DEST_DIR/matter_extension/examples/platform
cp -R $MATTER_ROOT/examples/providers $DEST_DIR/matter_extension/examples
cp -R $MATTER_ROOT/examples/common/QRCode/repo/c $DEST_DIR/matter_extension/examples/common/QRCode/repo
cp -R $MATTER_ROOT/third_party/lwip $DEST_DIR/matter_extension/third_party/lwip
cp -R $MATTER_ROOT/third_party/nlassert $DEST_DIR/matter_extension/third_party/nlassert
cp -R $MATTER_ROOT/third_party/pigweed $DEST_DIR/matter_extension/third_party/pigweed
cp -R $MATTER_ROOT/third_party/nlio $DEST_DIR/matter_extension/third_party/nlio
cp -R $MATTER_ROOT/zzz_generated/app-common $DEST_DIR/matter_extension/zzz_generated
cp -R $MATTER_ROOT/zzz_generated/controller-clusters $DEST_DIR/matter_extension/zzz_generated
cp -R $MATTER_ROOT/third_party/silabs/wiseconnect-wifi-bt-sdk/sapi $DEST_DIR/matter_extension/third_party/silabs/wiseconnect-wifi-bt-sdk/

cp $MATTER_ROOT/matter.slce $DEST_DIR/matter_extension
cp $MATTER_ROOT/matter.slsdk $DEST_DIR/matter_extension
cp $MATTER_ROOT/matter_templates.xml $DEST_DIR/matter_extension
cp $MATTER_ROOT/matter_docs.xml $DEST_DIR/matter_extension
cp $MATTER_ROOT/matter_demos.xml $DEST_DIR/matter_extension
cp $MATTER_ROOT/slc/EXTENSION_README.md $DEST_DIR/matter_extension/README.md

# Remove unnecessary directories
rm -fr $DEST_DIR/matter_extension/src/test_driver/
rm -fr $DEST_DIR/matter_extension/slc/sample-app/*/brd*/
rm -fr $DEST_DIR/matter_extension/slc/copy-app.sh
rm -fr $DEST_DIR/matter_extension/slc/copy-extension.sh
rm -fr $DEST_DIR/matter_extension/slc/build.sh
rm -fr $DEST_DIR/matter_extension/slc/README.md
rm -fr $DEST_DIR/matter_extension/slc/script/
rm -fr $DEST_DIR/matter_extension/slc/EXTENSION_README.md

echo "Matter extension code copied to $DEST_DIR"

exit 0
