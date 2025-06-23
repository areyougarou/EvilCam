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

# Create complete module directory structure
MODULE_DIR="MagicCam-Module"
rm -rf "$MODULE_DIR"
mkdir -p "$MODULE_DIR"

echo "Creating complete module structure..."

# Copy all essential module files
cp module.prop "$MODULE_DIR/"
cp -r META-INF "$MODULE_DIR/"
cp customize.sh "$MODULE_DIR/"
cp service.sh "$MODULE_DIR/"
cp post-fs-data.sh "$MODULE_DIR/"
cp uninstall.sh "$MODULE_DIR/"
cp system.prop "$MODULE_DIR/"
cp -r lib "$MODULE_DIR/"

# Copy webroot directory for WebUI
if [ -d "webroot" ]; then
    cp -r webroot "$MODULE_DIR/"
    echo "WebUI files copied"
else
    echo "Warning: webroot directory not found"
fi

# Create additional module directories that will be populated during install
mkdir -p "$MODULE_DIR/zygisk"

# Set proper permissions
find "$MODULE_DIR" -type f -name "*.sh" -exec chmod +x {} \;
find "$MODULE_DIR" -type f -name "*.so" -exec chmod 755 {} \;

echo "Build completed successfully!"
echo "Module directory: $MODULE_DIR"
echo ""
echo "Installation instructions:"
echo "1. Copy the entire '$MODULE_DIR' folder to your device"
echo "2. Install through KernelSU/Magisk Manager"
echo "3. Ensure Zygisk is enabled"
echo "4. Reboot your device"
echo "5. Configure target apps via WebUI"
echo "6. Module will remain INACTIVE until apps are configured"