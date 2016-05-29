
from pololui2c import pololuI2C
import time

i2c = pololuI2C()
i2c.set_i2c_address(0x14)

def fillsensorvals( sensortofill ):
  "function to fill sensor values"
  #print "read I2C sensor : "+str(sensortofill['id'])
  try:
    sensorread = i2c.read_unpack((sensortofill['id']-1)*8,8,'BBBBBBBB')
    sensortofill['status']=sensorread[1]
    #sensortofill['temperature']=float(sensorread[2]+sensorread[3]*256)
    sensortofill['temperature']=sensorread[2]+sensorread[3]*256
    sensortofill['timestamp']=sensorread[4]+sensorread[5]*256+sensorread[6]*256*256+sensorread[7]*256*256*256
    #print "READ OK "+str(sensortofill['id'])
    return True
  except IOError:
    #print "IOERROR: "+str(sensortofill['id'])
    pass
    return False

def printsensorvals( sensortoprint ):
  "function to print sensor values"
  print "id: "+str(sensortoprint['id'])+" zone: "+ str(sensortoprint['zone'])+" status: " + str(sensortoprint['status'])+" temperature: "+str(sensortoprint['temperature'])+" timestamp: "+str(sensortoprint['timestamp'])

sensor1={'id':1, 'zone':"girlbedroom", 'status':0, 'temperature':0, 'timestamp':0}
sensor2={'id':2, 'zone':"bararea", 'status':0, 'temperature':0, 'timestamp':0}
sensor3={'id':3, 'zone':"ausbedroom", 'status':0, 'temperature':0, 'timestamp':0}
sensor4={'id':4, 'zone':"loft", 'status':0, 'temperature':0, 'timestamp':0}
sensor5={'id':5, 'zone':"garage", 'status':0, 'temperature':0, 'timestamp':0}
sensor6={'id':6, 'zone':"outsidenorth", 'status':0, 'temperature':0, 'timestamp':0}

sensorlist=[sensor1, sensor2, sensor3, sensor4, sensor5, sensor6]

for id in range( len( sensorlist ) ):
  if( fillsensorvals( sensorlist[id] ) ):
    printsensorvals( sensorlist[id] )
  time.sleep(1)

#for id in range( len( sensorlist ) ):
#  printsensorvals( sensorlist[id] )


