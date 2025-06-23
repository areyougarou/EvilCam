#pragma once

#include <jni.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraCaptureSession.h>
#include <media/NdkImageReader.h>
#include <android/native_window.h>
#include <string>
#include <memory>

namespace magiccam {

class CameraHook {
public:
    static CameraHook& getInstance();
    
    bool initialize();
    void hookCameraAPIs();
    void setVirtualVideoSource(const std::string& videoPath);
    void enableVirtualMode(bool enable);
    bool isVirtualModeEnabled() const { return m_virtualMode; }
    
private:
    CameraHook() = default;
    ~CameraHook() = default;
    
    CameraHook(const CameraHook&) = delete;
    CameraHook& operator=(const CameraHook&) = delete;
    
    void hookCamera2NDK();
    void hookCameraHAL();
    void injectVirtualFrame(ANativeWindow* window);
    void loadVirtualVideo();
    
    bool m_initialized = false;
    bool m_virtualMode = false;
    std::string m_videoPath;
    std::unique_ptr<uint8_t[]> m_virtualFrameData;
    size_t m_virtualFrameSize = 0;
    int m_virtualWidth = 1920;
    int m_virtualHeight = 1080;
};

} // namespace magiccam