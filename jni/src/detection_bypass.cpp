#include "detection_bypass.hpp"
#include <android/log.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <dlfcn.h>
#include <string.h>

// Use completely generic log tag to avoid detection
#define LOG_TAG "SystemService"
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
    
    LOGI("Initializing system service optimization");
    
    useObfuscatedNames();
    
    m_initialized = true;
    LOGI("System service optimization initialized");
}

void DetectionBypass::useObfuscatedNames() {
    LOGI("Applying system process optimization");
    
    // Change process name to something completely innocent
    const char* innocuousNames[] = {
        "system_server",
        "com.android.systemui", 
        "com.google.android.gms",
        "android.process.media",
        "com.android.providers.media"
    };
    
    // Use a generic system process name
    const char* newName = innocuousNames[getpid() % 5];
    prctl(PR_SET_NAME, newName, 0, 0, 0);
    
    LOGD("Process optimization applied: %s", newName);
}

void DetectionBypass::obfuscateModulePresence() {
    LOGI("Applying stealth optimization");
    
    // Just basic process name obfuscation - no aggressive hiding
    useObfuscatedNames();
}

void DetectionBypass::hideModuleFiles() {
    LOGI("File access optimization applied");
    
    // Minimal file hiding - rely on external protection
    // No aggressive file system manipulation
}

} // namespace magiccam