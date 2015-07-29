#!/usr/bin/env python3


import sys
import xmlrpc.client
import serial
import struct
import time
import logging


##### Public classes #####
class Siphon :
	def __init__(self, port, speed) :
		self._tty = serial.Serial(port, speed)

	def send(self, download, upload, download_flag, upload_flag) :
		self._tty.write(struct.pack("<HHcc", download, upload, self._makeByte(download_flag), self._makeByte(upload_flag)))

	def receive(self) :
		return struct.unpack("<HH", self._tty.read(4))

	def _makeByte(self, value) :
		return bytes([int(value)])

class Server :
	def __init__(self, url) :
		self._server = xmlrpc.client.ServerProxy(url)
		self._prev_down = None
		self._prev_up = None

	def getSpeed(self) :
		multicall = xmlrpc.client.MultiCall(self._server)
		multicall.get_down_rate()
		multicall.get_up_rate()
		return tuple(map(self._makeSpeed, multicall()))

	def setSpeedLimits(self, download, upload) :
		if self._prev_down != download or self._prev_up != upload :
			multicall = xmlrpc.client.MultiCall(self._server)
			if self._prev_down != download :
				multicall.set_download_rate(self._makeLimit(download))
				self._prev_down = download
			if self._prev_up != upload :
				multicall.set_upload_rate(self._makeLimit(upload))
				self._prev_up = upload
			multicall()
			return True
		return False

	def _makeSpeed(self, speed) :
		return int(speed * 8.0 / (1024.0 ** 2))

	def _makeLimit(self, speed) :
		return int(speed / 8.0 * (1024.0 ** 2))


##### Main #####
def main() :
	assert len(sys.argv) == 3

	logger = logging.getLogger("siphon")
	logger.setLevel(logging.DEBUG)
	handler = logging.StreamHandler()
	handler.setLevel(logging.DEBUG)
	formatter = logging.Formatter("%(asctime)s - %(name)s [%(levelname)s]: %(message)s")
	handler.setFormatter(formatter)
	logger.addHandler(handler)

	server = Server(sys.argv[1])
	siphon = Siphon(sys.argv[2], 115200)

	while True :
		(download, upload) = server.getSpeed()
		logger.info("siphon << server: speed:  D:%d / U:%d", download, upload)
		siphon.send(download, upload, download != 0, upload != 0)

		(download, upload) = siphon.receive()
		if server.setSpeedLimits(download, upload) :
			logger.info("siphon >> server: limits: D:%d / U:%d", download, upload)
		time.sleep(1)


if __name__ == "__main__" :
	main()

