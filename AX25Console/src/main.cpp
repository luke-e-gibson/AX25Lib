#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <inicpp.h>

#include "socket.hpp"
#include "AX25Config.hpp"
#include "AX25FrameBuilder.hpp"
#include "AX25Decoder.hpp"
#include "Utill.h"

int main(int argc, char** argv)
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout << std::unitbuf; // ensure prompts flush before blocking for input

    std::filesystem::path execDir;
    if (argc > 0)
    {
        try
        {
            execDir = std::filesystem::canonical(std::filesystem::path(argv[0])).parent_path();
        }
        catch (...)
        {
            execDir.clear();
        }
    }

    std::vector<std::filesystem::path> candidates = {
        std::filesystem::current_path() / "config.ini",
        std::filesystem::current_path() / "AX25Console/config.ini"
    };
    if (!execDir.empty())
    {
        candidates.push_back(execDir / "config.ini");
    }

    auto cfg = Utill::loadConfigFile(candidates);
    
    socket_init();
    socket_t kissSocket = connect_kiss(cfg.kiss.host, cfg.kiss.port);
    
    AX25Config axCfg;
    axCfg.callsignTo = cfg.ax25.toCallsign;
    axCfg.ssidTo = cfg.ax25.toSSID;
    axCfg.callsignFrom = cfg.ax25.fromCallsign;
    axCfg.ssidFrom = cfg.ax25.fromSSID;

    AX25FrameBuilder builder(axCfg);
    AX25Decoder decoder;

    std::cout << "AX.25 console (Direwolf). Type messages and press Enter." << std::endl;
    std::cout << "Loaded config: " << cfg.ax25.fromCallsign << "-" << cfg.ax25.fromSSID
        << " -> " << cfg.ax25.toCallsign << "-" << cfg.ax25.toSSID
        << "; Kiss Host: " << cfg.kiss.host << ":" << cfg.kiss.port << std::endl;
    
    
    for (;;)
    {
        std::cout << ">>> " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        std::vector<uint8_t> payload(line.begin(), line.end());

        //Build AX.25 frame, then wrap in KISS frame
        auto frame = builder.buildAx25Frame(payload);
        frame = builder.buildKissFrame(frame);
        //send to KISS TNC
        size_t sent = send(kissSocket, reinterpret_cast<const char*>(frame.data()), frame.size(), 0);
        
        std::cout << "AX.25 KISS Frame: ";
        Utill::printHex(frame);

        auto decoded = decoder.decodePacket(frame);
        if (decoded)
        {
            std::cout << "[DECODED] " << decoded->callsignFrom << " -> "
                << decoded->callsignTo << " : " << decoded->textData << std::endl;
        }
        else
        {
            std::cout << "[DECODED] <invalid frame>" << std::endl;
        }
    }
    
    socket_cleanup();
    return 0;
}
