#!/usr/bin/env python3


import xmlrpc.client
import serial
import struct
import time


##### Public classes #####
class Siphon :
	def __init__(self, port, speed) :
		self._tty = serial.Serial(port, speed)

	def send(self, download, upload, download_flag, upload_flag) :
		self._tty.write(struct.pack("<HHcc", download, upload, bytes([int(download_flag)]), bytes([int(upload_flag)])))

	def receive(self) :
		return struct.unpack("<HH", self._tty.read(4))

class Server :
	def __init__(self, url) :
		self._server = xmlrpc.client.ServerProxy(url)
		self._prev_down = None
		self._prev_up = None

	def getSpeed(self) :
		return map(lambda arg : int(arg * 8.0 / (1024.0 ** 2)), (
				self._server.get_down_rate(),
				self._server.get_up_rate(),
			))

	def setSpeedLimits(self, download, upload) :
		if self._prev_down != download :
			self._server.set_download_rate(int(download / 8.0 * ( 1024.0 ** 2 )))
			self._prev_down = download
		if self._prev_up != upload :
			self._server.set_upload_rate(int(upload / 8.0 * ( 1024.0 ** 2 )))
			self._prev_up = upload


##### Main #####
def main() :
	siphon = Siphon("/dev/ttyACM0", 115200)
	server = Server("http://192.168.0.2/RPC2")
	a, b = (0, 0)
	while True :
		#(speed_down, speed_up) = server.getSpeed()
		#siphon.send(speed_down, speed_up, False, False)
		siphon.send(a, b, True, False)
		a, b = siphon.receive()
		print (a, b)
#		server.setSpeedLimits(*siphon.getSpeedLimits())
		time.sleep(1)

if __name__ == "__main__" :
	main()

