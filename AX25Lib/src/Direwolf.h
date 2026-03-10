#pragma once
#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "utill/fatal_error.hpp"

struct DirewolfConnectionInfo
{
    std::string host;
    int port;
};

struct DirewolfPacketInfo
{
    std::string toCallsign;
    int toSSID;
    std::string fromCallsign;
    int fromSSID;
};

struct DirewolfConfig
{
    DirewolfConnectionInfo connectionInfo;
    DirewolfPacketInfo packetInfo;
};

namespace direwolf_detail
{
template<typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

struct AnyInit
{
    template<typename T>
    constexpr operator T() const noexcept;
};

template<typename T, std::size_t... Indices>
auto testAggregateInitialization(std::index_sequence<Indices...>)
    -> decltype(T{(static_cast<void>(Indices), AnyInit{})...}, std::true_type{});

template<typename T, std::size_t FieldCount, typename = void>
struct is_brace_constructible_with_n_fields : std::false_type
{
};

template<typename T, std::size_t FieldCount>
struct is_brace_constructible_with_n_fields<T, FieldCount,
    std::void_t<decltype(testAggregateInitialization<T>(std::make_index_sequence<FieldCount>{}))>> : std::true_type
{
};

template<typename T, std::size_t Current = 0, std::size_t MaxFields = 16>
constexpr std::size_t aggregate_field_count()
{
    if constexpr (!std::is_aggregate_v<T>)
    {
        return 0;
    }
    else if constexpr (Current == MaxFields)
    {
        return MaxFields;
    }
    else if constexpr (is_brace_constructible_with_n_fields<T, Current + 1>::value)
    {
        return aggregate_field_count<T, Current + 1, MaxFields>();
    }
    else
    {
        return Current;
    }
}

template<typename T>
inline constexpr bool is_string_v = std::is_same_v<remove_cvref_t<T>, std::string>;

template<typename T>
inline constexpr bool is_c_string_v = std::is_same_v<remove_cvref_t<T>, char*> || std::is_same_v<remove_cvref_t<T>, const char*>;

template<typename T>
inline constexpr bool is_byte_vector_v = std::is_same_v<remove_cvref_t<T>, std::vector<std::uint8_t>>;

template<typename T>
inline constexpr bool is_char_array_v = std::is_array_v<remove_cvref_t<T>> &&
    std::is_same_v<std::remove_cv_t<std::remove_extent_t<remove_cvref_t<T>>>, char>;

class ByteWriter
{
public:
    void write(const void* data, std::size_t size)
    {
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        m_buffer.insert(m_buffer.end(), bytes, bytes + size);
    }

    void writeUInt32(std::uint32_t value)
    {
        const std::array<std::uint8_t, 4> bytes = {
            static_cast<std::uint8_t>(value & 0xFFu),
            static_cast<std::uint8_t>((value >> 8) & 0xFFu),
            static_cast<std::uint8_t>((value >> 16) & 0xFFu),
            static_cast<std::uint8_t>((value >> 24) & 0xFFu)
        };
        write(bytes.data(), bytes.size());
    }

    [[nodiscard]] std::vector<std::uint8_t> take()
    {
        return std::move(m_buffer);
    }

private:
    std::vector<std::uint8_t> m_buffer;
};

class ByteReader
{
public:
    explicit ByteReader(const std::vector<std::uint8_t>& buffer) : m_buffer(buffer)
    {
    }

    void read(void* destination, std::size_t size)
    {
        if (m_offset + size > m_buffer.size())
        {
            std::ostringstream message;
            message << "Failed to deserialize packet payload: needed " << size
                << " more byte(s) at offset " << m_offset
                << ", but the payload only contains " << m_buffer.size() << " byte(s).";
            direwolf_fatal::fail("Packet payload is truncated", message);
        }

        std::memcpy(destination, m_buffer.data() + m_offset, size);
        m_offset += size;
    }

    [[nodiscard]] std::uint32_t readUInt32()
    {
        std::array<std::uint8_t, 4> bytes{};
        read(bytes.data(), bytes.size());
        return static_cast<std::uint32_t>(bytes[0]) |
            (static_cast<std::uint32_t>(bytes[1]) << 8u) |
            (static_cast<std::uint32_t>(bytes[2]) << 16u) |
            (static_cast<std::uint32_t>(bytes[3]) << 24u);
    }

    [[nodiscard]] std::string readString()
    {
        const auto length = static_cast<std::size_t>(readUInt32());
        if (m_offset + length > m_buffer.size())
        {
            std::ostringstream message;
            message << "Failed to deserialize a string field: expected " << length
                << " byte(s) starting at offset " << m_offset
                << ", but the payload only contains " << m_buffer.size() << " byte(s).";
            direwolf_fatal::fail("Packet string field is truncated", message);
        }

        std::string value(reinterpret_cast<const char*>(m_buffer.data() + m_offset), length);
        m_offset += length;
        return value;
    }

    [[nodiscard]] bool isFullyConsumed() const
    {
        return m_offset == m_buffer.size();
    }

private:
    const std::vector<std::uint8_t>& m_buffer;
    std::size_t m_offset = 0;
};

struct DeserializeState
{
    std::deque<std::string> stringStorage;
};

template<typename T, typename F>
void for_each_field(T&& value, F&& callback)
{
    using ValueType = remove_cvref_t<T>;
    constexpr std::size_t fieldCount = aggregate_field_count<ValueType>();

    static_assert(fieldCount < 16, "Packet aggregates with more than 15 fields are not supported");

    if constexpr (fieldCount == 0)
    {
        (void)value;
        (void)callback;
    }
    else if constexpr (fieldCount == 1)
    {
        auto&& [f0] = value;
        callback(f0);
    }
    else if constexpr (fieldCount == 2)
    {
        auto&& [f0, f1] = value;
        callback(f0);
        callback(f1);
    }
    else if constexpr (fieldCount == 3)
    {
        auto&& [f0, f1, f2] = value;
        callback(f0);
        callback(f1);
        callback(f2);
    }
    else if constexpr (fieldCount == 4)
    {
        auto&& [f0, f1, f2, f3] = value;
        callback(f0);
        callback(f1);
        callback(f2);
        callback(f3);
    }
    else if constexpr (fieldCount == 5)
    {
        auto&& [f0, f1, f2, f3, f4] = value;
        callback(f0);
        callback(f1);
        callback(f2);
        callback(f3);
        callback(f4);
    }
    else if constexpr (fieldCount == 6)
    {
        auto&& [f0, f1, f2, f3, f4, f5] = value;
        callback(f0);
        callback(f1);
        callback(f2);
        callback(f3);
        callback(f4);
        callback(f5);
    }
    else if constexpr (fieldCount == 7)
    {
        auto&& [f0, f1, f2, f3, f4, f5, f6] = value;
        callback(f0);
        callback(f1);
        callback(f2);
        callback(f3);
        callback(f4);
        callback(f5);
        callback(f6);
    }
    else if constexpr (fieldCount == 8)
    {
        auto&& [f0, f1, f2, f3, f4, f5, f6, f7] = value;
        callback(f0);
        callback(f1);
        callback(f2);
        callback(f3);
        callback(f4);
        callback(f5);
        callback(f6);
        callback(f7);
    }
    else if constexpr (fieldCount == 9)
    {
        auto&& [f0, f1, f2, f3, f4, f5, f6, f7, f8] = value;
        callback(f0);
        callback(f1);
        callback(f2);
        callback(f3);
        callback(f4);
        callback(f5);
        callback(f6);
        callback(f7);
        callback(f8);
    }
    else if constexpr (fieldCount == 10)
    {
        auto&& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9] = value;
        callback(f0);
        callback(f1);
        callback(f2);
        callback(f3);
        callback(f4);
        callback(f5);
        callback(f6);
        callback(f7);
        callback(f8);
        callback(f9);
    }
    else if constexpr (fieldCount == 11)
    {
        auto&& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10] = value;
        callback(f0);
        callback(f1);
        callback(f2);
        callback(f3);
        callback(f4);
        callback(f5);
        callback(f6);
        callback(f7);
        callback(f8);
        callback(f9);
        callback(f10);
    }
    else if constexpr (fieldCount == 12)
    {
        auto&& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11] = value;
        callback(f0);
        callback(f1);
        callback(f2);
        callback(f3);
        callback(f4);
        callback(f5);
        callback(f6);
        callback(f7);
        callback(f8);
        callback(f9);
        callback(f10);
        callback(f11);
    }
    else if constexpr (fieldCount == 13)
    {
        auto&& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12] = value;
        callback(f0);
        callback(f1);
        callback(f2);
        callback(f3);
        callback(f4);
        callback(f5);
        callback(f6);
        callback(f7);
        callback(f8);
        callback(f9);
        callback(f10);
        callback(f11);
        callback(f12);
    }
    else if constexpr (fieldCount == 14)
    {
        auto&& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13] = value;
        callback(f0);
        callback(f1);
        callback(f2);
        callback(f3);
        callback(f4);
        callback(f5);
        callback(f6);
        callback(f7);
        callback(f8);
        callback(f9);
        callback(f10);
        callback(f11);
        callback(f12);
        callback(f13);
    }
    else if constexpr (fieldCount == 15)
    {
        auto&& [f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14] = value;
        callback(f0);
        callback(f1);
        callback(f2);
        callback(f3);
        callback(f4);
        callback(f5);
        callback(f6);
        callback(f7);
        callback(f8);
        callback(f9);
        callback(f10);
        callback(f11);
        callback(f12);
        callback(f13);
        callback(f14);
    }
}

template<typename T>
void serialize(ByteWriter& writer, const T& value);

template<typename T>
void deserialize(ByteReader& reader, T& value, DeserializeState& state);

template<typename T>
void serializeStringLike(ByteWriter& writer, const T& value)
{
    const char* data = value == nullptr ? "" : value;
    const std::uint32_t length = static_cast<std::uint32_t>(std::strlen(data));
    writer.writeUInt32(length);
    writer.write(data, length);
}

inline void serialize(ByteWriter& writer, const std::string& value)
{
    writer.writeUInt32(static_cast<std::uint32_t>(value.size()));
    writer.write(value.data(), value.size());
}

template<typename T>
void serialize(ByteWriter& writer, const T& value)
{
    using ValueType = remove_cvref_t<T>;

    if constexpr (std::is_enum_v<ValueType>)
    {
        using UnderlyingType = std::underlying_type_t<ValueType>;
        const auto underlying = static_cast<UnderlyingType>(value);
        serialize(writer, underlying);
    }
    else if constexpr (std::is_arithmetic_v<ValueType>)
    {
        writer.write(&value, sizeof(value));
    }
    else if constexpr (is_string_v<ValueType>)
    {
        serialize(writer, static_cast<const std::string&>(value));
    }
    else if constexpr (is_c_string_v<ValueType>)
    {
        serializeStringLike(writer, value);
    }
    else if constexpr (is_char_array_v<ValueType>)
    {
        const std::string text(value);
        serialize(writer, text);
    }
    else if constexpr (is_byte_vector_v<ValueType>)
    {
        writer.writeUInt32(static_cast<std::uint32_t>(value.size()));
        if (!value.empty())
        {
            writer.write(value.data(), value.size());
        }
    }
    else if constexpr (std::is_array_v<ValueType>)
    {
        for (const auto& element : value)
        {
            serialize(writer, element);
        }
    }
    else if constexpr (std::is_aggregate_v<ValueType>)
    {
        for_each_field(value, [&](const auto& field)
        {
            serialize(writer, field);
        });
    }
    else
    {
        static_assert(!sizeof(ValueType), "Packet type is not serializable by Direwolf");
    }
}

inline void deserialize(ByteReader& reader, std::string& value, DeserializeState&)
{
    value = reader.readString();
}

template<typename T>
void deserialize(ByteReader& reader, T& value, DeserializeState& state)
{
    using ValueType = remove_cvref_t<T>;

    if constexpr (std::is_enum_v<ValueType>)
    {
        using UnderlyingType = std::underlying_type_t<ValueType>;
        UnderlyingType underlying{};
        deserialize(reader, underlying, state);
        value = static_cast<ValueType>(underlying);
    }
    else if constexpr (std::is_arithmetic_v<ValueType>)
    {
        reader.read(&value, sizeof(value));
    }
    else if constexpr (is_string_v<ValueType>)
    {
        deserialize(reader, static_cast<std::string&>(value), state);
    }
    else if constexpr (std::is_same_v<ValueType, const char*>)
    {
        state.stringStorage.push_back(reader.readString());
        value = state.stringStorage.back().c_str();
    }
    else if constexpr (std::is_same_v<ValueType, char*>)
    {
        state.stringStorage.push_back(reader.readString());
        value = state.stringStorage.back().data();
    }
    else if constexpr (is_char_array_v<ValueType>)
    {
        const auto text = reader.readString();
        std::memset(value, 0, sizeof(value));
        std::memcpy(value, text.c_str(), std::min<std::size_t>(text.size(), sizeof(value) - 1));
    }
    else if constexpr (is_byte_vector_v<ValueType>)
    {
        const auto length = static_cast<std::size_t>(reader.readUInt32());
        value.resize(length);
        if (length != 0)
        {
            reader.read(value.data(), value.size());
        }
    }
    else if constexpr (std::is_array_v<ValueType>)
    {
        for (auto& element : value)
        {
            deserialize(reader, element, state);
        }
    }
    else if constexpr (std::is_aggregate_v<ValueType>)
    {
        for_each_field(value, [&](auto& field)
        {
            deserialize(reader, field, state);
        });
    }
    else
    {
        static_assert(!sizeof(ValueType), "Packet type is not deserializable by Direwolf");
    }
}

template<typename T>
std::vector<std::uint8_t> serializePacketPayload(int packetType, const T& packet)
{
    ByteWriter writer;
    serialize(writer, static_cast<std::int32_t>(packetType));
    serialize(writer, packet);
    return writer.take();
}

template<typename T>
struct DecodedPacket
{
    T packet{};
    DeserializeState state;
};

template<typename T>
DecodedPacket<T> deserializePacketPayload(const std::vector<std::uint8_t>& payload)
{
    DecodedPacket<T> decoded;
    ByteReader payloadReader(payload);
    std::int32_t packetType = 0;
    deserialize(payloadReader, packetType, decoded.state);
    deserialize(payloadReader, decoded.packet, decoded.state);
    if (!payloadReader.isFullyConsumed())
    {
        std::ostringstream message;
        message << "Packet type " << packetType << " decoded successfully, but "
            << "there are unexpected trailing bytes in the payload. "
            << "This usually means the sender and receiver packet layouts do not match.";
        direwolf_fatal::fail("Packet payload contains trailing bytes", message);
    }
    return decoded;
}

inline int readPacketType(const std::vector<std::uint8_t>& payload)
{
    ByteReader reader(payload);
    std::int32_t packetType = 0;
    DeserializeState state;
    deserialize(reader, packetType, state);
    return static_cast<int>(packetType);
}
}

class Direwolf
{
public:
    Direwolf();
    explicit Direwolf(DirewolfConfig config);
    ~Direwolf();

    Direwolf(const Direwolf&) = delete;
    Direwolf& operator=(const Direwolf&) = delete;
    Direwolf(Direwolf&&) = delete;
    Direwolf& operator=(Direwolf&&) = delete;
    
    template<typename T>
    void onPacketReceived(int packetType, std::function<void(T packet)> callback)
    {
        registerPacketHandler(packetType, [callback = std::move(callback)](const std::vector<std::uint8_t>& payload) mutable
        {
            auto decoded = direwolf_detail::deserializePacketPayload<T>(payload);
            callback(decoded.packet);
        });
    }

    template<typename T>
    void sendPacket(int packetType, const T& packet, bool compress = false)
    {
        sendSerializedPacket(direwolf_detail::serializePacketPayload(packetType, packet), compress);
    }
    
    void listen();
    
private:
    struct Impl;
    using PacketHandler = std::function<void(const std::vector<std::uint8_t>& payload)>;

    void ensureConnected();
    void registerPacketHandler(int packetType, PacketHandler callback);
    void sendSerializedPacket(const std::vector<std::uint8_t>& payload, bool compress);
    void receiverLoop();
    void dispatchPayload(const std::vector<std::uint8_t>& payload);
    static std::vector<std::vector<std::uint8_t>> extractFrames(std::vector<std::uint8_t>& buffer);

    DirewolfConfig config;
    std::unique_ptr<Impl> m_impl;
};
