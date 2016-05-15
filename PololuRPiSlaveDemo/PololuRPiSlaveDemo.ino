#include <Arduino.h>

#include <PololuRPiSlave.h>

/* This example program shows how to make the A-Star 32U4 Robot
 * Controller into a Raspberry Pi I2C slave.  The RPi and A-Star can
 * exchange data bidirectionally, allowing each device to do what it
 * does best: high-level programming can be handled in a language such
 * as Python on the RPi, while the A-Star takes charge of motor
 * control, analog inputs, and other low-level I/O.
 *
 * The example and libraries are available for download at:
 *
 * https://github.com/pololu/pololu-rpi-slave-arduino-library
 *
 * You will need the corresponding Raspberry Pi code, which is
 * available in that repository under the pi/ subfolder.  The Pi code
 * sets up a simple Python-based web application as a control panel
 * for your Raspberry Pi robot.
 */

// Custom data structure that we will use for interpreting the buffer.
// We recommend keeping this under 64 bytes total.  If you change the
// data format, make sure to update the corresponding code in
// a_star.py on the Raspberry Pi.


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

PololuRPiSlave<struct Data,0> slave;

void setup()
{
   Serial.begin(9600);
   Serial.println("Started.");

  // Set up the slave at I2C address 20.
  slave.init(20);

  // Call updateBuffer() before using the buffer, to get the latest
  // data including recent master writes.
  slave.updateBuffer();
  
  // pre-fill various values into the data structure.
  for(uint8_t i = 0; i < _numSensors; i++)
  {
    slave.buffer.sensordata[i].id = i+1;
  }
  // When you are done WRITING, call finalizeWrites() to make modified
  // data available to I2C master.
  slave.finalizeWrites();
}

void loop()
{
  // Call updateBuffer() before using the buffer, to get the latest
  // data including recent master writes.
  slave.updateBuffer();

  // Write various values into the data structure.
  //for(uint8_t i = 0; i < _numSensors; i++)
  //{
  //  slave.buffer.sensordata[i].id = i+1;
  //}

  // READING the buffer is allowed before or after finalizeWrites().
      for( int i = 0; i < _numSensors; i++ )
      {
        Serial.print("id = ");
        Serial.print(slave.buffer.sensordata[i].id);
        Serial.print(", status = ");
        Serial.print(slave.buffer.sensordata[i].status, HEX);
        Serial.print(", temperature = ");
        Serial.print(slave.buffer.sensordata[i].temperature);
        Serial.print(", time = ");
        Serial.println(slave.buffer.sensordata[i].timestamp);
      }

  // When you are done WRITING, call finalizeWrites() to make modified
  // data available to I2C master.
  slave.finalizeWrites();
}
