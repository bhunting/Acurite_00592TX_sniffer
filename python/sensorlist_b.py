
from pololui2c import pololuI2C

i2c = pololuI2C()
i2c.set_i2c_address(0x14)

def fillsensorvals( sensortofill ):
  "function to fill sensor values"
  print "read I2C sensor : "+str(sensortofill['id'])
  sensortofill['temperature']=45

sensor1={'id':1, 'status':0, 'temperature':0, 'timestamp':0}
sensor2={'id':2, 'status':0, 'temperature':0, 'timestamp':0}
sensor3={'id':3, 'status':0, 'temperature':0, 'timestamp':0}
sensor4={'id':4, 'status':0, 'temperature':0, 'timestamp':0}
sensor5={'id':5, 'status':0, 'temperature':0, 'timestamp':0}
sensor6={'id':6, 'status':0, 'temperature':0, 'timestamp':0}

sensorlist=[sensor1, sensor2, sensor3, sensor4, sensor5, sensor6]

for id in range( len( sensorlist ) ):
  print str(sensorlist[id]['id'])+"  "+str(sensorlist[id]['temperature'])

for id in range( len( sensorlist ) ):
  fillsensorvals( sensorlist[id] )


for id in range( len( sensorlist ) ):
  print str(sensorlist[id]['id'])+"  "+str(sensorlist[id]['temperature'])


print i2c.read_unpack(0,8,'BBBBBBBB')

