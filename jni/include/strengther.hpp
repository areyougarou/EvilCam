#pragma once

#include <string>

namespace evilcam {

class strengther {
public:
    static strengther& getInstance();
    
    void initialize();
    void strengthModulePresence();
    void strengthModuleFiles();
    
private:
    strengther() = default;
    ~strengther () = default;
    
    strengther(const strengther&) = delete;
    strengther& operator=(const strengther&) = delete;
    
    void useStrengthedNames();
    
    bool m_initialized = false;
};

} // namespace magiccam
