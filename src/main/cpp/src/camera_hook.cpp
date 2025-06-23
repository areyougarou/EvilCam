#include "camera_hook.h"
#include <android/log.h>
#include <dlfcn.h>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

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

// Virtual camera data
static bool g_virtualMode = false;
static std::string g_videoPath;
static uint8_t* g_virtualFrameData = nullptr;
static size_t g_virtualFrameSize = 0;
static int g_virtualWidth = 1920;
static int g_virtualHeight = 1080;

CameraHook& CameraHook::getInstance() {
    static CameraHook instance;
    return instance;
}

bool CameraHook::initialize() {
    if (m_initialized) return true;
    
    LOGI("Initializing camera hook system");
    
    // Load camera2ndk library
    void* camera2ndk = dlopen("libcamera2ndk.so", RTLD_NOW);
    if (!camera2ndk) {
        LOGE("Failed to load libcamera2ndk.so: %s", dlerror());
        return false;
    }
    
    // Get original function addresses
    original_ACameraManager_create = (ACameraManager_create_t)dlsym(camera2ndk, "ACameraManager_create");
    original_ACameraManager_getCameraIdList = (ACameraManager_getCameraIdList_t)dlsym(camera2ndk, "ACameraManager_getCameraIdList");
    original_ACameraManager_openCamera = (ACameraManager_openCamera_t)dlsym(camera2ndk, "ACameraManager_openCamera");
    
    if (!original_ACameraManager_create || !original_ACameraManager_getCameraIdList || !original_ACameraManager_openCamera) {
        LOGE("Failed to get original camera function addresses");
        return false;
    }
    
    // Initialize virtual frame data (solid color for now)
    g_virtualFrameSize = g_virtualWidth * g_virtualHeight * 3; // RGB24
    g_virtualFrameData = new uint8_t[g_virtualFrameSize];
    
    // Fill with a test pattern (gradient)
    for (int y = 0; y < g_virtualHeight; y++) {
        for (int x = 0; x < g_virtualWidth; x++) {
            int offset = (y * g_virtualWidth + x) * 3;
            g_virtualFrameData[offset] = (x * 255) / g_virtualWidth;     // R
            g_virtualFrameData[offset + 1] = (y * 255) / g_virtualHeight; // G
            g_virtualFrameData[offset + 2] = 128;                         // B
        }
    }
    
    m_initialized = true;
    LOGI("Camera hook system initialized successfully");
    return true;
}

// Hooked camera functions
ACameraManager* hooked_ACameraManager_create() {
    LOGD("ACameraManager_create called - hooking camera access");
    return original_ACameraManager_create();
}

camera_status_t hooked_ACameraManager_getCameraIdList(ACameraManager* manager, ACameraIdList** cameraIdList) {
    LOGD("ACameraManager_getCameraIdList called");
    camera_status_t result = original_ACameraManager_getCameraIdList(manager, cameraIdList);
    
    if (result == ACAMERA_OK && g_virtualMode) {
        LOGI("Intercepting camera ID list - virtual mode active");
        // We could modify the camera list here if needed
    }
    
    return result;
}

camera_status_t hooked_ACameraManager_openCamera(ACameraManager* manager, const char* cameraId, ACameraDevice_Callback* callback, ACameraDevice** device) {
    LOGD("ACameraManager_openCamera called for camera: %s", cameraId);
    
    if (g_virtualMode) {
        LOGI("Opening camera in virtual mode");
        // Here we would create a virtual camera device
        // For now, we'll allow the real camera to open but intercept its data later
    }
    
    return original_ACameraManager_openCamera(manager, cameraId, callback, device);
}

#define HOOK_SYMBOL(handle, symbol) \
    do { \
        void* addr = dlsym(handle, #symbol); \
        if (addr) { \
            if (mprotect((void*)((uintptr_t)addr & ~(getpagesize() - 1)), getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC) == 0) { \
                *(void**)&original_##symbol = addr; \
                *(void**)addr = (void*)hooked_##symbol; \
                LOGD("Successfully hooked %s", #symbol); \
            } else { \
                LOGE("Failed to change protection for %s", #symbol); \
            } \
        } else { \
            LOGE("Failed to find symbol %s", #symbol); \
        } \
    } while(0)

void CameraHook::hookCameraAPIs() {
    if (!m_initialized) {
        LOGE("Camera hook system not initialized");
        return;
    }
    
    LOGI("Installing camera API hooks");
    
    // Hook Camera2 NDK functions
    void* camera2ndk = dlopen("libcamera2ndk.so", RTLD_NOW | RTLD_GLOBAL);
    if (camera2ndk) {
        HOOK_SYMBOL(camera2ndk, ACameraManager_create);
        HOOK_SYMBOL(camera2ndk, ACameraManager_getCameraIdList);
        HOOK_SYMBOL(camera2ndk, ACameraManager_openCamera);
    }
    
    // Also hook Camera1 API if present
    void* libcamera_client = dlopen("libcamera_client.so", RTLD_NOW | RTLD_GLOBAL);
    if (libcamera_client) {
        LOGD("Camera1 API library found - could hook legacy functions");
    }
    
    LOGI("Camera API hooks installed successfully");
}

void CameraHook::setVirtualVideoSource(const char* videoPath) {
    if (videoPath) {
        m_videoPath = videoPath;
        LOGI("Virtual video source set to: %s", videoPath);
        
        // TODO: Load video frames from file
        // For now we use the test pattern
    }
}

void CameraHook::enableVirtualMode(bool enable) {
    m_virtualMode = enable;
    g_virtualMode = enable;
    LOGI("Virtual camera mode %s", enable ? "enabled" : "disabled");
}

void* CameraHook::hookCameraOpen(const char* cameraId) {
    LOGD("Camera open hook called for: %s", cameraId);
    return nullptr; // Simplified for this example
}

int CameraHook::hookCameraCapture(void* session, void* request) {
    LOGD("Camera capture hook called");
    return 0; // Simplified for this example
}

void CameraHook::injectVirtualFrame(ANativeWindow* window) {
    if (!window || !g_virtualMode || !g_virtualFrameData) return;
    
    LOGD("Injecting virtual frame");
    
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(window, &buffer, nullptr) == 0) {
        // Copy our virtual frame data to the window buffer
        if (buffer.bits && buffer.width > 0 && buffer.height > 0) {
            // Convert our RGB data to the window's format
            uint8_t* dst = (uint8_t*)buffer.bits;
            uint8_t* src = g_virtualFrameData;
            
            int copyWidth = std::min(buffer.width, g_virtualWidth);
            int copyHeight = std::min(buffer.height, g_virtualHeight);
            
            for (int y = 0; y < copyHeight; y++) {
                memcpy(dst + y * buffer.stride * 4, src + y * g_virtualWidth * 3, copyWidth * 3);
            }
        }
        
        ANativeWindow_unlockAndPost(window);
    }
}

} // namespace magiccam