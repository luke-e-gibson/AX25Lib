#include <chrono>
#include <cstdio>
#include <filesystem>
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

ConsoleConfig loadConfigFile(const std::vector<std::filesystem::path>& candidates)
{
    ConsoleConfig cfg{"NOCALL", 0, "NOCALL", 0};
    for (const auto& path : candidates) {
        try {
            if (!std::filesystem::exists(path)) continue;
            ini::IniFile configIn;
            configIn.load(path.string().c_str());
            cfg.toCallsign = configIn["TO_RADIO"]["callSign"].as<std::string>();
            cfg.toSSID = configIn["TO_RADIO"]["SSID"].as<int>();
            cfg.fromCallsign = configIn["FROM_RADIO"]["callSign"].as<std::string>();
            cfg.fromSSID = configIn["FROM_RADIO"]["SSID"].as<int>();
            return cfg; // Loaded successfully
        } catch (...) {
            continue; // Try next candidate
        }
    }
    std::cerr << "Warning: failed to load config.ini; using defaults" << std::endl;
    return cfg;
}

int main(int argc, char** argv)
{
    std::filesystem::path execDir;
    if (argc > 0) {
        try {
            execDir = std::filesystem::canonical(std::filesystem::path(argv[0])).parent_path();
        } catch (...) {
            execDir.clear();
        }
    }

    std::vector<std::filesystem::path> candidates = {
        std::filesystem::current_path() / "config.ini",
        std::filesystem::current_path() / "AX25Console/config.ini"
    };
    if (!execDir.empty()) {
        candidates.push_back(execDir / "config.ini");
    }

    auto cfg = loadConfigFile(candidates);

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
