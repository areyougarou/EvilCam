#pragma once

#include <jni.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraCaptureSession.h>
#include <media/NdkImageReader.h>
#include <android/native_window.h>

namespace magiccam {

class CameraHook {
public:
    static CameraHook& getInstance();
    
    bool initialize();
    void hookCameraAPIs();
    void setVirtualVideoSource(const char* videoPath);
    void enableVirtualMode(bool enable);
    
private:
    CameraHook() = default;
    ~CameraHook() = default;
    
    // Prevent copying
    CameraHook(const CameraHook&) = delete;
    CameraHook& operator=(const CameraHook&) = delete;
    
    static void* hookCameraOpen(const char* cameraId);
    static int hookCameraCapture(void* session, void* request);
    static void injectVirtualFrame(ANativeWindow* window);
    
    bool m_initialized = false;
    bool m_virtualMode = false;
    std::string m_videoPath;
    void* m_originalCameraOpen = nullptr;
    void* m_originalCameraCapture = nullptr;
};

} // namespace magiccam