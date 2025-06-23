# Magic Cam - Zygisk Virtual Camera Module

A modern Android virtual camera module using the Zygisk framework for KernelSU/Magisk. This module provides system-level camera hooking with advanced detection bypass capabilities.

## Features

- **Zygisk Framework**: Uses modern Zygisk API instead of outdated Xposed
- **Multi-Architecture**: Supports both `armeabi-v7a` and `arm64-v8a`
- **Android 10-14**: Compatible with modern Android versions (API 29+)
- **System-Level Hooking**: Hooks Camera2 NDK and HAL APIs
- **Detection Bypass**: Advanced anti-detection and stealth mechanisms
- **Wide App Support**: Works with camera apps and social media platforms

## Supported Applications

### Camera Apps
- Google Camera, Samsung Camera, AOSP Camera
- Manufacturer-specific camera apps (Xiaomi, OnePlus, etc.)

### Social Media & Communication
- Instagram, Snapchat, WhatsApp, Facebook
- TikTok, Twitter/X, WeChat, Discord
- Zoom, Skype, Microsoft Teams, Google Meet

## Installation

### Prerequisites
- Rooted Android device with **Magisk 24.3+** or **KernelSU**
- **Zygisk enabled** in your root manager
- Android 10+ (API 29 or higher)

### Installation Steps

1. **Download** the latest `MagicCam-v1.0.0.zip` from [Releases](https://github.com/YOUR_USERNAME/magic-cam/releases)

2. **Install via Magisk Manager**:
   - Open Magisk Manager
   - Go to Modules tab
   - Tap "Install from storage"
   - Select the downloaded ZIP file
   - Reboot when prompted

3. **Verify Installation**:
   - Check that Zygisk is enabled in Magisk settings
   - Open any camera app - the module hooks automatically
   - Check logs: `logcat | grep MagicCam`

## Configuration

### Virtual Video Sources
Place video files in `/sdcard/MagicCam/` directory:
```bash
# Create directory
mkdir -p /sdcard/MagicCam

# Copy your video file
cp your_video.mp4 /sdcard/MagicCam/default.mp4
```

### Supported Video Formats
- MP4 (H.264/H.265)
- AVI, MOV, MKV
- Resolution: Up to 4K (3840x2160)

## Technical Details

### Architecture
- **Language**: C++17 with Android NDK
- **Framework**: Zygisk module system
- **Hooking**: Camera2 NDK API and HAL layer
- **Detection Bypass**: Multi-layered stealth mechanisms

### Hooked APIs
- `ACameraManager_create`
- `ACameraManager_getCameraIdList` 
- `ACameraManager_openCamera`
- Camera HAL functions
- Media framework APIs

### Detection Bypass Features
- Process name spoofing
- File system access blocking
- System property spoofing
- Root indicator hiding
- SafetyNet bypass techniques
- Memory map patching

## Building from Source

### Prerequisites
- Android NDK r25c+
- CMake 3.22.1+
- Linux/macOS build environment

### Build Steps
```bash
# Clone repository
git clone https://github.com/YOUR_USERNAME/magic-cam.git
cd magic-cam

# Set NDK path
export ANDROID_NDK_ROOT=/path/to/ndk

# Build module
chmod +x build.sh
./build.sh

# Install generated ZIP
# MagicCam-v1.0.0.zip will be created
```

## Troubleshooting

### Module Not Working
1. **Check Zygisk**: Ensure Zygisk is enabled in Magisk
2. **Check Logs**: `logcat | grep MagicCam`
3. **Verify Root**: Confirm device is properly rooted
4. **Android Version**: Requires Android 10+ (API 29+)

### App Detection
Some apps have advanced detection mechanisms:
- Banking apps may detect root despite bypass
- Some social media apps update detection regularly
- Module includes comprehensive bypass but 100% invisibility isn't guaranteed

### Debug Commands
```bash
# Check module status
ls -la /data/adb/modules/com_twj_mc

# View module logs
logcat | grep MagicCam

# Check Zygisk status
magisk --sqlite "SELECT * FROM modules WHERE id='com_twj_mc'"

# Test camera access
am start -n com.android.camera/.Camera
```

## Security & Privacy

This module is designed for:
- **Educational purposes**: Learning Android internals
- **Privacy protection**: Virtual backgrounds and camera control
- **Security research**: Testing app behavior
- **Development**: App testing with virtual camera input

**Important**: Use responsibly and comply with local laws and app terms of service.

## Contributing

1. Fork the repository
2. Create feature branch: `git checkout -b feature-name`
3. Make changes and test thoroughly
4. Commit: `git commit -am 'Add feature'`
5. Push: `git push origin feature-name`
6. Create Pull Request

## Changelog

### v1.0.0
- Initial release with Zygisk framework
- Multi-architecture support (arm64-v8a, armeabi-v7a)
- Camera2 NDK API hooking
- Comprehensive detection bypass
- Support for 30+ popular apps

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) for details.

## Disclaimer

This software is for educational and research purposes only. Users are responsible for complying with applicable laws and terms of service. The developers assume no liability for misuse.

## Acknowledgments

- [Magisk](https://github.com/topjohnwu/Magisk) and Zygisk framework
- Android Camera2 NDK documentation
- Open source camera hooking research