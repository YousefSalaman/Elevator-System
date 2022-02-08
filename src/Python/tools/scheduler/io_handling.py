
import threading
from time import time

import serial
import serial.tools.list_ports as list_ports


class SerialMessager:
    """"""

    _baudrate = 9600
    _messagers = set()
    _outgoing_msgs = {}  # Messages that
    _rx_lock = threading.Lock()

    def __init__(self, port):

        self.thread = threading.Thread(target=self.mcu_io_handler)
        self.serial_ch = serial.Serial(port=port, baudrate=self._baudrate)

        self._messagers.add(self)
        self._outgoing_msgs[self.thread.ident] = None

    @classmethod
    def set_baudrate(cls, baudrate):

        cls._baudrate = baudrate

    @classmethod
    def pass_outgoing_message(cls, pkt, thread_id):
        """Pass a packet to the SerialMessager, so it can send it to an MCU"""

        cls._outgoing_msgs[thread_id] = pkt

    def mcu_io_handler(self):

        while True:

            # Send outgoing message if there is any
            if self._outgoing_msgs.get(self.thread.ident) is not None:
                pass

            # Read incoming message if
            if self.serial_ch.in_waiting:
                pass
