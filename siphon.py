#!/usr/bin/env python3


import sys
import xmlrpc.client
import serial
import struct
import time
import logging


# =====
class Siphon:
    def __init__(self, device):
        self._tty = serial.Serial(device, 115200)

    def send(self, download, upload, has_download, has_upload):
        self._tty.write(struct.pack("<cHccccc", *((b"\x01", download) + (b"\x00",) * 5)))
        self._tty.write(struct.pack("<cHccccc", *((b"\x02", upload) + (b"\x00",) * 5)))
        self._tty.write(struct.pack("<cccccccc", *((b"\x03", self._make_byte(has_download)) + (b"\x00",) * 6)))
        self._tty.write(struct.pack("<cccccccc", *((b"\x04", self._make_byte(has_upload)) + (b"\x00",) * 6)))

    def _make_byte(self, value):
        return bytes([int(value)])

    def receive(self):
        self._tty.write(struct.pack("<cccccccc", *((b"\x05",) + (b"\x00",) * 7)))
        download = struct.unpack("<H", self._tty.read(2))[0]
        self._tty.write(struct.pack("<cccccccc", *((b"\x06",) + (b"\x00",) * 7)))
        upload = struct.unpack("<H", self._tty.read(2))[0]
        return (download, upload)


class Server:
    def __init__(self, url) :
        self._server = xmlrpc.client.ServerProxy(url)
        self._prev_down = None
        self._prev_up = None

    def get_speed(self) :
        multicall = xmlrpc.client.MultiCall(self._server)
        multicall.get_down_rate()
        multicall.get_up_rate()
        return tuple(map(self._make_speed, multicall()))

    def set_speed_limits(self, download, upload) :
        if self._prev_down != download or self._prev_up != upload :
            multicall = xmlrpc.client.MultiCall(self._server)
            if self._prev_down != download :
                multicall.set_download_rate(self._make_limit(download))
                self._prev_down = download
            if self._prev_up != upload :
                multicall.set_upload_rate(self._make_limit(upload))
                self._prev_up = upload
            multicall()
            return True
        return False

    def _make_speed(self, speed) :
        return int(speed * 8.0 / (1024.0 ** 2))

    def _make_limit(self, speed) :
        return int(speed / 8.0 * (1024.0 ** 2))


# =====
def main():
    assert len(sys.argv) == 3

    logger = logging.getLogger("siphon")
    logger.setLevel(logging.DEBUG)
    handler = logging.StreamHandler()
    handler.setLevel(logging.DEBUG)
    formatter = logging.Formatter("%(asctime)s - %(name)s [%(levelname)s]: %(message)s")
    handler.setFormatter(formatter)
    logger.addHandler(handler)

    server = Server(sys.argv[1])
    siphon = Siphon(sys.argv[2])

    while True :
        (download, upload) = server.get_speed()
        logger.info("siphon << server: speed:  D:%d / U:%d", download, upload)
        siphon.send(download, upload, download != 0, upload != 0)

        (download, upload) = siphon.receive()
        if server.set_speed_limits(download, upload):
            logger.info("siphon >> server: limits: D:%d / U:%d", download, upload)
        time.sleep(1)


if __name__ == "__main__" :
    main()
