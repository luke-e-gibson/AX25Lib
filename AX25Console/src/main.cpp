#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <inicpp.h>

#include "AX25Config.hpp"
#include "AX25FrameBuilder.hpp"
#include "AX25Decoder.hpp"

void printHex(const std::vector<uint8_t>& data) {
    for (uint8_t byte : data) {
        printf("%02X ", byte);
    }
    printf("\n");
}

struct ConsoleConfig
{
    std::string toCallsign;
    int toSSID;
    std::string fromCallsign;
    int fromSSID;
};

ConsoleConfig loadConfigFile(const std::string& filename)
{
    ConsoleConfig cfg{"NOCALL", 0, "NOCALL", 0};
    ini::IniFile configIn;
    try {
        configIn.load(filename.c_str());
        cfg.toCallsign = configIn["TO_RADIO"]["callSign"].as<std::string>();
        cfg.toSSID = configIn["TO_RADIO"]["SSID"].as<int>();
        cfg.fromCallsign = configIn["FROM_RADIO"]["callSign"].as<std::string>();
        cfg.fromSSID = configIn["FROM_RADIO"]["SSID"].as<int>();
    } catch (...) {
        std::cerr << "Warning: failed to load config.ini; using defaults" << std::endl;
    }
    return cfg;
}

int main()
{
    auto cfg = loadConfigFile("config.ini");

    AX25Config axCfg;
    axCfg.callsignTo = cfg.toCallsign;
    axCfg.ssidTo = cfg.toSSID;
    axCfg.callsignFrom = cfg.fromCallsign;
    axCfg.ssidFrom = cfg.fromSSID;

    AX25FrameBuilder builder(axCfg);
    AX25Decoder decoder;

    std::cout << "AX.25 console (no KISS/Direwolf). Type messages and press Enter." << std::endl;
    std::cout << "Loaded config: " << cfg.fromCallsign << "-" << cfg.fromSSID
              << " -> " << cfg.toCallsign << "-" << cfg.toSSID << std::endl;

    for (;;) {
        std::cout << ">>> " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        std::vector<uint8_t> payload(line.begin(), line.end());
        auto frame = builder.buildAx25Frame(payload);

        std::cout << "AX.25 Frame: ";
        printHex(frame);

        auto decoded = decoder.decodePacket(frame);
        if (decoded) {
            std::cout << "[DECODED] " << decoded->callsignFrom << " -> "
                      << decoded->callsignTo << " : " << decoded->textData << std::endl;
        } else {
            std::cout << "[DECODED] <invalid frame>" << std::endl;
        }
    }

    return 0;
}
