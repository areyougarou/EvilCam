#include <jni.h>
#include <android/log.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <fstream>

#include "zygisk.hpp"
#include "camera_hook.hpp"
#include "detection_bypass.hpp"

// Use completely generic log tag for maximum stealth
#define LOG_TAG "SystemService"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace zygisk;

class SystemCameraModule : public ModuleBase {
private:
    Api *api = nullptr;
    JNIEnv *env = nullptr;
    bool shouldHook = false;
    std::vector<std::string> targetApps;

public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
        LOGI("System camera service loaded");
        
        // Load target apps from configuration file - NO FALLBACK
        loadTargetApps();
        
        // Initialize minimal detection bypass
        magiccam::DetectionBypass::getInstance().initialize();
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        if (!args || !args->nice_name) return;
        
        const char *packageName = env->GetStringUTFChars(args->nice_name, nullptr);
        if (!packageName) return;
        
        LOGD("Checking application: %s", packageName);
        
        // Only hook if app is in user-configured target list
        std::string pkg(packageName);
        for (const auto &target : targetApps) {
            if (pkg == target) {
                shouldHook = true;
                LOGI("Target application detected: %s - preparing optimization", packageName);
                break;
            }
        }
        
        env->ReleaseStringUTFChars(args->nice_name, packageName);
        
        if (shouldHook) {
            auto& bypass = magiccam::DetectionBypass::getInstance();
            bypass.obfuscateModulePresence();
            LOGI("Camera service optimization activated");
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        if (!shouldHook) return;
        
        const char *packageName = env->GetStringUTFChars(args->nice_name, nullptr);
        LOGI("Initializing camera optimization for: %s", packageName ? packageName : "target");
        
        // Initialize camera hooking system
        auto& cameraHook = magiccam::CameraHook::getInstance();
        if (cameraHook.initialize()) {
            cameraHook.hookCameraAPIs();
            cameraHook.enableVirtualMode(true);
            cameraHook.setVirtualVideoSource("/sdcard/MagicCam/default.mp4");
            LOGI("Camera service hooks successfully installed");
        } else {
            LOGE("Failed to initialize camera service hooks");
        }
        
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

private:
    void loadTargetApps() {
        // Load target apps ONLY from configuration file - NO HARDCODED FALLBACK
        const char* configPath = "/data/adb/modules/twj_mc/target_apps.txt";
        
        std::ifstream file(configPath);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty() && line[0] != '#') {
                    targetApps.push_back(line);
                }
            }
            file.close();
            LOGI("Loaded %zu target applications from config", targetApps.size());
        } else {
            LOGW("Target apps config not found - module will not hook any apps");
            // NO FALLBACK - module remains completely inactive until user configures it
        }
        
        if (targetApps.empty()) {
            LOGI("No target applications configured - module inactive");
        }
    }
};

// Register the Zygisk module
REGISTER_ZYGISK_MODULE(SystemCameraModule)