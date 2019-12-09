#!/usr/bin/python3

import mtca4u
import time

mtca4u.set_dmap_location('example2.dmap')
d=mtca4u.Device('oven')

#cooling rate c
c=0.001 #deg /(deg*s)

#heating rate h
h=0.0003 #deg/(A*s)

ovenTemp = 25.
environment = 25.

while True:
    I=d.read('heater','heatingCurrent')[0]
    tempChange = 1 * (I*h +                  (environment-ovenTemp)*c)
            #   1s   current*heating rate    deltaT * cooling rate

    ovenTemp = ovenTemp + tempChange
    d.write('heater','temperatureReadback.DUMMY_WRITEABLE',ovenTemp)
    print('change ' + str(tempChange) +', new temp ' +str(ovenTemp))

    time.sleep(1)
