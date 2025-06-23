#include <jni.h>
#include <android/log.h>
#include <string>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/system_properties.h>

#include "zygisk.h"
#include "camera_hook.h"
#include "detection_bypass.h"

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
        // Social media apps
        "com.instagram.android",
        "com.snapchat.android",
        "com.whatsapp",
        "com.facebook.katana",
        "com.twitter.android",
        "com.tencent.mm", // WeChat
        "com.ss.android.ugc.trill", // TikTok
        "us.zoom.videomeetings",
        "com.skype.raider",
        "com.discord"
    };

public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
        LOGI("MagicCam module loaded");
        
        // Initialize detection bypass early
        magiccam::DetectionBypass::getInstance().initialize();
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
        
        if (shouldHook) {
            // Enable all detection bypass methods
            magiccam::DetectionBypass::getInstance().hideFromProcessList();
            magiccam::DetectionBypass::getInstance().spoofSystemProperties();
            magiccam::DetectionBypass::getInstance().bypassRootDetection();
            magiccam::DetectionBypass::getInstance().hideXposedTraces();
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        if (!shouldHook) return;
        
        LOGI("Initializing camera hooks for target app");
        
        // Initialize camera hooking
        if (magiccam::CameraHook::getInstance().initialize()) {
            magiccam::CameraHook::getInstance().hookCameraAPIs();
            magiccam::CameraHook::getInstance().enableVirtualMode(true);
            LOGI("Camera hooks successfully installed");
        } else {
            LOGE("Failed to initialize camera hooks");
        }
        
        // Additional stealth measures
        magiccam::DetectionBypass::getInstance().hideFromFileSystem();
    }
};

// Register the module
REGISTER_ZYGISK_MODULE(MagicCamModule)

// JNI functions for the Android app
extern "C" JNIEXPORT void JNICALL
Java_com_twj_mc_MainActivity_initializeModule(JNIEnv *env, jobject thiz) {
    LOGI("Module initialization requested from Java");
    
    // Initialize detection bypass
    magiccam::DetectionBypass::getInstance().initialize();
    
    // Initialize camera hook system
    if (magiccam::CameraHook::getInstance().initialize()) {
        LOGI("Camera system initialized successfully");
    } else {
        LOGE("Failed to initialize camera system");
    }
}