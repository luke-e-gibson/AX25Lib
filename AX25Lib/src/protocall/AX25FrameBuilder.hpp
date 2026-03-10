#include "AX25Config.hpp"
#include <vector>
#include <cstdint>

class AX25FrameBuilder
{
public:
    AX25FrameBuilder(const AX25Config& config);
    AX25FrameBuilder(const AX25Config& config, const std::vector<uint8_t>& control, const std::vector<uint8_t>& pid);
    ~AX25FrameBuilder() = default;
    
    std::vector<uint8_t> buildAx25Frame(const std::vector<uint8_t>& payload);
    std::vector<uint8_t> buildKissFrame(const std::vector<uint8_t>& payload);

private:
    AX25Config m_config;
    
    std::vector<uint8_t> m_callFrameTo; // To send to
    std::vector<uint8_t> m_callFrameFrom; // From us
    std::vector<uint8_t> m_control;
    std::vector<uint8_t> m_pid;
    
    static std::vector<uint8_t> m_buildAX25Call(const std::string& callsign, int ssid, bool last);
};