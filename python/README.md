Goal is to capture sensor readings into an SQLite database.

Insure SQLite is installed on the raspberry pi using apt-get.

The sensor readings database is located at:

```
pi@raspberrypi:/var/www$ ll
total 8
-rw-r--r-- 1 root root  177 May 29  2016 index.html
-rw-r--r-- 1 root root 2048 May 29  2016 sensordata_A1.db
pi@raspberrypi:/var/www$
```

Use the tool sqlite3 to view and manipulate the database from the command line.

```
pi@raspberrypi:/var/www$ file sensordata_A1.db
sensordata_A1.db: SQLite 3.x database
```

```
pi@raspberrypi:/var/www$ sqlite3 sensordata_A1.db
SQLite version 3.7.13 2012-06-11 02:05:22
Enter ".help" for instructions
Enter SQL statements terminated with a ";"
sqlite> .databases
seq  name             file
---  ---------------  ----------------------------------------------------------
0    main             /var/www/sensordata_A1.db
sqlite> .tables
sensordata
sqlite> select * from sensordata;
2016-05-29|04:48:01|99|bedroom|192|0|2345
2016-05-29|04:49:46|99|bedroom|192|0|2345
sqlite> .exit
pi@raspberrypi:/var/www$
```

Eventually will be using python to read the sensor data from the Arduino and manipulate the sensor data in the database. 
Eventually will be using python and a web interface library to support a remote web interface to view the sensor data.

