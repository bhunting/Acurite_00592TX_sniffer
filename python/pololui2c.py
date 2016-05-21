# Copyright Pololu Corporation.  For more information, see https://www.pololu.com/
import smbus
import struct

class pololuI2C(object):
  def __init__(self):
    self.bus = smbus.SMBus(1)

  def set_address( _addr ):
    addr = _addr

  def read_unpack(self, address, size, format):
    # Ideally we could do this:
    #    byte_list = self.bus.read_i2c_block_data(20, address, size)
    # But the AVR's TWI module can't handle a quick write->read transition,
    # since the STOP interrupt will occasionally happen after the START
    # condition, and the TWI module is disabled until the interrupt can
    # be processed.

    self.bus.write_byte(20,address)
    byte_list = []
    for n in range(0,size):
      byte_list.append(self.bus.read_byte(20))
    return struct.unpack(format,bytes(bytearray(byte_list)))

  def write_pack(self, address, format, *data):
    data_array = map(ord, list(struct.pack(format, *data)))
    self.bus.write_i2c_block_data(20, address, data_array)

