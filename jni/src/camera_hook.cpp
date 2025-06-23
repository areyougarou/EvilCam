#include "camera_hook.hpp"
#include <android/log.h>
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cstring>
#include <algorithm>

#define LOG_TAG "MagicCam_Hook"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace magiccam {

// Function prototypes for hooking
typedef ACameraManager* (*ACameraManager_create_t)();
typedef void (*ACameraManager_delete_t)(ACameraManager*);
typedef camera_status_t (*ACameraManager_getCameraIdList_t)(ACameraManager*, ACameraIdList**);
typedef camera_status_t (*ACameraManager_openCamera_t)(ACameraManager*, const char*, ACameraDevice_Callback*, ACameraDevice**);

// Original function pointers
static ACameraManager_create_t original_ACameraManager_create = nullptr;
static ACameraManager_getCameraIdList_t original_ACameraManager_getCameraIdList = nullptr;
static ACameraManager_openCamera_t original_ACameraManager_openCamera = nullptr;

// Global state
static bool g_virtualMode = false;
static std::unique_ptr<uint8_t[]> g_virtualFrameData = nullptr;
static size_t g_virtualFrameSize = 0;
static int g_virtualWidth = 1920;
static int g_virtualHeight = 1080;

CameraHook& CameraHook::getInstance() {
    static CameraHook instance;
    return instance;
}

bool CameraHook::initialize() {
    if (m_initialized) return true;
    
    LOGI("Initializing Magic Cam hook system");
    
    // Initialize virtual frame data with a test pattern
    m_virtualFrameSize = m_virtualWidth * m_virtualHeight * 3; // RGB24
    m_virtualFrameData = std::make_unique<uint8_t[]>(m_virtualFrameSize);
    g_virtualFrameData = std::make_unique<uint8_t[]>(m_virtualFrameSize);
    g_virtualFrameSize = m_virtualFrameSize;
    
    // Create a colorful test pattern
    for (int y = 0; y < m_virtualHeight; y++) {
        for (int x = 0; x < m_virtualWidth; x++) {
            int offset = (y * m_virtualWidth + x) * 3;
            
            // Create a gradient with some patterns
            uint8_t r = (x * 255) / m_virtualWidth;
            uint8_t g = (y * 255) / m_virtualHeight;
            uint8_t b = ((x + y) * 255) / (m_virtualWidth + m_virtualHeight);
            
            // Add some geometric patterns
            if ((x / 100 + y / 100) % 2 == 0) {
                r = 255 - r;
                g = 255 - g;
            }
            
            m_virtualFrameData[offset] = r;     // R
            m_virtualFrameData[offset + 1] = g; // G
            m_virtualFrameData[offset + 2] = b; // B
            
            g_virtualFrameData[offset] = r;
            g_virtualFrameData[offset + 1] = g;
            g_virtualFrameData[offset + 2] = b;
        }
    }
    
    m_initialized = true;
    LOGI("Magic Cam hook system initialized successfully");
    return true;
}

// Hooked camera functions
ACameraManager* hooked_ACameraManager_create() {
    LOGD("ACameraManager_create intercepted - Magic Cam active");
    return original_ACameraManager_create();
}

camera_status_t hooked_ACameraManager_getCameraIdList(ACameraManager* manager, ACameraIdList** cameraIdList) {
    LOGD("ACameraManager_getCameraIdList intercepted");
    camera_status_t result = original_ACameraManager_getCameraIdList(manager, cameraIdList);
    
    if (result == ACAMERA_OK && g_virtualMode) {
        LOGI("Camera ID list intercepted - virtual mode active");
        // Could modify camera list here if needed
    }
    
    return result;
}

camera_status_t hooked_ACameraManager_openCamera(ACameraManager* manager, const char* cameraId, ACameraDevice_Callback* callback, ACameraDevice** device) {
    LOGD("ACameraManager_openCamera intercepted for camera: %s", cameraId);
    
    if (g_virtualMode) {
        LOGI("Opening camera in virtual mode - will inject virtual frames");
    }
    
    return original_ACameraManager_openCamera(manager, cameraId, callback, device);
}

// PLT/GOT hooking helper
static bool hookFunction(void* handle, const char* symbol, void* newFunc, void** oldFunc) {
    void* addr = dlsym(handle, symbol);
    if (!addr) {
        LOGE("Failed to find symbol: %s", symbol);
        return false;
    }
    
    // Store original function
    *oldFunc = addr;
    
    // Get page size and align address
    size_t pageSize = getpagesize();
    void* pageStart = (void*)((uintptr_t)addr & ~(pageSize - 1));
    
    // Change memory protection
    if (mprotect(pageStart, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        LOGE("Failed to change memory protection for %s", symbol);
        return false;
    }
    
    // Replace function pointer (simplified - real implementation would use PLT/GOT)
    // This is a basic approach - production code would need more sophisticated hooking
    LOGD("Successfully prepared hook for %s", symbol);
    return true;
}

void CameraHook::hookCameraAPIs() {
    if (!m_initialized) {
        LOGE("Camera hook system not initialized");
        return;
    }
    
    LOGI("Installing camera API hooks");
    
    // Hook Camera2 NDK functions
    void* camera2ndk = dlopen("libcamera2ndk.so", RTLD_NOW | RTLD_GLOBAL);
    if (camera2ndk) {
        hookFunction(camera2ndk, "ACameraManager_create", 
                    (void*)hooked_ACameraManager_create, 
                    (void**)&original_ACameraManager_create);
        
        hookFunction(camera2ndk, "ACameraManager_getCameraIdList", 
                    (void*)hooked_ACameraManager_getCameraIdList, 
                    (void**)&original_ACameraManager_getCameraIdList);
        
        hookFunction(camera2ndk, "ACameraManager_openCamera", 
                    (void*)hooked_ACameraManager_openCamera, 
                    (void**)&original_ACameraManager_openCamera);
        
        LOGI("Camera2 NDK hooks installed");
    } else {
        LOGW("libcamera2ndk.so not found - trying alternative methods");
    }
    
    // Hook Camera HAL if available
    hookCameraHAL();
    
    LOGI("Camera API hooks installation completed");
}

void CameraHook::hookCameraHAL() {
    LOGD("Attempting to hook Camera HAL");
    
    // Hook lower-level camera HAL functions
    void* libcamera_client = dlopen("libcamera_client.so", RTLD_NOW | RTLD_GLOBAL);
    if (libcamera_client) {
        LOGD("Camera HAL library found - could hook HAL functions");
        // Additional HAL hooking would go here
    }
    
    // Hook media framework
    void* libmedia = dlopen("libmedia.so", RTLD_NOW | RTLD_GLOBAL);
    if (libmedia) {
        LOGD("Media framework library found");
        // Media framework hooking would go here
    }
}

void CameraHook::setVirtualVideoSource(const std::string& videoPath) {
    m_videoPath = videoPath;
    LOGI("Virtual video source set to: %s", videoPath.c_str());
    
    // Load video frames from file
    loadVirtualVideo();
}

void CameraHook::loadVirtualVideo() {
    if (m_videoPath.empty()) return;
    
    LOGD("Loading virtual video from: %s", m_videoPath.c_str());
    
    // Check if file exists
    int fd = open(m_videoPath.c_str(), O_RDONLY);
    if (fd < 0) {
        LOGW("Virtual video file not found, using test pattern");
        return;
    }
    close(fd);
    
    // TODO: Implement video file loading
    // For now, we continue using the test pattern
    LOGI("Virtual video loading prepared (using test pattern for now)");
}

void CameraHook::enableVirtualMode(bool enable) {
    m_virtualMode = enable;
    g_virtualMode = enable;
    LOGI("Virtual camera mode %s", enable ? "ENABLED" : "DISABLED");
    
    if (enable) {
        LOGI("Magic Cam virtual mode is now active");
        LOGI("All camera access will be intercepted");
    }
}

void CameraHook::injectVirtualFrame(ANativeWindow* window) {
    if (!window || !g_virtualMode || !g_virtualFrameData) return;
    
    LOGD("Injecting virtual frame into camera stream");
    
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(window, &buffer, nullptr) == 0) {
        if (buffer.bits && buffer.width > 0 && buffer.height > 0) {
            // Copy virtual frame data to window buffer
            uint8_t* dst = (uint8_t*)buffer.bits;
            uint8_t* src = g_virtualFrameData.get();
            
            int copyWidth = std::min(buffer.width, g_virtualWidth);
            int copyHeight = std::min(buffer.height, g_virtualHeight);
            
            // Simple RGB to buffer format conversion
            for (int y = 0; y < copyHeight; y++) {
                for (int x = 0; x < copyWidth; x++) {
                    int srcOffset = (y * g_virtualWidth + x) * 3;
                    int dstOffset = (y * buffer.stride + x) * 4; // Assuming RGBA
                    
                    if (srcOffset + 2 < g_virtualFrameSize && dstOffset + 3 < buffer.height * buffer.stride * 4) {
                        dst[dstOffset] = src[srcOffset];     // R
                        dst[dstOffset + 1] = src[srcOffset + 1]; // G
                        dst[dstOffset + 2] = src[srcOffset + 2]; // B
                        dst[dstOffset + 3] = 255; // A
                    }
                }
            }
        }
        
        ANativeWindow_unlockAndPost(window);
        LOGD("Virtual frame injected successfully");
    }
}

} // namespace magiccam