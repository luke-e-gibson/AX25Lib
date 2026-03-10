#include "AX25Decoder.hpp"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <vector>

#include <zlib.h>

#include "AX25Util.hpp"

AX25Decoder::AX25Decoder() {}

std::vector<uint8_t> AX25Decoder::m_extractFirstFrame(const std::vector<uint8_t>& ax25Frame)
{
    const auto firstFend = std::find(ax25Frame.begin(), ax25Frame.end(), static_cast<uint8_t>(0xC0));
    if (firstFend == ax25Frame.end())
    {
        return ax25Frame;
    }

    auto chunkStart = ax25Frame.begin();
    while (chunkStart != ax25Frame.end())
    {
        chunkStart = std::find_if(chunkStart, ax25Frame.end(), [](uint8_t byte) { return byte != 0xC0; });
        if (chunkStart == ax25Frame.end())
        {
            return {};
        }

        const auto chunkEnd = std::find(chunkStart, ax25Frame.end(), static_cast<uint8_t>(0xC0));
        if (chunkStart != chunkEnd)
        {
            return std::vector<uint8_t>(chunkStart, chunkEnd);
        }

        chunkStart = chunkEnd;
    }

    return {};
}

std::vector<uint8_t> AX25Decoder::m_maybeDecompressPayload(const std::vector<uint8_t>& payload)
{
    if (payload.empty())
    {
        return payload;
    }

    z_stream stream{};
    stream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(payload.data()));
    stream.avail_in = static_cast<uInt>(payload.size());
    if (inflateInit(&stream) != Z_OK)
    {
        return payload;
    }

    std::vector<uint8_t> output;
    std::array<uint8_t, 4096> buffer{};
    int status = Z_OK;
    do
    {
        stream.next_out = buffer.data();
        stream.avail_out = static_cast<uInt>(buffer.size());
        status = inflate(&stream, Z_NO_FLUSH);

        if (status != Z_OK && status != Z_STREAM_END)
        {
            inflateEnd(&stream);
            return payload;
        }

        const auto written = buffer.size() - stream.avail_out;
        output.insert(output.end(), buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(written));
    }
    while (status != Z_STREAM_END);

    inflateEnd(&stream);
    return output;
}

std::optional<AX25DecodedPacket> AX25Decoder::decodePacket(const std::vector<uint8_t>& ax25Frame)
{
    try {
        if (ax25Frame.empty()) return std::nullopt;

        std::vector<uint8_t> frame = m_extractFirstFrame(ax25Frame);

        // Remove optional KISS command byte (0x00)
        if (!frame.empty() && frame[0] == 0x00) frame.erase(frame.begin());

        // Unescape KISS
        frame = AX25Util::kissUnescape(frame);

        // Minimal AX.25 length: 2 * 7 addresses + CONTROL + PID = 16
        if (frame.size() < 16) return std::nullopt;

        AddressParseResult af;
        try {
            af = m_parseAddress(frame);
        } catch (...) {
            return std::nullopt;
        }
        if (af.addresses.size() < 2) return std::nullopt;

        auto dest_raw = af.addresses[0];
        auto src_raw  = af.addresses[1];

        if (frame.size() < af.consumed + 2) return std::nullopt;
        std::vector<uint8_t> payload(frame.begin() + af.consumed + 2, frame.end());
        auto decodedPayload = m_maybeDecompressPayload(payload);

        AX25DecodedPacket res;
        res.callsignTo = m_decodeCall(dest_raw);
        res.callsignFrom  = m_decodeCall(src_raw);
        res.payloadData = std::move(decodedPayload);
        res.textData = std::string(res.payloadData.begin(), res.payloadData.end());
        return res;
    } catch (...) {
        return std::nullopt;
    }
}

std::string AX25Decoder::m_decodeCall(const std::array<uint8_t, 7>& addr)
{
    std::string call;
    call.reserve(8);
    for (int i = 0; i < 6; ++i) {
        char c = static_cast<char>(addr[i] >> 1);
        if (c != ' ') call.push_back(c);
    }
    unsigned char ssid = (addr[6] >> 1) & 0x0F;
    call.push_back('-');
    call += std::to_string(ssid);
    return call;
}

AX25Decoder::AddressParseResult AX25Decoder::m_parseAddress(const std::vector<uint8_t>& ax25Frame)
{
    AddressParseResult result;
    result.consumed = 0;
    while (true) {
        if (result.consumed + 7 > ax25Frame.size()) {
            throw std::runtime_error("truncated AX.25 address field");
        }
        std::array<uint8_t, 7> addr;
        for (std::size_t i = 0; i < 7; ++i) addr[i] = ax25Frame[result.consumed + i];
        result.addresses.push_back(addr);
        result.consumed += 7;
        if (addr[6] & 0x01) break;
    }
    return result;
}


