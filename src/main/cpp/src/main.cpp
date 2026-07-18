#include <jni.h>
#include <android/log.h>
#include <string>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/system_properties.h>

#include "zygisk.h"
#include "camera_hook.h"
#include "strengther.h"

#define LOG_TAG "MagicCam"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace zygisk;

class MagicCamModule : public ModuleBase {
private:
    Api *api = nullptr;
    JNIEnv *env = nullptr;
    bool shouldHook = false;
    
    // Target apps for camera hooking
    const std::vector<std::string> targetApps = {
        "com.android.camera",
        "com.android.camera2", 
        "com.google.android.GoogleCamera",
        "com.sec.android.app.camera",
        "com.huawei.camera",
        "com.xiaomi.camera",
        "com.oneplus.camera",
        "com.oppo.camera",
        "com.vivo.camera",
        "org.lineageos.snap",
};

public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
        LOGI("MagicCam module loaded");
        
        magiccam::strengther::getInstance().initialize();
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        if (!args || !args->nice_name) return;
        
        const char *packageName = env->GetStringUTFChars(args->nice_name, nullptr);
        if (!packageName) return;
        
        LOGD("Checking package: %s", packageName);
        
        // Check if this is a target app
        std::string pkg(packageName);
        for (const auto &target : targetApps) {
            if (pkg == target) {
                shouldHook = true;
                LOGI("Target app detected: %s", packageName);
                break;
            }
        }
        
        env->ReleaseStringUTFChars(args->nice_name, packageName);
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        if (!shouldHook) return;
        
        LOGI("Initializing camera");
        
        // Initialize camera hooking
        if (magiccam::CameraHook::getInstance().initialize()) {
            magiccam::CameraHook::getInstance().hookCameraAPIs();
            magiccam::CameraHook::getInstance().enableVirtualMode(true);
            LOGI("Camera hooks successfully installed");
        } else {
            LOGE("Failed to initialize camera hooks");
        }
        
        // Additional stealth measures
        magiccam::strengther::getInstance().hideFromFileSystem();
    }
};

// Register the module
REGISTER_ZYGISK_MODULE(MagicCamModule)

// JNI functions for the Android app
extern "C" JNIEXPORT void JNICALL
Java_com_twj_mc_MainActivity_initializeModule(JNIEnv *env, jobject thiz) {
    LOGI("Module initialization requested from Java");
    
    // Initialize detection bypass
    evilcam::strengther::getInstance().initialize();
    
    // Initialize camera hook system
    if (evilcam::CameraHook::getInstance().initialize()) {
        LOGI("Camera system initialized successfully");
    } else {
        LOGE("Failed to initialize camera system");
    }
}
