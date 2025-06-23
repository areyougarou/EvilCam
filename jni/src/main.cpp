#include <jni.h>
#include <android/log.h>
#include <string>
#include <vector>
#include <unistd.h>

#include "zygisk.hpp"
#include "camera_hook.hpp"
#include "detection_bypass.hpp"

// Use obfuscated log tag
#define LOG_TAG "SystemService"
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
        // Camera apps
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
        "com.motorola.camera3",
        "com.asus.camera",
        // Social media and communication apps
        "com.instagram.android",
        "com.snapchat.android",
        "com.whatsapp",
        "com.facebook.katana",
        "com.facebook.orca", // Messenger
        "com.twitter.android",
        "com.tencent.mm", // WeChat
        "com.ss.android.ugc.trill", // TikTok
        "com.zhiliaoapp.musically", // TikTok alternative
        "us.zoom.videomeetings",
        "com.skype.raider",
        "com.discord",
        "com.microsoft.teams",
        "com.google.android.apps.meetings", // Google Meet
        "com.viber.voip",
        "org.telegram.messenger",
        "com.linkedin.android",
        "com.pinterest",
        "com.reddit.frontpage",
        // Dating apps
        "com.tinder",
        "com.bumble.app",
        "com.match.android.matchmobile",
        // Live streaming
        "tv.twitch.android.app",
        "com.google.android.youtube",
        "com.facebook.pages.app"
    };

public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
        LOGI("Service module loaded"); // Obfuscated log message
        
        // Initialize minimal detection bypass
        magiccam::DetectionBypass::getInstance().initialize();
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        if (!args || !args->nice_name) return;
        
        const char *packageName = env->GetStringUTFChars(args->nice_name, nullptr);
        if (!packageName) return;
        
        LOGD("Checking target: %s", packageName);
        
        // Check if this is a target app
        std::string pkg(packageName);
        for (const auto &target : targetApps) {
            if (pkg == target) {
                shouldHook = true;
                LOGI("Target detected: %s - preparing service", packageName);
                break;
            }
        }
        
        env->ReleaseStringUTFChars(args->nice_name, packageName);
        
        if (shouldHook) {
            // Only basic obfuscation
            auto& bypass = magiccam::DetectionBypass::getInstance();
            bypass.obfuscateModulePresence();
            
            LOGI("Service obfuscation activated");
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        if (!shouldHook) return;
        
        const char *packageName = env->GetStringUTFChars(args->nice_name, nullptr);
        LOGI("Initializing service for: %s", packageName ? packageName : "target");
        
        // Initialize camera hooking system
        auto& cameraHook = magiccam::CameraHook::getInstance();
        if (cameraHook.initialize()) {
            cameraHook.hookCameraAPIs();
            cameraHook.enableVirtualMode(true);
            
            // Set default virtual video source if available
            cameraHook.setVirtualVideoSource("/sdcard/MagicCam/default.mp4");
            
            LOGI("Service hooks successfully installed");
        } else {
            LOGE("Failed to initialize service hooks");
        }
        
        // Hide module files
        magiccam::DetectionBypass::getInstance().hideModuleFiles();
        
        if (packageName) {
            env->ReleaseStringUTFChars(args->nice_name, packageName);
        }
    }

    void preServerSpecialize(ServerSpecializeArgs *args) override {
        // Server process - no action needed
    }

    void postServerSpecialize(const ServerSpecializeArgs *args) override {
        // Server process - no action needed
    }
};

// Register the Zygisk module
REGISTER_ZYGISK_MODULE(MagicCamModule)