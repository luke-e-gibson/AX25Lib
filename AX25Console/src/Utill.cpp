#include "Utill.h"
#include <iostream>
#include <inicpp.h>


void Utill::printHex(const std::vector<uint8_t>& data)
{
    for (uint8_t byte : data)
    {
        std::cout << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(byte) << ' ';
    }
    std::cout << std::dec << std::endl;
}

Config Utill::loadConfigFile(const std::vector<std::filesystem::path>& candidates)
{
    ConsoleConfig consoleCfg{"NOCALL", 0, "NOCALL", 0};
    KissConfig kissConfig{"127.0.0.1", 8001};
    Config cfg = { kissConfig, consoleCfg };
    for (const auto& path : candidates)
    {
        try
        {
            if (!std::filesystem::exists(path)) continue;
            ini::IniFile configIn;
            configIn.load(path.string().c_str());
            consoleCfg.toCallsign = configIn["TO_RADIO"]["callSign"].as<std::string>();
            consoleCfg.toSSID = configIn["TO_RADIO"]["SSID"].as<int>();
            consoleCfg.fromCallsign = configIn["FROM_RADIO"]["callSign"].as<std::string>();
            consoleCfg.fromSSID = configIn["FROM_RADIO"]["SSID"].as<int>();
            kissConfig.host = configIn["KISS"]["kiss_host"].as<std::string>();
            kissConfig.port = configIn["KISS"]["kiss_port"].as<int>();
            cfg.kiss = kissConfig;
            cfg.ax25 = consoleCfg;
        }
        catch (...)
        {
            continue; // Try next candidate
        }
    }
    std::cerr << "Warning: failed to load config.ini; using defaults" << std::endl;
    return cfg;
}