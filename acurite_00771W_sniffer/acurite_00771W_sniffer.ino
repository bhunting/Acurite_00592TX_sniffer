/**********************************************************************
 * Arduino code to sniff the Acurite 00771W wireless temperature
 * probe output data stream.
 *
 * Ideas on decoding protocol and prototype code from
 * Ray Wang (Rayshobby LLC) http://rayshobby.net/?p=8998
 *
 * Sniff the AcuRite model 00771W Indoor / Outdoor Thermometer
 * wireless data stream and display the results.
 * http://www.acurite.com/media/manuals/00754-instructions.pdf
 *
 * Code based on Ray Wang's humidity_display.ino source.
 * Heavily modified by Brad Hunting.
 *
 * The 00771W wireless temperature probe contains a 433 MHz
 * wireless transmitter. The temperature from the probe is
 * sent approximately every 16 seconds.
 *
 * The 00771W typically only sends one SYNC pulse + DATA stream
 * per temperature reading. Infrequently two sync/data streams
 * are sent during the same transmit window but that seems to 
 * be the exception.
 *
 * Ray Wang's code is for a different model of probe, one that 
 * transmits both temperature and humidity. Ray' code relies on 
 * two sync streams with a preceeding delay. 
 * 
 * The 00771W usually starts the data sync bits right after
 * the RF sync pulses which are random length and polarity.
 *
 * Do not rely on a dead/mark time at the beginning of the 
 * data sync stream.
 *
 * The 00771W first emits a seemingly random length string of 
 * random width hi/lo pulses, most like to provide radio
 * radio synchronization.
 *
 * The probe then emits 4 data sync pulses of approximately 50% 
 * duty cycle and 1.2 ms period. The sync pulses start with a 
 * high level and continue for 4 high / low pulses.
 *
 * The data bits immediately follow the fourth low of the data
 * sync pulses. Data bits are sent every ~0.61 msec as:
 *
 * 1 bit ~0.4 msec high followed by ~0.2 msec low
 * 0 bit ~0.2 msec high followed by ~0.4 msec low
 *
 * The 00771W sends the 4 sync pulses followed by
 * 7 bytes of data equalling 56 bits.
 *
 * The code below works by receiving a level change interrupt 
 * on each changing edge of the data stream from the RF module
 * and recording the time in uSec between each edge.
 *
 * 8 measured hi and lo pulses in a row, 4 high and 4 low, of 
 * approximately 600 uSec each constitue a sync stream.
 *
 * The remaining 56 bits of data, or 112 edges, are measured
 * and converted to 1s and 0s by checking the high to low
 * pulse times.
 *
 * The first 4 pulses, or 8 edges, are the sync pulses followed
 * by the 56 bits, or 112 edges, of the data pulses.
 *
 * We measure 8 sync edges followed by 112 data edges so the 
 * time capture buffer needs to be at least 120 long.
 *
 * This code presently does not calculate the checksum of
 * the data stream. It simply displays the results of what was 
 * captured from the RF module.
 *
 * The data stream is 7 bytes long.
 * The first and second bytes are unique address bytes per probe.
 *   The upper two bits of the first byte are the probe channel indicator:
 *   11 = channel A
 *   10 = channel B
 *   00 = channel C
 *   The remaining 6 bits of the first byte and the 8 bits of the second
 *   byte are a unique identifier per probe.
 * The next two bytes seem to always be 0x44 followed by 0x90, for all of
 *   the probes I have tested (a sample of 6 probes).
 * The next two bytes are the temperature. The temperature is encoded as the
 *   lower 7 bits of both bytes with the most significant bit being an
 *   even parity bit.  The MSB will be set if required to insure an even
 *   number of bits are set to 1 in the byte. If the least significant
 *   seven bits have an even number of 1 bits set the MSB will be 0,
 *   otherwise the MSB will be set to 1 to insure an even number of bits.
 * The last byte is a simple running sum, modulo 256, of the previous 6 data bytes.
 */

// ring buffer size has to be large enough to fit
// data and sync signal, at least 120
// round up to 128 for now
#define RING_BUFFER_SIZE  128

#define SYNC_HIGH       600
#define SYNC_LOW        600
#define PULSE_LONG      400
#define PULSE_SHORT     220
#define BIT1_HIGH       PULSE_LONG
#define BIT1_LOW        PULSE_SHORT
#define BIT0_HIGH       PULSE_SHORT
#define BIT0_LOW        PULSE_LONG

// On the arduino connect the data pin, the pin that will be 
// toggling with the incomming data from the RF module, to
// digital pin 3. Pin D3 is interrupt 1 and can be configured
// for interrupt on change, change to high or low.
// The squelch pin in an input to the radio that squelches, or
// blanks the incoming data stream. Use the squelch pin to 
// stop the data stream and prevent interrupts between the 
// data packets if desired.
//
#define DATAPIN         (3)             // D3 is interrupt 1
#define SQUELCHPIN      (4)
#define SYNCPULSECNT    (4)             // 4 pulses (8 edges)
#define SYNCPULSEEDGES  (SYNCPULSECNT*2)
#define DATABYTESCNT    (7)
#define DATABITSCNT     (DATABYTESCNT*8)// 7 bytes * 8 bits
#define DATABITSEDGES   (DATABITSCNT*2)

// The pulse durations are the measured time in micro seconds between
// pulse edges.
unsigned long pulseDurations[RING_BUFFER_SIZE];
unsigned int syncIndex  = 0;    // index of the last bit time of the sync signal
unsigned int dataIndex  = 0;    // index of the first bit time of the data bits (syncIndex+1)
bool         syncFound = false; // true if sync pulses found
bool         received  = false; // true if sync plus enough bits found
unsigned int changeCount = 0;

/*
 * helper code to print formatted hex 
 */
void PrintHex8(uint8_t *data, uint8_t length) // prints 8-bit data in hex
{
 char tmp[length*2+1];
 byte first;
 int j = 0;
 for (uint8_t i = 0; i < length; i++) 
 {
   first = (data[i] >> 4) | 48;
   if (first > 57) tmp[j] = first + (byte)39;
   else tmp[j] = first ;
   j++;

   first = (data[i] & 0x0F) | 48;
   if (first > 57) tmp[j] = first + (byte)39; 
   else tmp[j] = first;
   j++;
 }
 tmp[length*2] = 0;
 Serial.print(tmp);
}


/*
 * Look for the sync pulse train, 4 high-low pulses of
 * 600 uS high and 600 uS low.
 * idx is index of last captured bit duration.
 * Search backwards 8 times looking for 4 pulses
 * approximately 600 uS long.
 */
bool isSync(unsigned int idx) 
{
   // check if we've received 4 pulses of matching timing
   for( int i = 0; i < SYNCPULSEEDGES; i += 2 )
   {
      unsigned long t1 = pulseDurations[(idx+RING_BUFFER_SIZE-i) % RING_BUFFER_SIZE];
      unsigned long t0 = pulseDurations[(idx+RING_BUFFER_SIZE-i-1) % RING_BUFFER_SIZE];    
      
      // any of the preceeding 8 pulses are out of bounds, short or long,
      // return false, no sync found
      if( t0<(SYNC_HIGH-100) || t0>(SYNC_HIGH+100) ||
          t1<(SYNC_LOW-100)  || t1>(SYNC_LOW+100) )
      {
         return false;
      }
   }
   return true;
}

/* Interrupt 1 handler 
 * Tied to pin 3 INT1 of arduino.
 * Set to interrupt on edge (level change) high or low transition.
 * Change the state of the Arduino LED (pin 13) on each interrupt. 
 * This allows scoping pin 13 to see the interrupt / data pulse train.
 */
void handler() 
{
   static unsigned long duration = 0;
   static unsigned long lastTime = 0;
   static unsigned int ringIndex = 0;
   static unsigned int syncCount = 0;
   static unsigned int bitState  = 0;

   bitState = digitalRead(DATAPIN);
   digitalWrite(13, bitState);

   // ignore if we haven't finished processing the previous 
   // received signal in the main loop.
   if( received == true )
   {
      return;
   }

   // calculating timing since last change
   long time = micros();
   duration = time - lastTime;
   lastTime = time;

   // Known error in bit stream is runt/short pulses.
   // If we ever get a really short, or really long, 
   // pulse we know there is an error in the bit stream
   // and should start over.
   if( (duration > (PULSE_LONG+100)) || (duration < (PULSE_SHORT-100)) )
   {
      received = false;
      syncFound = false;
      changeCount = 0;  // restart looking for data bits
   }

   // store data in ring buffer
   ringIndex = (ringIndex + 1) % RING_BUFFER_SIZE;
   pulseDurations[ringIndex] = duration;
   changeCount++; // found another edge

   // detect sync signal
   if( isSync(ringIndex) )
   {
      syncFound = true;
      changeCount = 0;  // restart looking for data bits
      syncIndex = ringIndex;
      dataIndex = (syncIndex + 1)%RING_BUFFER_SIZE;
   }

   // If a sync has been found the start looking for the
   // DATABITSEDGES data bit edges.
   if( syncFound )
   {
      // if not enough bits yet, no message received yet
      if( changeCount < DATABITSEDGES )
      {
         received = false;
      }
      else if( changeCount > DATABITSEDGES )
      {
        // if too many bits received then reset and start over
         received = false;
         syncFound = false;
      }
      else
      {
         received = true;
      }
   }
}


void setup()
{
   Serial.begin(9600);
   Serial.println("Started.");
   pinMode(DATAPIN, INPUT);             // data interrupt input
   pinMode(13, OUTPUT);                 // LED output
   attachInterrupt(1, handler, CHANGE);
   pinMode(SQUELCHPIN, OUTPUT);         // data squelch pin on radio module
   digitalWrite(SQUELCHPIN, HIGH);      // UN-squelch data
}

/*
 * Convert pulse durations to bits.
 * 
 * 1 bit ~0.4 msec high followed by ~0.2 msec low
 * 0 bit ~0.2 msec high followed by ~0.4 msec low
 */
int convertTimingToBit(unsigned int t0, unsigned int t1) 
{
   if( t0 > (BIT1_HIGH-100) && t0 < (BIT1_HIGH+100) && t1 > (BIT1_LOW-100) && t1 < (BIT1_LOW+100) )
   {
      return 1;
   }
   else if( t0 > (BIT0_HIGH-100) && t0 < (BIT0_HIGH+100) && t1 > (BIT0_LOW-100) && t1 < (BIT0_LOW+100) )
   {
      return 0;
   }
   return -1;  // undefined
}

/*
 * Main Loop
 * Wait for received to be true, meaning a sync stream plus
 * all of the data bit edges have been found.
 * Convert all of the pulse timings to bits and calculate
 * the results.
 */
void loop()
{
   if( received == true )
   {
      // disable interrupt to avoid new data corrupting the buffer
      detachInterrupt(1);

      // extract temperature value
      unsigned int startIndex, stopIndex, ringIndex;
      unsigned long temperature = 0;
      bool fail = false;

// Print the bit stream for debugging. 
// Generates a lot of chatter, normally disable this.
//#define DISPLAY_BIT_TIMING      
#ifdef DISPLAY_BIT_TIMING
      Serial.print("syncFound = ");
      Serial.println(syncFound);
      Serial.print("changeCount = ");
      Serial.println(changeCount);

      Serial.print("syncIndex = ");
      Serial.println(syncIndex);

      Serial.print("dataIndex = ");
      Serial.println(dataIndex);

      ringIndex = (syncIndex - (SYNCPULSEEDGES-1))%RING_BUFFER_SIZE;

      for( int i = 0; i < (SYNCPULSECNT+DATABITSCNT); i++ )
      {
         int bit = convertTimingToBit( pulseDurations[ringIndex%RING_BUFFER_SIZE], 
                                       pulseDurations[(ringIndex+1)%RING_BUFFER_SIZE] );

         Serial.print("bit ");
         Serial.print( i );
         Serial.print(" = ");
         Serial.print(bit);
         Serial.print(" t1 = ");
         Serial.print(pulseDurations[ringIndex%RING_BUFFER_SIZE]);
         Serial.print(" t2 = ");
         Serial.println(pulseDurations[(ringIndex+1)%RING_BUFFER_SIZE]);

         ringIndex += 2;
      }
#endif // DISPLAY_BIT_TIMING

// Display the raw data received in hex
#define DISPLAY_DATA_BYTES
#ifdef DISPLAY_DATA_BYTES
      unsigned char dataBytes[DATABYTESCNT];
      fail = false; // reset bit decode error flag

      // clear the data bytes array
      for( int i = 0; i < DATABYTESCNT; i++ )
      {
        dataBytes[i] = 0;
      }
        
      ringIndex = (syncIndex+1)%RING_BUFFER_SIZE;

      for( int i = 0; i < DATABITSCNT; i++ )
      {
         int bit = convertTimingToBit( pulseDurations[ringIndex%RING_BUFFER_SIZE], 
                                       pulseDurations[(ringIndex+1)%RING_BUFFER_SIZE] );
                                       
         if( bit < 0 )
         {  
            fail = true;
            break;      // exit loop
         }
         else
         {
            dataBytes[i/8] |= bit << (7-(i%8));
         }
         
         ringIndex += 2;
      }

      if( fail )
      {
         Serial.println("Data Byte Display : Decoding error.");
      }
      else
      {
              for( int i = 0; i < DATABYTESCNT; i++ )
              {
                PrintHex8(&dataBytes[i], 1);
                //Serial.print(dataBytes[i], HEX);
                Serial.print(",");
              }
              //Serial.println();
      
              for( int i = 0; i < DATABYTESCNT; i++ )
              {
                Serial.print(dataBytes[i], BIN);
                Serial.print(",");
              }
              //Serial.println();
      }

#endif  

      // extract temperature value
      unsigned long temp = 0;
      fail = false;
      // most significant 4 bits
      startIndex = (dataIndex + (4*8+4)*2) % RING_BUFFER_SIZE;
      stopIndex  = (dataIndex + (4*8+8)*2) % RING_BUFFER_SIZE;
      for( int i = startIndex; i != stopIndex; i = (i+2)%RING_BUFFER_SIZE )
      {
         int bit = convertTimingToBit(pulseDurations[i], pulseDurations[(i+1)%RING_BUFFER_SIZE]);
         temp = (temp<<1) + bit;
         if( bit < 0 )  fail = true;
      }
      // least significant 7 bits
      startIndex = (dataIndex + (5*8+1)*2) % RING_BUFFER_SIZE;
      stopIndex  = (dataIndex + (5*8+8)*2) % RING_BUFFER_SIZE;
      for( int i=startIndex; i!=stopIndex; i=(i+2)%RING_BUFFER_SIZE )
      {
         int bit = convertTimingToBit( pulseDurations[i%RING_BUFFER_SIZE], 
                                       pulseDurations[(i+1)%RING_BUFFER_SIZE] );
         temp = ( temp << 1 ) + bit; // shift and insert next bit
         if( bit < 0 )  fail = true;
      }    

      if( fail )
      {
         Serial.println("Decoding error.");
      }
      else
      {
#ifdef VERBOSE_OUTPUT      
         Serial.print("Temperature: ");
         Serial.print((int)((temp-1024)/10+2.4+0.5));  // round to the nearest integer
         //Serial.write(176);    // degree symbol
         Serial.print("C/");
         Serial.print((int)(((temp-1024)/10+2.4+0.5)*9/5+32));  // convert to F
         //Serial.write(176);    // degree symbol
         Serial.print("F at ");
         Serial.print( millis() );
         Serial.println(" msec");
#else
         Serial.print((int)((temp-1024)/10+2.4+0.5));  // round to the nearest integer
         Serial.print(",");
         Serial.print((int)(((temp-1024)/10+2.4+0.5)*9/5+32));  // convert to F
         Serial.print(",");
         Serial.print( millis() );
         Serial.println();
#endif                 
      } 
      // delay for 1 second to avoid repetitions
      delay(1000);
      received = false;
      syncFound = false;

      // re-enable interrupt
      attachInterrupt(1, handler, CHANGE);
   }
}


