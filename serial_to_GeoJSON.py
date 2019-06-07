#!/usr/bin/env python

import serial

ser = serial.Serial('/dev/ttyUSB0', 57600)

'''
FUNCTION: Check if data is comming from the serial and transform it into GeoJSON file
return: void
'''
def main():
	print ("START UP")
	while True:
		#Check if line tells data is comming
		line = ser.readline()
		if line == "data\r\n":
			#set up data file
			with open("data.geojson", "w") as file:
				file.write('''{
\t"type": "FeatureCollection",
\t"features": [
\t\t{
\t\t\t"type": "Feature",
\t\t\t"properties": {},
\t\t\t"geometry": {
\t\t\t\t"type": "LineString",
\t\t\t\t"coordinates": [''')
				text = ""
				#while data is comming
				while True:
					line = ser.readline()
					if line == "end\r\n":
						#if end setup file 
						text = text[:-1]
						text += "\n\t\t\t\t]\n\t\t\t}\n\t\t}\n\t]\n}"
						break
					#print data in data file
					array = line.split(";")
					text += "\n\t\t\t\t\t[ {}, {} ],".format(float(array[0])/1000000, float(array[1])/1000000)
				file.write(text)
				#close file
				file.close()
			#and exit
			print("END")
			break

if __name__ == '__main__':
	main()