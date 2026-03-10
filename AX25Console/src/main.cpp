#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <inicpp.h>

#include "Direwolf.h"
#include "Utill.h"

enum PacketTypes
{
    e_MessagePacket
};

struct MessagePacket
{
    const char* name;
    const char* message;
    
};

int main(int argc, char** argv)
{
    DirewolfConfig config = {};
    config.connectionInfo.host = "127.0.0.1";
    config.connectionInfo.port = 8001;
    
    config.packetInfo.fromCallsign = "NOCALL";
    config.packetInfo.fromSSID = 0;
    config.packetInfo.toCallsign = "NOCALL";
    config.packetInfo.toSSID = 0;
    
    Direwolf* direwolf = new Direwolf(config);

    direwolf->onPacketReceived<MessagePacket>(PacketTypes::e_MessagePacket, [] (MessagePacket packet) {
        std::cout << "Received message packet: " << packet.name << " says " << packet.message << "\n";
    });
    
    direwolf->listen(); // Starts listening for packets (blocking call)

    // Get user from console
    const std::string userName = []() {
        std::string name;
        std::cout << "Enter your name: ";
        std::getline(std::cin, name);
        return name;
    }();
    
    while (true)
    {
        const std::string message = []() {
            std::string msg;
            std::cout << "Enter a message to send: ";
            std::getline(std::cin, msg);
            return msg;
        }();
        
        MessagePacket packet;
        packet.name = userName.c_str();
        packet.message = message.c_str();
        
        direwolf->sendPacket(PacketTypes::e_MessagePacket, packet, true);
    }
    
}
