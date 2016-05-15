# PololuRPiSlaveDemo

Demo / Test code to play with interfacing between the RasPi and a custom Arduino I2C slave.
The RasPi main CPU has a bug in the silicon relating to slave I2C clock stretching.
Using the Arduino Wire library request / receive events does not work well. There are significant numbers of read error on the RasPi side. Pololu has written a library that seems to work by breaking the read / writes into bytes and inserting delays as required to accomodate the RPi bug.

https://github.com/pololu/pololu-rpi-slave-arduino-library

This a simple modification of the Pololu demo code to test the library.

Testing on the Pi side
```
pi@raspberrypi:~$ sudo i2cdetect -y 1
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- 14 -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
70: -- -- -- -- -- -- -- --
```
0x14 is the custom Arduino slave

The arduino slave defines a Data structure containing an array of 6 data structures each data structure being 8 bytes long.

```
typedef struct sensortemperatureData
{
    uint8_t     id;             // sensor id (1 - N_sensors)
    uint8_t     status;         // 0x80 = BATTERY LOW bit
    uint16_t    temperature;    // temperature value in C, no offset
    uint32_t    timestamp;      // number of seconds since startup
} sensortemperatureData;

static const uint8_t _numSensors = 6; // I happen to have 6 sensor probes

struct Data
{
    sensortemperatureData sensordata[_numSensors];
};
```
Information is written to the Data structure asynchronously by both the Arduino and RasPi. Double (triple) buffering is used by the Pololu library so the two functions

```
  // Call updateBuffer() before using the buffer, to get the latest
  // data including recent master writes.
  slave.updateBuffer();
  
  // When you are done WRITING, call finalizeWrites() to make modified
  // data available to I2C master.
  slave.finalizeWrites();
```
are required to update the buffers before / after reading / writing.

The Arduino can access the data as:

dataStructureInstance.buffer.datamember;

```
  // pre-fill various values into the data structure.
  for(uint8_t i = 0; i < _numSensors; i++)
  {
    slave.buffer.sensordata[i].id = i+1;
  }
```

On the Raspberry Pi side when reading or writing data use the raw offset into the data buffer as part of the I2C read or write.

Use the command i2c.read_i2c_block_data(0x14, 0x00, 8) to read from I2C address 0x14, read from offset 0 in the buffer, and read 8 bytes which is the length of one sensor data structure.

Use the command i2c.read_i2c_block_data(0x14, 0x08, 8) to read the second sensor data structure.
Use the command i2c.read_i2c_block_data(0x14, 0x10, 8) to read the third sensor data structure.
Use the command i2c.read_i2c_block_data(0x14, 0x18, 8) to read the fourth sensor data structure.

The serial output from the Arduino on startup is

```
id = 1, status = 0, temperature = 0, time = 0
id = 2, status = 0, temperature = 0, time = 0
id = 3, status = 0, temperature = 0, time = 0
id = 4, status = 0, temperature = 0, time = 0
id = 5, status = 0, temperature = 0, time = 0
id = 6, status = 0, temperature = 0, time = 0
```

Using Python to read from the Arduino

```
pi@raspberrypi:~$ sudo python
Python 2.7.3 (default, Mar 18 2014, 05:13:23)
[GCC 4.6.3] on linux2
Type "help", "copyright", "credits" or "license" for more information.
>>> import smbus
>>> i2c = smbus.SMBus(1)
>>> i2c.read_i2c_block_data(0x14,0,8)
[1, 0, 0, 0, 0, 0, 0, 0]
>>> i2c.read_i2c_block_data(0x14,0x08,8)
[2, 0, 0, 0, 0, 0, 0, 0]
>>> i2c.read_i2c_block_data(0x14,0x10,8)
[3, 0, 0, 0, 0, 0, 0, 0]
>>> i2c.read_i2c_block_data(0x14,0x18,8)
[4, 0, 0, 0, 0, 0, 0, 0]
>>>
```

To write to the data structure on the Arduino use:

i2c.write_i2c_block_data(0x14, 0x00, [1,2,3,4,5,6,7,8]) to write at offset 0 the 8 values 1,2,3,4,5,6,7,8.

Results in the Arduino displaying

```
id = 1, status = 2, temperature = 1027, time = 134678021
id = 2, status = 0, temperature = 0, time = 0
id = 3, status = 0, temperature = 0, time = 0
id = 4, status = 0, temperature = 0, time = 0
id = 5, status = 0, temperature = 0, time = 0
id = 6, status = 0, temperature = 0, time = 0
```

Now reading back the data from the Arduino to the RasPi


```
pi@raspberrypi:~$ sudo python
Python 2.7.3 (default, Mar 18 2014, 05:13:23)
[GCC 4.6.3] on linux2
Type "help", "copyright", "credits" or "license" for more information.
>>> import smbus
>>> i2c = smbus.SMBus(1)
>>> i2c.read_i2c_block_data(0x14,0,8)
[1, 0, 0, 0, 0, 0, 0, 0]
>>> i2c.read_i2c_block_data(0x14,0x08,8)
[5, 6, 7, 8, 9, 10, 11, 12]
>>> i2c.read_i2c_block_data(0x14,0x08,8)
[2, 0, 0, 0, 0, 0, 0, 0]
>>> i2c.read_i2c_block_data(0x14,0x10,8)
[3, 0, 0, 0, 0, 0, 0, 0]
>>> i2c.read_i2c_block_data(0x14,0x18,8)
[4, 0, 0, 0, 0, 0, 0, 0]
>>>
>>>
>>> i2c.write_i2c_block_data(0x14, 0x00, [1,2,3,4,5,6,7,8])
>>>
>>> i2c.read_i2c_block_data(0x14,0x00,8)
[1, 2, 3, 4, 5, 6, 7, 8]
>>>
```

