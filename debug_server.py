import configparser
import socket
import sys
from pathlib import Path

def load_kiss_config(config_path: Path) -> tuple[str, int]:
    cfg = configparser.ConfigParser()
    if not cfg.read(config_path):
        raise FileNotFoundError(f"Could not read config file: {config_path}")

    host = cfg.get("KISS", "kiss_host", fallback="127.0.0.1")
    port = cfg.getint("KISS", "kiss_port", fallback=8001)
    return host, port


def find_default_config() -> Path:
    candidates = [
        Path("config.ini"),
        Path("AX25Console") / "config.ini",
    ]
    for p in candidates:
        if p.exists():
            return p
    return Path("config.ini")


def run_server(host: str, port: int) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((host, port))
        server.listen(5)
        print(f"Listening on {host}:{port}")

        while True:
            conn, addr = server.accept()
            print(f"Connected: {addr[0]}:{addr[1]}")
            with conn:
                while True:
                    data = conn.recv(4096)
                    if not data:
                        break
                    print(data.hex().upper())
            print(f"Disconnected: {addr[0]}:{addr[1]}")


def main() -> None:
    config_path = Path(sys.argv[1]) if len(sys.argv) > 1 else find_default_config()
    host, port = load_kiss_config(config_path)
    run_server(host, port)


if __name__ == "__main__":
    main()