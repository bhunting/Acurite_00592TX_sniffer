## Acurite_00592TX_sniffer
Arduino code to sniff and decode Acurite 00592TX remote wireless temperature probe

Ideas on decoding protocol and prototype code from Ray Wang (Rayshobby LLC) http://rayshobby.net/?p=8998

Sniff the AcuRite model 00771W Indoor / Outdoor Thermometer wireless data stream and display the results.

 * http://www.acurite.com/media/manuals/00754-instructions.pdf

Code based on Ray Wang's humidity_display.ino source. Heavily modified by Brad Hunting.

The 00592TX wireless temperature probe contains a 433 MHz wireless transmitter. The temperature from the probe is sent approximately every 16 seconds.

The 00592TX typically only sends one SYNC pulse + DATA stream per temperature reading. Infrequently two sync/data streams are sent during the same transmit window but that seems to be the exception.

Ray Wang's code is for a different model of probe, one that transmits both temperature and humidity. Ray' code relies on two sync streams with a preceeding delay. 
 
The 00592TX usually starts the data sync bits right after the RF sync pulses which are random length and polarity.

Do not rely on a dead/mark time at the beginning of the data sync stream.

The 00592TX first emits a seemingly random length string of random width hi/lo pulses, most like to provide radio radio synchronization.

The probe then emits 4 data sync pulses of approximately 50% duty cycle and 1.2 ms period. The sync pulses start with a high level and continue for 4 high / low pulses.

The data bits immediately follow the fourth low of the data sync pulses. Data bits are sent every ~0.61 msec as:

 * 1 bit ~0.4 msec high followed by ~0.2 msec low
 * 0 bit ~0.2 msec high followed by ~0.4 msec low

The 00771W sends the 4 sync pulses followed by 7 bytes of data equalling 56 bits.

The code below works by receiving a level change interrupt on each changing edge of the data stream from the RF module and recording the time in uSec between each edge.

Eight (8) measured hi and lo pulses in a row, 4 high and 4 low, of approximately 600 uSec each constitue a sync stream.

The remaining 56 bits of data, or 112 edges, are measured and converted to 1s and 0s by checking the high to low pulse times.

The first 4 pulses, or 8 edges, are the sync pulses followed by the 56 bits, or 112 edges, of the data pulses.

We measure 8 sync edges followed by 112 data edges so the time capture buffer needs to be at least 120 long.

This code presently does not calculate the checksum of the data stream. It simply displays the results of what was captured from the RF module.

The data stream is 7 bytes long.

The first and second bytes are unique address bytes per probe.
  The upper two bits of the first byte are the probe channel indicator:
   * 11 = channel A
   * 10 = channel B
   * 00 = channel C
  The remaining 6 bits of the first byte and the 8 bits of the second byte are a unique identifier per probe.

The next two bytes are always 0x44 followed by 0x90, for all of the probes I have tested (a sample of 6 probes).

The next two bytes are the temperature value. 
 The temperature is encoded as the lower 7 bits of both bytes with the most significant bit being an even parity bit.  The MSB will be set if required to insure an even number of bits are set to 1 in the byte. If the least significant seven bits have an even number of 1 bits set the MSB will be 0, otherwise the MSB will be set to 1 to insure an even number of bits.

The last byte is a simple running sum, modulo 256, of the previous 6 data bytes.
