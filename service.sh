#!/system/bin/sh

# Magic Cam Service Script
# This runs as root during boot

# Create Magic Cam directory for virtual video sources
mkdir -p /sdcard/MagicCam
chmod 755 /sdcard/MagicCam

# Set SELinux context if needed
if [ -x /system/bin/chcon ]; then
    chcon -R u:object_r:media_rw_data_file:s0 /sdcard/MagicCam 2>/dev/null
fi

# Log module activation
echo "$(date): Magic Cam module service started" >> /data/local/tmp/magiccam.log