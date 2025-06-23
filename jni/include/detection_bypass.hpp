#pragma once

#include <string>

namespace magiccam {

class DetectionBypass {
public:
    static DetectionBypass& getInstance();
    
    void initialize();
    void obfuscateModulePresence();
    void hideModuleFiles();
    
private:
    DetectionBypass() = default;
    ~DetectionBypass() = default;
    
    DetectionBypass(const DetectionBypass&) = delete;
    DetectionBypass& operator=(const DetectionBypass&) = delete;
    
    void useObfuscatedNames();
    
    bool m_initialized = false;
};

} // namespace magiccam