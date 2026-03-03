#include <algorithm>
#include <cctype>
#include <string>
#include <cstdint>

#include "AX25FrameBuilder.hpp"
#include "AX25Util.hpp"

AX25FrameBuilder::AX25FrameBuilder(const AX25Config& config) : m_config(config)
{
    // Destination is not marked as last; source terminates the address list.
    m_callFrameTo = m_buildAX25Call(config.callsignTo, config.ssidTo, false);
    m_callFrameFrom = m_buildAX25Call(config.callsignFrom, config.ssidFrom, true);
}

std::vector<uint8_t> AX25FrameBuilder::buildKissFrame(const std::vector<uint8_t>& payload)
{
    std::vector<uint8_t> frame;
    
    frame.reserve(2 + payload.size() + 1); // Reserve space for header, payload, and FCS
    frame.push_back(0xC0); // Flag
    frame.push_back(0x00); // Address field (placeholder)
    
    auto escapedPayload = AX25Util::kissEscape(payload);
    frame.insert(frame.end(), escapedPayload.begin(), escapedPayload.end());
    
    frame.push_back(0xC0); // Flag
    
    return frame;
}

std::vector<uint8_t> AX25FrameBuilder::buildAx25Frame(const std::vector<uint8_t>& payload)
{
    constexpr uint8_t CONTROL = 0x03; // UI frame
    constexpr uint8_t PID = 0xF0; // No layer 3 protocol

    std::vector<uint8_t> out;
    out.reserve(m_callFrameTo.size() + m_callFrameFrom.size() + 2 + payload.size());

    out.insert(out.end(), m_callFrameTo.begin(), m_callFrameTo.end());
    out.insert(out.end(), m_callFrameFrom.begin(), m_callFrameFrom.end());
    out.push_back(CONTROL);
    out.push_back(PID);
    out.insert(out.end(), payload.begin(), payload.end());

    return out;
}

std::vector<uint8_t> AX25FrameBuilder::m_buildAX25Call(const std::string& callsign, int ssid, bool last)
{
    std::string callsignStr(callsign);
    std::transform(callsignStr.begin(), callsignStr.end(), callsignStr.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    if (callsignStr.size() > 6) {
        callsignStr.resize(6);
    } else if (callsignStr.size() < 6) {
        callsignStr.append(6 - callsignStr.size(), ' ');
    }

    std::vector<uint8_t> out;
    out.reserve(7);
    for (size_t i = 0; i < 6; ++i) {
        out.push_back(static_cast<uint8_t>((static_cast<uint8_t>(callsignStr[i]) << 1) & 0xFE));
    }

    uint8_t ssid_byte = static_cast<uint8_t>(0x60 | ((ssid & 0x0F) << 1));
    if (last) ssid_byte |= 0x01;
    out.push_back(ssid_byte);

    return out;
}