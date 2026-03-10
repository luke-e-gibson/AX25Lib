#include "Direwolf.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <mutex>
#include <sstream>
#include <thread>

#include <zlib.h>

#include "protocall/AX25Config.hpp"
#include "protocall/AX25Decoder.hpp"
#include "protocall/AX25FrameBuilder.hpp"
#include "utill/socket.hpp"

namespace
{
AX25Config makeAx25Config(const DirewolfConfig& config)
{
    AX25Config ax25Config{};
    ax25Config.callsignTo = config.packetInfo.toCallsign;
    ax25Config.ssidTo = config.packetInfo.toSSID;
    ax25Config.callsignFrom = config.packetInfo.fromCallsign;
    ax25Config.ssidFrom = config.packetInfo.fromSSID;
    return ax25Config;
}

void closeSocketIfOpen(socket_t& socketHandle)
{
    if (socketHandle != INVALID_SOCK)
    {
        socket_close(socketHandle);
        socketHandle = INVALID_SOCK;
    }
}

void sendAll(socket_t socketHandle, const std::vector<std::uint8_t>& buffer)
{
    std::size_t totalSent = 0;
    while (totalSent < buffer.size())
    {
        const auto remaining = static_cast<int>(buffer.size() - totalSent);
        const auto sent = send(socketHandle, reinterpret_cast<const char*>(buffer.data() + totalSent), remaining, 0);
        if (sent <= 0)
        {
            std::ostringstream message;
            message << "TCP send failed while writing " << buffer.size()
                << " byte(s) to the Direwolf KISS socket after " << totalSent << " byte(s) were sent.";
            direwolf_fatal::fail("Failed to send packet to Direwolf", message);
        }

        totalSent += static_cast<std::size_t>(sent);
    }
}

std::vector<std::uint8_t> compressPayloadIfRequested(const std::vector<std::uint8_t>& payload, bool compress)
{
    if (!compress || payload.empty())
    {
        return payload;
    }

    uLongf compressedSize = compressBound(static_cast<uLong>(payload.size()));
    std::vector<std::uint8_t> compressed(compressedSize);
    const auto status = compress2(
        compressed.data(),
        &compressedSize,
        reinterpret_cast<const Bytef*>(payload.data()),
        static_cast<uLong>(payload.size()),
        Z_BEST_COMPRESSION);
    if (status != Z_OK)
    {
        return payload;
    }

    compressed.resize(static_cast<std::size_t>(compressedSize));
    if (compressed.size() >= payload.size())
    {
        return payload;
    }

    return compressed;
}
}

struct Direwolf::Impl
{
    explicit Impl(const DirewolfConfig& direwolfConfig)
        : frameBuilder(makeAx25Config(direwolfConfig))
    {
    }

    std::mutex socketMutex;
    std::mutex handlerMutex;
    socket_t socketHandle = INVALID_SOCK;
    bool winsockStarted = false;
    std::atomic<bool> stopRequested = false;
    std::atomic<bool> receiverRunning = false;
    std::thread receiverThread;
    AX25FrameBuilder frameBuilder;
    AX25Decoder decoder;
    std::unordered_map<int, std::vector<Direwolf::PacketHandler>> packetHandlers;
};

Direwolf::Direwolf()
    : Direwolf(DirewolfConfig{
        DirewolfConnectionInfo{"127.0.0.1", 8001},
        DirewolfPacketInfo{"NOCALL", 0, "NOCALL", 0}
    })
{
}

Direwolf::Direwolf(DirewolfConfig config)
    : config(std::move(config)),
      m_impl(std::make_unique<Impl>(this->config))
{
}

Direwolf::~Direwolf()
{
    if (!m_impl)
    {
        return;
    }

    m_impl->stopRequested = true;
    {
        std::lock_guard<std::mutex> lock(m_impl->socketMutex);
        closeSocketIfOpen(m_impl->socketHandle);
    }

    if (m_impl->receiverThread.joinable())
    {
        m_impl->receiverThread.join();
    }

    if (m_impl->winsockStarted)
    {
        socket_cleanup();
    }
}

void Direwolf::ensureConnected()
{
    std::lock_guard<std::mutex> lock(m_impl->socketMutex);
    if (m_impl->socketHandle != INVALID_SOCK)
    {
        return;
    }

    if (!m_impl->winsockStarted)
    {
        socket_init();
        m_impl->winsockStarted = true;
    }

    m_impl->socketHandle = connect_kiss(config.connectionInfo.host, config.connectionInfo.port);
}

void Direwolf::registerPacketHandler(int packetType, PacketHandler callback)
{
    std::lock_guard<std::mutex> lock(m_impl->handlerMutex);
    m_impl->packetHandlers[packetType].push_back(std::move(callback));
}

void Direwolf::sendSerializedPacket(const std::vector<std::uint8_t>& payload, bool compress)
{
    ensureConnected();

    auto ax25Frame = m_impl->frameBuilder.buildAx25Frame(compressPayloadIfRequested(payload, compress));
    auto kissFrame = m_impl->frameBuilder.buildKissFrame(ax25Frame);

    socket_t socketHandle = INVALID_SOCK;
    {
        std::lock_guard<std::mutex> lock(m_impl->socketMutex);
        socketHandle = m_impl->socketHandle;
    }

    if (socketHandle == INVALID_SOCK)
    {
        direwolf_fatal::fail(
            "Direwolf socket is not connected",
            "The library tried to send a packet before a TCP connection to Direwolf was available. "
            "Verify the Direwolf host/port and call listen() after Direwolf is running."
        );
    }

    sendAll(socketHandle, kissFrame);
}

void Direwolf::listen()
{
    ensureConnected();

    if (m_impl->receiverThread.joinable())
    {
        if (m_impl->receiverRunning)
        {
            return;
        }

        m_impl->receiverThread.join();
    }

    m_impl->stopRequested = false;
    m_impl->receiverThread = std::thread(&Direwolf::receiverLoop, this);
}

void Direwolf::receiverLoop()
{
    m_impl->receiverRunning = true;
    std::vector<std::uint8_t> receiveBuffer;

    while (!m_impl->stopRequested)
    {
        std::array<std::uint8_t, 1024> chunk{};
        socket_t socketHandle = INVALID_SOCK;
        {
            std::lock_guard<std::mutex> lock(m_impl->socketMutex);
            socketHandle = m_impl->socketHandle;
        }

        if (socketHandle == INVALID_SOCK)
        {
            break;
        }

        const int received = recv(socketHandle, reinterpret_cast<char*>(chunk.data()), static_cast<int>(chunk.size()), 0);
        if (received <= 0)
        {
            std::ostringstream message;
            message << "The TCP connection to Direwolf at " << config.connectionInfo.host
                << ':' << config.connectionInfo.port
                << " was closed or returned an error while waiting for incoming KISS frames.";
            direwolf_fatal::fail("Lost connection to Direwolf", message);
        }

        receiveBuffer.insert(receiveBuffer.end(), chunk.begin(), chunk.begin() + received);
        auto frames = extractFrames(receiveBuffer);
        for (const auto& frame : frames)
        {
            auto decoded = m_impl->decoder.decodePacket(frame);
            if (!decoded)
            {
                continue;
            }

            const std::vector<std::uint8_t> payload(decoded->textData.begin(), decoded->textData.end());
            dispatchPayload(payload);
        }
    }

    m_impl->receiverRunning = false;
}

void Direwolf::dispatchPayload(const std::vector<std::uint8_t>& payload)
{
    const int packetType = direwolf_detail::readPacketType(payload);

    std::vector<PacketHandler> handlers;
    {
        std::lock_guard<std::mutex> lock(m_impl->handlerMutex);
        const auto it = m_impl->packetHandlers.find(packetType);
        if (it == m_impl->packetHandlers.end())
        {
            return;
        }

        handlers = it->second;
    }

    for (auto& handler : handlers)
    {
        handler(payload);
    }
}

std::vector<std::vector<std::uint8_t>> Direwolf::extractFrames(std::vector<std::uint8_t>& buffer)
{
    std::vector<std::vector<std::uint8_t>> frames;

    auto firstDelimiter = std::find(buffer.begin(), buffer.end(), static_cast<std::uint8_t>(0xC0));
    if (firstDelimiter != buffer.begin())
    {
        buffer.erase(buffer.begin(), firstDelimiter);
    }

    while (buffer.size() >= 2)
    {
        if (buffer.front() != 0xC0)
        {
            auto delimiter = std::find(buffer.begin(), buffer.end(), static_cast<std::uint8_t>(0xC0));
            buffer.erase(buffer.begin(), delimiter);
            continue;
        }

        const auto end = std::find(buffer.begin() + 1, buffer.end(), static_cast<std::uint8_t>(0xC0));
        if (end == buffer.end())
        {
            break;
        }

        if (end != buffer.begin() + 1)
        {
            frames.emplace_back(buffer.begin(), end + 1);
        }

        buffer.erase(buffer.begin(), end + 1);
    }

    return frames;
}
