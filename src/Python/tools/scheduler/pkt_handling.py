"""
This module handles packet processing for the scheduling system.
"""

from __future__ import print_function

import struct

from . import constants


class SchedulerPacket:

    def __init__(self):

        self.buf = bytearray()

    def process_incoming_byte(self, byte):
        """Process incoming information byte-by-byte"""

        # If byte is 0, then a packet was found
        if byte == b'\x00':
            return True

        # Reset rx buffer if max allowed size is reached
        if len(self.buf) >= constants.MAX_ENCODED_PKT_BUF_SIZE:
            self.buf = bytearray()

        self.buf += bytearray([byte])

        return False

    def process_incoming_pkt(self, task_table):
        """Process a completed rx task packet

        Processing an rx packet entails the following:

        1. It will COBS decode the rx packet.

        2. It will verify different attributes of the packet
           to see if the packet is valid or not.
        """

        # Check for minimum header length
        if len(self.buf) < constants.ENCODED_HDR_SIZE:
            print("PKT RX PROC ERROR: Packet is shorter than minimum header size")
            return

        # Decode incoming packet
        if not self._decode():
            print("PKT RX PROC ERROR: Error in decoding incoming packet")
            return

        print('\n')
        print("Processed incoming pkt")
        print(self.buf)
        print("task id {}".format(self.buf[constants.TASK_ID_OFFSET]))
        print("task type {}".format(self.buf[constants.TASK_TYPE_OFFSET]))
        print(':'.join(hex(char) for char in self.buf) + '\n')

        # crc16 verification to validate decoded packet
        # Read the two crc16 bytes assuming little-endianess
        crc16_checksum = struct.unpack('<H', self.buf[constants.CRC16_OFFSET: constants.CRC16_OFFSET + 2])[0]

        # TODO: For now, I'm just checking for 0. Change for an actual crc16 checker.
        if crc16_checksum != 0:
            print("PKT RX PROC ERROR: crc16 fail")
            return

        # Check if the requested task type is internal
        if self.buf[constants.TASK_TYPE_OFFSET] == constants.INTERNAL_TASK:
            return

        entry = task_table.get(self.buf[constants.TASK_ID_OFFSET])

        # Check if entry was registered
        if entry is None:
            print("PKT RX PROC ERROR: Task {} was not registered".format(self.buf[constants.TASK_ID_OFFSET]))
            return

        # Check if stored packet size in task table matches size of
        # the current packet
        if entry.size > 0:
            if len(self.buf) != entry.size + constants.DECODED_HDR_SIZE:
                print()
                return

        return entry

    def process_outgoing_pkt(self, task_id, task_type, payload_pkt):

        # Check if the task payload is small enough to fit
        if len(payload_pkt) + constants.DECODED_HDR_SIZE > constants.MAX_DECODED_PKT_BUF_SIZE:
            return False

        # Create the decoded packet to be sent and then encode
        self.buf = bytearray(constants.PAYLOAD_OFFSET)

        self.buf[constants.TASK_ID_OFFSET] = task_id
        self.buf[constants.TASK_TYPE_OFFSET] = task_type
        self.buf += bytearray(payload_pkt)

        # TODO: Put crc16 stuff here for the input buffer (for now I'm just entering 0)
        self.buf[constants.CRC16_OFFSET: constants.CRC16_OFFSET + 2] = bytearray([0, 0])

        print('\n')
        print("Processed outgoing pkt")
        print(self.buf)
        print(':'.join(hex(char) for char in self.buf) + '\n')

        self._encode()

        return True

    def _encode(self):
        """COBS encode a packet and add COBS delimeter.

        Based on C library function made by Jacques Fortier.
        """

        code = 1  # Encoded byte in the output (indicates where the next 0 is)
        code_index = 0  # Index where the last zero was found
        read_index = 0  # Index for byte to read in input
        write_index = 1  # Index for byte to write in output
        length = len(self.buf)
        encoded_pkt = bytearray(constants.MAX_ENCODED_PKT_BUF_SIZE)

        while read_index < length:

            byte = self.buf[read_index]
            read_index += 1

            # If byte is not 0, pass to output
            if byte:
                encoded_pkt[write_index] = byte
                write_index += 1
                code += 1

            # If byte is 0 or code reached 255 (block completed), then restart the code count
            if not byte or code == 255:
                encoded_pkt[code_index] = code
                code = 1
                code_index = write_index
                write_index += 1

        encoded_pkt[write_index] = 0  # Place the COBS delimiter byte at the end
        encoded_pkt[code_index] = code  # Place the location of the COBS delimiter byte
        write_index += 1

        self.buf = encoded_pkt[:write_index]

    def _decode(self):
        """COBS decode a packet.

        Returns True if the decoding was successful. Otherwise,
        it'll return False.

        Based on C library function made by Jacques Fortier.
        """

        read_index = 0
        write_index = 0
        length = len(self.buf)
        decoded_pkt = bytearray(constants.MAX_DECODED_PKT_BUF_SIZE)

        while read_index < length:

            code = self.buf[read_index]

            # If encoded byte (code) points outside the range of the input, activate packet buffer
            if read_index + code > length and code != 1:
                self.buf = bytearray()
                return False

            read_index += 1

            # Directly pass other values that are not the encoded zero
            for i in range(1, code):
                decoded_pkt[write_index] = self.buf[read_index]
                read_index += 1
                write_index += 1

            # Translate encoded 0 to byte
            if code != 255 and read_index != length:
                decoded_pkt[write_index] = 0
                write_index += 1

        self.buf = decoded_pkt[:write_index]
        return True
