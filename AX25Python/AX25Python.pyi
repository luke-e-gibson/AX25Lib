from typing import Optional

class AX25Config:
    callsign_to: str
    callsign_from: str
    ssid_to: int
    ssid_from: int
    def __init__(self, callsign_to: str = "", ssid_to: int = 0,
                 callsign_from: str = "", ssid_from: int = 0) -> None: ...

class AX25DecodedPacket:
    callsign_to: str
    callsign_from: str
    text: str

class AX25Decoder:
    def __init__(self) -> None: ...
    def decode_packet(self, frame: bytes) -> Optional[AX25DecodedPacket]: ...

class AX25FrameBuilder:
    def __init__(self, cfg: AX25Config) -> None: ...
    def buildAx25Frame(self, payload: bytes) -> bytes: ...
