#!/system/bin/sh

# System Camera Service Installation Script

ui_print "- Installing System Camera Service Module"
ui_print "- Version: 1.0.0"
ui_print "- Author: System Team"

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
mkdir -p $MODPATH/webroot

# Copy native libraries based on architecture
if [ "$ABI" = "arm64-v8a" ]; then
    cp $MODPATH/lib/arm64-v8a/libmagiccam.so $MODPATH/zygisk/libmagiccam.so
elif [ "$ABI" = "armeabi-v7a" ]; then
    cp $MODPATH/lib/armeabi-v7a/libmagiccam.so $MODPATH/zygisk/libmagiccam.so
fi

# Set permissions
set_perm_recursive $MODPATH 0 0 0755 0644
set_perm $MODPATH/zygisk/libmagiccam.so 0 0 0755
set_perm_recursive $MODPATH/webroot 0 0 0755 0644

# Create default configuration with NO apps
cat > $MODPATH/config.json << EOF
{
    "moduleEnabled": true,
    "virtualMode": true,
    "mediaType": "video",
    "videoPath": "",
    "photoPath": "",
    "videoQuality": "1080p",
    "photoResolution": "1080p",
    "aspectRatio": "16:9",
    "photoEffects": "none",
    "loopVideo": true,
    "detectionBypass": true,
    "debugLogging": false,
    "hookMethod": "camera2ndk"
}
EOF

# Create EMPTY target apps file - user MUST configure via WebUI
cat > $MODPATH/target_apps.txt << EOF
# Target Applications for Camera Optimization
# Add package names one per line
# Lines starting with # are comments
# 
# Module will NOT hook any apps until configured via WebUI
# 
# Examples (remove # to enable):
# com.android.camera
# com.android.camera2
# com.instagram.android
# com.whatsapp
# us.zoom.videomeetings
EOF

# Create MagicCam directory for videos
mkdir -p /sdcard/MagicCam
set_perm_recursive /sdcard/MagicCam 0 0 0755 0644

ui_print "- System Camera Service installed successfully"
ui_print "- WebUI available in KernelSU/Magisk Manager"
ui_print "- Reboot to activate the module"
ui_print "- IMPORTANT: Configure target apps via WebUI"
ui_print "- Module is INACTIVE until apps are configured"
ui_print "- Place video/photo files in /sdcard/MagicCam/"