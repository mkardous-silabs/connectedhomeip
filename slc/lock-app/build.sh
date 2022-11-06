#!/bin/bash
MATTER_ROOT=$( cd "../.." ; pwd -P )
GSDK_ROOT=$MATTER_ROOT/third_party/silabs/gecko_sdk

# Ensure Matter repo is registered as SDK extension
EXTENSION_DIR=$GSDK_ROOT/extension/matter
if [ ! -L "$EXTENSION_DIR" ] ; then
    ln -s ../../../../ $EXTENSION_DIR
fi

# Trust SDK and Matter extension
echo "Ensure SDK and Matter extension are trusted by SLC."
slc signature trust --sdk $GSDK_ROOT
slc signature trust --sdk $GSDK_ROOT --extension-path "$GSDK_ROOT/extension/matter/"

# Patch SDK
echo "Patching SDK."
if git -C $GSDK_ROOT apply --reverse --check $MATTER_ROOT/slc/script/gsdk_matter.patch >/dev/null 2>&1; then
    echo "SDK is already patched."
else
    git -C $GSDK_ROOT apply $MATTER_ROOT/slc/script/gsdk_matter.patch && echo "Patch successful."
fi

# Make ZAP available to SLC-CLI
export STUDIO_ADAPTER_PACK_PATH="$MATTER_ROOT/third_party/zap/repo/"

# Ensure adapter pack integration for ZAP references Matter templates, and outputs into the zap-generated subdirectory as required by the Matter source code
sed -i'.orig' \
    -e 's#app/zcl/zcl-zap.json#extension/matter/src/app/zap-templates/zcl/zcl.json#g' \
    -e 's#protocol/zigbee/app/framework/gen-template/gen-templates.json#extension/matter/src/app/zap-templates/app-templates.json#g' \
    -e 's#generationOutput} #generationOutput}/zap-generated #g' \
    "$STUDIO_ADAPTER_PACK_PATH/apack.json"

# Generate project
slc generate -d $1 -p lock-app.slcp -s $GSDK_ROOT --with $1

# Change to C++17 with GCC extensions
# Matter needs GCC extensions -- strnlen() and strtok_r() are POSIX library symbols otherwise not available
# Matter needs C++14 or higher
# Pigweed needs C++17 or higher

# Don't use MacOS-specific syntax
#sed -i'.orig' \
#    -e s"#-std=c\+\+11#-std=gnu\+\+17#g" \
#    "$1/lock-app.project.mak"


sed -i'.orig' \
    's/-std=c++11/-std=gnu++17/g' \
    $1/lock-app.project.mak

make -C $1 -f lock-app.Makefile -j4 
