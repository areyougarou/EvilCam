#include "detection_bypass.hpp"
#include <android/log.h>
#include <unistd.h>
#include <sys/prctl.h>

#define LOG_TAG "SystemService" // Obfuscated log tag
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace magiccam {

DetectionBypass& DetectionBypass::getInstance() {
    static DetectionBypass instance;
    return instance;
}

void DetectionBypass::initialize() {
    if (m_initialized) return;
    
    LOGI("Initializing basic obfuscation");
    
    useObfuscatedNames();
    
    m_initialized = true;
    LOGI("Basic obfuscation initialized");
}

void DetectionBypass::useObfuscatedNames() {
    LOGI("Using obfuscated process names");
    
    // Change process name to something generic
    const char* innocuousNames[] = {
        "system_server",
        "com.android.systemui", 
        "com.google.android.gms",
        "android.process.media"
    };
    
    // Use a generic system process name
    const char* newName = innocuousNames[getpid() % 4];
    prctl(PR_SET_NAME, newName, 0, 0, 0);
    
    LOGD("Process name obfuscated to: %s", newName);
}

void DetectionBypass::obfuscateModulePresence() {
    LOGI("Obfuscating module presence");
    
    // Just basic process name obfuscation
    useObfuscatedNames();
}

void DetectionBypass::hideModuleFiles() {
    LOGI("Module file hiding disabled - using external protection");
    
    // No file hiding since you already have path protection
}

} // namespace magiccam