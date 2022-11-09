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

mkdir  $DEST_DIR/matter
mkdir  $DEST_DIR/matter/examples/
mkdir  $DEST_DIR/matter/examples/platform/

cp -R $MATTER_ROOT/src $DEST_DIR/matter
cp -R $MATTER_ROOT/slc $DEST_DIR/matter
cp -R $MATTER_ROOT/examples/platform/efr32 $DEST_DIR/matter/examples/platform/
cp $MATTER_ROOT/matter.slce $DEST_DIR/matter
cp $MATTER_ROOT/matter.slsdk $DEST_DIR/matter
cp $MATTER_ROOT/matter_templates.xml $DEST_DIR/matter

# Remove unnecessary directories
rm -fr $DEST_DIR/matter/src/test_driver/

echo "Matter extension code copied to $DEST_DIR"

exit 0
