#pragma once

#include <string>
#include <vector>

namespace magiccam {

class DetectionBypass {
public:
    static DetectionBypass& getInstance();
    
    void initialize();
    void hideFromProcessList();
    void hideFromFileSystem();
    void spoofSystemProperties();
    void bypassRootDetection();
    void hideXposedTraces();
    
private:
    DetectionBypass() = default;
    ~DetectionBypass() = default;
    
    DetectionBypass(const DetectionBypass&) = delete;
    DetectionBypass& operator=(const DetectionBypass&) = delete;
    
    void hookSystemCalls();
    void hideModuleSignature();
    void spoofBuildProps();
    void bypassSafetyNet();
    
    std::vector<std::string> m_hiddenFiles;
    std::vector<std::string> m_spoofedProps;
    bool m_initialized = false;
};

} // namespace magiccam