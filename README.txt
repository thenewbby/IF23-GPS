

EEPROM Structure:

pos 0 : nb trajet (uint8)

pos 1 to ?? Struct trajet



Struct trajet {
	float distance_tot
	long delta_time
	//compute mean_speed
}

SD:
	file /Data/(Nb_trajet).pts

file struct:
	line: lon;lat;alt;cours;speed;nbsat,hdop;date;time\r\n
		  long;long;long;long;long;int;long;long;long
		  6;6;2;2;2;0;2;?;?



function volt to time:
-0.000197746 x^3 + 0.00617072 x^2 - 0.0713791 x + 1.53935


menu:
	0: volts
	1: position:
		10: lon
		11: lat
		12: alt
		13: nbs sat
	2: trajet:
		20: on/off
		//21: view trajet nb X
	3: serial:
		30: get trajet nb X
	//4: Param:
	//	40: point_time_delta