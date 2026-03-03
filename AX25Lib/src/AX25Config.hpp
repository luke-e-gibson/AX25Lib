#pragma once
#include <string>

struct AX25Config
{
    std::string callsignTo;   // To send to
    std::string callsignFrom; // From us
    int ssidTo;               // SSID for destination
    int ssidFrom;             // SSID for source
};
