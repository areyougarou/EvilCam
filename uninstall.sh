#!/system/bin/sh

# Magic Cam Uninstall Script

ui_print "- Uninstalling Magic Cam module"

# Remove MagicCam directory (optional - user data)
if [ -d "/sdcard/MagicCam" ]; then
    ui_print "- Removing MagicCam directory"
    rm -rf /sdcard/MagicCam
fi

# Remove log files
rm -f /data/local/tmp/magiccam.log

ui_print "- Magic Cam module uninstalled successfully"
ui_print "- Reboot to complete removal"