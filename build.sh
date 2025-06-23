#!/bin/bash

# Magic Cam Build Script

set -e

echo "Building Magic Cam Zygisk Module..."

# Check if NDK is available
if [ -z "$ANDROID_NDK_ROOT" ] && [ -z "$NDK_ROOT" ]; then
    echo "Error: Android NDK not found. Please set ANDROID_NDK_ROOT or NDK_ROOT"
    exit 1
fi

# Set NDK path
if [ -n "$ANDROID_NDK_ROOT" ]; then
    NDK_PATH="$ANDROID_NDK_ROOT"
else
    NDK_PATH="$NDK_ROOT"
fi

echo "Using NDK: $NDK_PATH"

# Create build directories
mkdir -p build/arm64-v8a
mkdir -p build/armeabi-v7a
mkdir -p lib/arm64-v8a
mkdir -p lib/armeabi-v7a

# Build for arm64-v8a
echo "Building for arm64-v8a..."
cd build/arm64-v8a
cmake -DCMAKE_TOOLCHAIN_FILE="$NDK_PATH/build/cmake/android.toolchain.cmake" \
      -DANDROID_ABI=arm64-v8a \
      -DANDROID_PLATFORM=android-29 \
      -DCMAKE_BUILD_TYPE=Release \
      ../..
make -j$(nproc)
cp libmagiccam.so ../../lib/arm64-v8a/
cd ../..

# Build for armeabi-v7a
echo "Building for armeabi-v7a..."
cd build/armeabi-v7a
cmake -DCMAKE_TOOLCHAIN_FILE="$NDK_PATH/build/cmake/android.toolchain.cmake" \
      -DANDROID_ABI=armeabi-v7a \
      -DANDROID_PLATFORM=android-29 \
      -DCMAKE_BUILD_TYPE=Release \
      ../..
make -j$(nproc)
cp libmagiccam.so ../../lib/armeabi-v7a/
cd ../..

# Create module package
echo "Creating module package..."
rm -f MagicCam-v1.0.0.zip

zip -r MagicCam-v1.0.0.zip \
    module.prop \
    META-INF/ \
    customize.sh \
    service.sh \
    lib/ \
    -x "*.git*" "build/" "jni/" "*.md" "*.sh" "CMakeLists.txt"

echo "Build completed successfully!"
echo "Module package: MagicCam-v1.0.0.zip"
echo ""
echo "Installation instructions:"
echo "1. Install the ZIP file through Magisk Manager"
echo "2. Ensure Zygisk is enabled in Magisk settings"
echo "3. Reboot your device"
echo "4. The module will automatically hook camera APIs in supported apps"