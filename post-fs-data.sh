#!/system/bin/sh

# Magic Cam Post-FS-Data Script
# Runs early in the boot process

# Create MagicCam directory for virtual media files
mkdir -p /sdcard/MagicCam
chmod 755 /sdcard/MagicCam

# Set SELinux context if needed
if [ -x /system/bin/chcon ]; then
    chcon -R u:object_r:media_rw_data_file:s0 /sdcard/MagicCam 2>/dev/null
fi

# Log script execution
echo "$(date): Magic Cam post-fs-data script executed" >> /data/local/tmp/magiccam.log