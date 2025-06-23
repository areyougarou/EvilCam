#!/system/bin/sh

# Magic Cam Module Installation Script

ui_print "- Installing Magic Cam Virtual Camera Module"
ui_print "- Version: 1.0.0"
ui_print "- Author: Magic Cam Team"

# Check if Zygisk is enabled
if [ ! -f /data/adb/magisk/zygisk ]; then
    ui_print "! Zygisk is not enabled!"
    ui_print "! Please enable Zygisk in Magisk settings and reboot"
    ui_print "! Then reinstall this module"
    exit 1
fi

ui_print "- Zygisk detected: OK"

# Check Android version
API=$(getprop ro.build.version.sdk)
if [ "$API" -lt 29 ]; then
    ui_print "! Android 10+ (API 29+) required"
    ui_print "! Current API: $API"
    exit 1
fi

ui_print "- Android API $API: OK"

# Check architecture
ABI=$(getprop ro.product.cpu.abi)
case $ABI in
    arm64-v8a|armeabi-v7a)
        ui_print "- Architecture $ABI: Supported"
        ;;
    *)
        ui_print "! Unsupported architecture: $ABI"
        ui_print "! Only arm64-v8a and armeabi-v7a are supported"
        exit 1
        ;;
esac

# Create module directories
mkdir -p $MODPATH/zygisk
mkdir -p $MODPATH/webui

# Copy native libraries based on architecture
if [ "$ABI" = "arm64-v8a" ]; then
    cp $MODPATH/lib/arm64-v8a/libmagiccam.so $MODPATH/zygisk/libmagiccam.so
elif [ "$ABI" = "armeabi-v7a" ]; then
    cp $MODPATH/lib/armeabi-v7a/libmagiccam.so $MODPATH/zygisk/libmagiccam.so
fi

# Set permissions
set_perm_recursive $MODPATH 0 0 0755 0644
set_perm $MODPATH/zygisk/libmagiccam.so 0 0 0755
set_perm_recursive $MODPATH/webui 0 0 0755 0644

# Create default configuration
cat > $MODPATH/config.json << EOF
{
    "moduleEnabled": true,
    "virtualMode": true,
    "mediaType": "video",
    "videoPath": "",
    "photoPath": "",
    "videoQuality": "1080p",
    "photoResolution": "1080p",
    "loopVideo": true,
    "detectionBypass": true,
    "debugLogging": false,
    "hookMethod": "camera2ndk"
}
EOF

# Create MagicCam directory for videos
mkdir -p /sdcard/MagicCam
set_perm_recursive /sdcard/MagicCam 0 0 0755 0644

ui_print "- Magic Cam module installed successfully"
ui_print "- WebUI available in KernelSU/Magisk Manager"
ui_print "- Reboot to activate the module"
ui_print "- Place video/photo files in /sdcard/MagicCam/"
ui_print "- No additional configuration required"