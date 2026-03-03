#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "AX25Decoder.hpp"
#include "AX25FrameBuilder.hpp"
#include "AX25Config.hpp"

namespace py = pybind11;

namespace
{
    std::vector<uint8_t> bytes_to_vec(const py::bytes& b)
    {
        std::string s = b; // pybind11 provides conversion to std::string
        return std::vector<uint8_t>(s.begin(), s.end());
    }

    py::bytes vec_to_bytes(const std::vector<uint8_t>& v)
    {
        return py::bytes(reinterpret_cast<const char*>(v.data()), v.size());
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
                 cfg.callsignTo = callsign_to.c_str();
                 cfg.callsignFrom = callsign_from.c_str();
                 cfg.ssidTo = ssid_to;
                 cfg.ssidFrom = ssid_from;
                 return cfg;
             }), py::arg("callsign_to") = "", py::arg("callsign_from") = "", py::arg("ssid_to") = 0,
             py::arg("ssid_from") = 0)
        .def_readwrite("callsign_to", &AX25Config::callsignTo)
        .def_readwrite("callsign_from", &AX25Config::callsignFrom)
        .def_readwrite("ssid_to", &AX25Config::ssidTo)
        .def_readwrite("ssid_from", &AX25Config::ssidFrom);

    py::class_<AX25DecodedPacket>(m, "AX25DecodedPacket")
        .def_readonly("callsign_to", &AX25DecodedPacket::callsignTo)
        .def_readonly("callsign_from", &AX25DecodedPacket::callsignFrom)
        .def_readonly("text", &AX25DecodedPacket::textData);

    py::class_<AX25Decoder>(m, "AX25Decoder")
        .def(py::init<>())
        .def("decode_packet", [](AX25Decoder& self, const py::bytes& frame_bytes)
        {
            auto frame = bytes_to_vec(frame_bytes);
            auto decoded = self.decodePacket(frame);
            if (decoded) return py::cast(*decoded);
            return py::object(py::none());
        }, py::arg("frame"));

    py::class_<AX25FrameBuilder>(m, "AX25FrameBuilder")
        .def(py::init<const AX25Config&>())
        .def("buildAx25Frame", [](AX25FrameBuilder& self, const py::bytes& payload_bytes)
        {
            auto payload = bytes_to_vec(payload_bytes);
            auto frame = self.buildAx25Frame(payload);
            return vec_to_bytes(frame);
        }, py::arg("payload"));
}
