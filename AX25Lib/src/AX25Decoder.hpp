#pragma once
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <cstdint>

struct AX25DecodedPacket
{
    std::string callsignTo;   // To send to
    std::string callsignFrom; // From us
    std::string textData;
};

class AX25Decoder
{
public:
    AX25Decoder();
    AX25Decoder(const AX25Decoder&) = default;
    ~AX25Decoder() = default;
    
    std::optional<AX25DecodedPacket> decodePacket(const std::vector<uint8_t>& ax25Frame);
private:
    struct AddressParseResult
    {
        std::vector<std::array<uint8_t, 7>> addresses;
        std::size_t consumed;
    };

    static AddressParseResult m_parseAddress(const std::vector<uint8_t>& ax25Frame);
    static std::string m_decodeCall(const std::array<uint8_t, 7>& call);
    
};
