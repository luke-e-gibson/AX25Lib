#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "protocall/AX25Decoder.hpp"
#include "protocall/AX25FrameBuilder.hpp"
#include "protocall/AX25Config.hpp"
#include "Direwolf.h"

namespace py = pybind11;

namespace
{
    std::vector<uint8_t> build_ax25_call(const std::string& callsign, int ssid, bool last)
    {
        std::string callsignStr(callsign);
        std::transform(callsignStr.begin(), callsignStr.end(), callsignStr.begin(),
            [](unsigned char value) { return static_cast<char>(std::toupper(value)); });

        if (callsignStr.size() > 6)
        {
            callsignStr.resize(6);
        }
        else if (callsignStr.size() < 6)
        {
            callsignStr.append(6 - callsignStr.size(), ' ');
        }

        std::vector<uint8_t> encoded;
        encoded.reserve(7);
        for (char value : callsignStr)
        {
            encoded.push_back(static_cast<uint8_t>((static_cast<uint8_t>(value) << 1) & 0xFE));
        }

        uint8_t ssidByte = static_cast<uint8_t>(0x60 | ((ssid & 0x0F) << 1));
        if (last)
        {
            ssidByte |= 0x01;
        }

        encoded.push_back(ssidByte);
        return encoded;
    }

    std::vector<uint8_t> bytes_to_vec(const py::bytes& b)
    {
        std::string s = b; // pybind11 provides conversion to std::string
        return std::vector<uint8_t>(s.begin(), s.end());
    }

    std::vector<uint8_t> bytes_like_to_vec(const py::object& obj)
    {
        if (py::isinstance<py::bytes>(obj))
        {
            return bytes_to_vec(obj.cast<py::bytes>());
        }

        if (py::isinstance<py::bytearray>(obj))
        {
            py::bytes asBytes(obj);
            return bytes_to_vec(asBytes);
        }

        throw py::type_error("expected bytes or bytearray");
    }

    py::bytes vec_to_bytes(const std::vector<uint8_t>& v)
    {
        return py::bytes(reinterpret_cast<const char*>(v.data()), v.size());
    }

    py::str decode_payload_text(const std::vector<uint8_t>& payload)
    {
        PyObject* decoded = PyUnicode_DecodeUTF8(
            reinterpret_cast<const char*>(payload.data()),
            static_cast<Py_ssize_t>(payload.size()),
            "replace");
        if (decoded == nullptr)
        {
            throw py::error_already_set();
        }

        return py::reinterpret_steal<py::str>(decoded);
    }

    py::object decode_packet_object(AX25Decoder& decoder, const py::bytes& frameBytes)
    {
        auto decoded = decoder.decodePacket(bytes_to_vec(frameBytes));
        if (!decoded)
        {
            return py::none();
        }

        return py::cast(*decoded);
    }

    py::object decode_packet_tuple(AX25Decoder& decoder, const py::bytes& frameBytes)
    {
        auto decoded = decoder.decodePacket(bytes_to_vec(frameBytes));
        if (!decoded)
        {
            return py::none();
        }

        return py::make_tuple(
            decoded->callsignTo,
            decoded->callsignFrom,
            decode_payload_text(decoded->payloadData));
    }
}

PYBIND11_MODULE(AX25Python, m)
{
    m.doc() = "AX.25 helpers";

    //Config Struct
    py::class_<AX25Config>(m, "AX25Config")
        .def(py::init<>())
        .def(py::init([](const std::string& callsign_to, int ssid_to, const std::string& callsign_from, int ssid_from)
        {
            AX25Config cfg;
            cfg.callsignTo = callsign_to;
            cfg.callsignFrom = callsign_from;
            cfg.ssidTo = ssid_to;
            cfg.ssidFrom = ssid_from;
            return cfg;
        }), py::arg("callsign_to") = "", py::arg("ssid_to") = 0, py::arg("callsign_from") = "",
            py::arg("ssid_from") = 0)
        .def_readwrite("callsign_to", &AX25Config::callsignTo)
        .def_readwrite("callsign_from", &AX25Config::callsignFrom)
        .def_readwrite("ssid_to", &AX25Config::ssidTo)
        .def_readwrite("ssid_from", &AX25Config::ssidFrom)
        .def_property("dest_call",
            [](const AX25Config& self) { return self.callsignTo; },
            [](AX25Config& self, const std::string& value) { self.callsignTo = value; })
        .def_property("dest_ssid",
            [](const AX25Config& self) { return self.ssidTo; },
            [](AX25Config& self, int value) { self.ssidTo = value; })
        .def_property("src_call",
            [](const AX25Config& self) { return self.callsignFrom; },
            [](AX25Config& self, const std::string& value) { self.callsignFrom = value; })
        .def_property("src_ssid",
            [](const AX25Config& self) { return self.ssidFrom; },
            [](AX25Config& self, int value) { self.ssidFrom = value; })
        .def_property_readonly("dest_frame", [](const AX25Config& self)
        {
            return vec_to_bytes(build_ax25_call(self.callsignTo, self.ssidTo, false));
        })
        .def_property_readonly("src_frame", [](const AX25Config& self)
        {
            return vec_to_bytes(build_ax25_call(self.callsignFrom, self.ssidFrom, true));
        });

    py::class_<AX25DecodedPacket>(m, "AX25DecodedPacket")
        .def_readonly("callsign_to", &AX25DecodedPacket::callsignTo)
        .def_readonly("callsign_from", &AX25DecodedPacket::callsignFrom)
        .def_property_readonly("text", [](const AX25DecodedPacket& self)
        {
            return decode_payload_text(self.payloadData);
        })
        .def_property_readonly("payload", [](const AX25DecodedPacket& self)
        {
            return vec_to_bytes(self.payloadData);
        });

    py::class_<AX25Decoder>(m, "AX25Decoder")
        .def(py::init<>())
        .def("decode_packet", [](AX25Decoder& self, const py::bytes& frame_bytes)
        {
            return decode_packet_object(self, frame_bytes);
        }, py::arg("frame"))
        .def("decode", [](AX25Decoder& self, const py::bytes& frame_bytes)
        {
            return decode_packet_tuple(self, frame_bytes);
        }, py::arg("frame"));

    py::class_<AX25FrameBuilder>(m, "AX25FrameBuilder")
        .def(py::init([](const AX25Config& config, const py::object& control, const py::object& pid)
        {
            return AX25FrameBuilder(config, bytes_like_to_vec(control), bytes_like_to_vec(pid));
        }), py::arg("cfg"), py::arg("control") = py::bytes("\x03", 1), py::arg("pid") = py::bytes("\x01", 1))
        .def("buildAx25Frame", [](AX25FrameBuilder& self, const py::bytes& payload_bytes)
        {
            auto payload = bytes_to_vec(payload_bytes);
            auto frame = self.buildAx25Frame(payload);
            return vec_to_bytes(frame);
        }, py::arg("payload"))
        .def("build_ax25_frame", [](AX25FrameBuilder& self, const py::bytes& payload_bytes)
        {
            auto payload = bytes_to_vec(payload_bytes);
            auto frame = self.buildAx25Frame(payload);
            return vec_to_bytes(frame);
        }, py::arg("payload"))
        .def("buildKissFrame", [](AX25FrameBuilder& self, const py::bytes& payload_bytes)
        {
            auto payload = bytes_to_vec(payload_bytes);
            auto frame = self.buildKissFrame(payload);
            return vec_to_bytes(frame);
        }, py::arg("payload"))
        .def("build_kiss_frame", [](AX25FrameBuilder& self, const py::bytes& payload_bytes)
        {
            auto payload = bytes_to_vec(payload_bytes);
            auto frame = self.buildKissFrame(payload);
            return vec_to_bytes(frame);
        }, py::arg("ax25_frame"))
        .def("decode", [](AX25FrameBuilder&, const py::bytes& frame_bytes)
        {
            AX25Decoder decoder;
            return decode_packet_tuple(decoder, frame_bytes);
        }, py::arg("frame"));

    py::class_<DirewolfConnectionInfo>(m, "DirewolfConnectionInfo")
        .def(py::init<>())
        .def(py::init<const std::string&, int>(), py::arg("host") = "127.0.0.1", py::arg("port") = 8001)
        .def_readwrite("host", &DirewolfConnectionInfo::host)
        .def_readwrite("port", &DirewolfConnectionInfo::port);

    py::class_<DirewolfPacketInfo>(m, "DirewolfPacketInfo")
        .def(py::init<>())
        .def(py::init([](const std::string& to_callsign, int to_ssid, const std::string& from_callsign, int from_ssid)
        {
            DirewolfPacketInfo info;
            info.toCallsign = to_callsign;
            info.toSSID = to_ssid;
            info.fromCallsign = from_callsign;
            info.fromSSID = from_ssid;
            return info;
        }),
            py::arg("to_callsign") = "NOCALL",
            py::arg("to_ssid") = 0,
            py::arg("from_callsign") = "NOCALL",
            py::arg("from_ssid") = 0)
        .def_readwrite("to_callsign", &DirewolfPacketInfo::toCallsign)
        .def_readwrite("to_ssid", &DirewolfPacketInfo::toSSID)
        .def_readwrite("from_callsign", &DirewolfPacketInfo::fromCallsign)
        .def_readwrite("from_ssid", &DirewolfPacketInfo::fromSSID);

    py::class_<DirewolfConfig>(m, "DirewolfConfig")
        .def(py::init<>())
        .def(py::init([](const DirewolfConnectionInfo& connection_info, const DirewolfPacketInfo& packet_info)
        {
            DirewolfConfig config;
            config.connectionInfo = connection_info;
            config.packetInfo = packet_info;
            return config;
        }), py::arg("connection_info"), py::arg("packet_info"))
        .def_readwrite("connection_info", &DirewolfConfig::connectionInfo)
        .def_readwrite("packet_info", &DirewolfConfig::packetInfo);

    py::class_<Direwolf>(m, "Direwolf")
        .def(py::init<>())
        .def(py::init<DirewolfConfig>(), py::arg("config"))
        .def("listen", &Direwolf::listen)
        .def("send_packet", [](Direwolf& self, int packet_type, const py::bytes& payload_bytes, bool compress)
        {
            self.sendPacket(packet_type, bytes_to_vec(payload_bytes), compress);
        }, py::arg("packet_type"), py::arg("payload"), py::arg("compress") = false)
        .def("on_packet_received", [](Direwolf& self, int packet_type, py::function callback)
        {
            self.onPacketReceived<std::vector<uint8_t>>(packet_type, [callback = std::move(callback)](std::vector<uint8_t> payload)
            {
                py::gil_scoped_acquire acquire;
                callback(vec_to_bytes(payload));
            });
        }, py::arg("packet_type"), py::arg("callback"));
}
