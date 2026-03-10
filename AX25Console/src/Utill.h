#pragma once
#include <filesystem>
#include <vector>
#include <string>

struct KissConfig
{
    std::string host;
    int port;
};

struct ConsoleConfig
{
    std::string toCallsign;
    int toSSID;
    std::string fromCallsign;
    int fromSSID;
};

struct Config
{
    KissConfig kiss;
    ConsoleConfig ax25;
};

class Utill
{
public:
    static void printHex(const std::vector<uint8_t>& data);
    static Config loadConfigFile(const std::vector<std::filesystem::path>& candidates);
    
};
