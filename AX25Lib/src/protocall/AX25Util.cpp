#include "AX25Util.hpp"

std::vector<uint8_t> AX25Util::kissEscape(const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> out;
    out.reserve(data.size() * 2); // worst-case
    for (uint8_t b : data) {
        if (b == 0xDB) { out.push_back(0xDB); out.push_back(0xDD); }
        else if (b == 0xC0) { out.push_back(0xDB); out.push_back(0xDC); }
        else { out.push_back(b); }
    }
    return out;
}

std::vector<uint8_t> AX25Util::kissUnescape(const std::vector<uint8_t>& in)
{
    std::vector<uint8_t> out;
    out.reserve(in.size());

    std::size_t index = 0;
    while (index < in.size())
    {
        const uint8_t byte = in[index];
        if (byte != 0xDB)
        {
            out.push_back(byte);
            ++index;
            continue;
        }

        if (index + 1 >= in.size())
        {
            out.push_back(0xDB);
            ++index;
            continue;
        }

        const uint8_t nextByte = in[index + 1];
        if (nextByte == 0xDC)
        {
            out.push_back(0xC0);
            index += 2;
        }
        else if (nextByte == 0xDD)
        {
            out.push_back(0xDB);
            index += 2;
        }
        else
        {
            out.push_back(0xDB);
            ++index;
        }
    }

    return out;
}