
#include <LiquidCrystal.h>
#include <SD.h>
#include <EEPROM.h> 
#include "TinyGPS.h"
#include <SoftwareSerial.h>
#include <Bounce2.h>
#include <string.h>

//DEFINE PINS

//GPS
#define RXPIN 3
#define TXPIN 2

//BATTERIES VOLTS
#define VOLTS 14

//BUTTONS
#define BUT_ACTIVATE 17
#define BUT_BIT0 16
#define BUT_BIT1 15

//SD CS
#define SD_CS_PIN 10

//COEF FOR BATTERIES LIFE
#define COEF3 -0.000197746
#define COEF2 0.00617072
#define COEF1 -0.0713791
#define COEF0 1.53935

//NB OF MENU
#define MAX_MENU_BASE 3 //NB_MENU -1
#define MAX_MENU_1 3 //NB_MENU -1
#define MAX_MENU_2 1 //NB_MENU -1
#define MAX_MENU_21 2 //NB_MENU -1
#define MAX_MENU_3 0 //NB_MENU -1

//REFRESH TIME
#define REFRESH 1000 //ms
#define REFRESH_DATA 2000 //ms

//ENUM FOR BUTTONS
enum BUTTON
{
	SW1, SW2, SW3, SW4, NONE
};

//STRUCT FOR 
struct Trajet
{
	float dist_tot; // cm
	unsigned long time; // ms
	//speed calculate with vars above
};

LiquidCrystal lcd(4,5,6,7,8,9);
TinyGPS gps;

SoftwareSerial nss(RXPIN, TXPIN);

Bounce debounce = Bounce();

//VARS FOR RUNNING THE CODE
uint8_t menu = 0, last_menu = 1, get_course = 0, nb_course;
bool last_but_active = false, save_course = false, last_save_course = false;

float last_lat = 0, last_lon = 0, last_alt = 0, course_dist;
unsigned long last_refresh, last_data_time;
int sats = 0;
Trajet course;

/*
FUNCTION: 	Print nb of courses on LCD screen
return: 	NONE
*/
void printNbCourse(){

	lcd.setCursor(0,1);
	if(nb_course == 0) {
		lcd.print("NO COURSES");
	} else {
		// (get_course + 1):To show to the user course nb 1 to X
		lcd.print(get_course + 1);
	}
}
/*
FUNCTION: 	Check a rising edge on a button
return: 	ENUM of the button pressed
*/
inline enum BUTTON checkButton(){
	//check if any button is press and is a rising edge
	bool cur = debounce.read();
	if(cur && !last_but_active) {
		last_but_active = cur;

		//detect wich button
		bool but0 = digitalRead(BUT_BIT0);
		bool but1 = digitalRead(BUT_BIT1);
		if (but0 && but1){ //sw4
			return SW4;
		} else if (!but0 && but1) { //sw3
			return SW3;
		} else if (but0 && !but1) { //sw2
			return SW2;
		} else { //sw1
			return SW1;
		}
	}
	last_but_active = cur;
	return NONE;
}

/*
FUNCTION:	Do the correct action when the a button is pressed
			in a given menu
return: 	NONE
*/ 
inline void processButton(){  
	//find if button is pressed
	enum BUTTON but = checkButton();

	//process button
	if(but != BUTTON::NONE){

		if(but == BUTTON::SW1){
			menu--;
		}else if(but == BUTTON::SW2){
			if (menu >= 10)
			{
				menu = menu / 10;
			}
		}else if(but == BUTTON::SW3){
			if (0 < menu && menu < 10)
			{
				menu = menu * 10;
			} else if(menu == 20){
				if(sats == 0){
					lcd.clear();
					lcd.setCursor(0,0);
					lcd.print("NO SATS");
					lcd.setCursor(0,1);
					lcd.print("AVAILAB");
					save_course = false;
					// hold screen for REFRESH sec
					last_refresh = millis() + REFRESH;	
				} else {
					save_course = !save_course;
					// force refresh
					last_refresh -= REFRESH;	
				}

			} else if(menu == 30 || menu == 21){
				get_course = (get_course + 1) % nb_course;
				// force refresh
				last_refresh -= REFRESH;
			}
			
		// }else if(but == BUTTON::SW4){
			} else {
			if(menu == 30){
				lcd.clear();
				if(!save_course && nb_course != 0){
					lcd.clear();
					lcd.setCursor(0,0);
					lcd.print("SENDING");
					// hold screen for REFRESH sec
					last_refresh = millis() + REFRESH;
					String str = String(get_course) + ".csv";
					char __str[sizeof(str)];
	    			str.toCharArray(__str, sizeof(__str));

					File file = SD.open(__str, FILE_READ );
					Serial.println("data");
					if (file){
						while (file.available()) {
	     					Serial.write(file.read());
	    				}
	  				} 
	    			file.close();
					Serial.println("end");

				} else {
					if (save_course){
					lcd.clear();

					lcd.setCursor(0,0);
					lcd.print("STOP");
					lcd.setCursor(0,1);
					lcd.print("SAVE");
					// hold screen for REFRESH sec
					last_refresh =  millis() + REFRESH;

					} else {
					lcd.clear();
					lcd.setCursor(0,0);
					lcd.print("NO");
					lcd.setCursor(0,1);
					lcd.print("COURSES");
					// hold screen for REFRESH sec
					last_refresh = millis() + REFRESH;

					}
				}

			} else if(menu == 21){
				if (nb_course != 0)
				{
					menu = menu * 10;
				} else {
					lcd.clear();
					lcd.setCursor(0,0);
					lcd.print("NO");
					lcd.setCursor(0,1);
					lcd.print("COURSES");
					// hold screen for REFRESH sec
					last_refresh = millis() + REFRESH;
				}
			} else {
				menu++;	
			}
		}
		
	}
}

/*
FUNCTION: Show menu on the lcd screen
retun: NONE
*/
inline void LcdMenu(){
	//if new menu or need refresh
	if (menu != last_menu || (millis() - last_refresh > REFRESH && millis() - last_refresh < 120 * REFRESH)){
		lcd.clear();
		last_refresh = millis();

		for(;;){
			if(menu == 0) { // VOLT menu
				float volt = 0;
				volt = analogRead(VOLTS) * (5.0 / (1023.0));;
				lcd.setCursor(0,0);
				lcd.print("volt:");
				lcd.print(volt, 1);
				lcd.setCursor(0,1);
				lcd.print("t:");
				lcd.print(COEF3*volt*volt*volt + COEF2*volt*volt + COEF1*volt + COEF0);
				lcd.print("h");
				break;

			} else if(menu == 1) { // VALUES
				lcd.setCursor(0,0);
				lcd.print("VALUES");
				break;
			} else if(menu == 10) { // Latitude
				lcd.setCursor(0,0);
				lcd.print("lat:");
				lcd.setCursor(0,1);
				lcd.print(last_lat);
				lcd.print("deg");
				break;
			} else if(menu == 11) { // Longitude
				lcd.setCursor(0,0);
				lcd.print("lon:");
				lcd.setCursor(0,1);
				lcd.print(last_lon);
				lcd.print("deg");
				break;
			} else if(menu == 12) { // Altitude
				lcd.setCursor(0,0);
				lcd.print("alt:");
				lcd.setCursor(0,1);
				lcd.print(last_alt);
				lcd.print("m");
				break;
			} else if(menu == 13) { // nb satellites
				lcd.setCursor(0,0);
				lcd.print("sats:");
				lcd.setCursor(0,1);
				lcd.print(sats);
				break;

			} else if(menu == 2) { // COURSE menu
				lcd.setCursor(0,0);
				lcd.print("COURSE");
				break;
			} else if(menu == 20) { // Save
				lcd.setCursor(0,0);
				lcd.print("Save:");
				lcd.setCursor(0,1);
				if (save_course)
				{
					lcd.print("true");
				} else {
					lcd.print("false");
				}
				break;
			} else if(menu == 21) { // See course
				lcd.setCursor(0,0);
				lcd.print("See course:");
				printNbCourse();
				break;

			} else if(210 <= menu && menu < 213) {
				Trajet myTraj;
				EEPROM.get(sizeof(Trajet)*get_course + 1, myTraj);
				if(menu ==210){ // see dist of course
					lcd.setCursor(0,0);
					lcd.print("dis:");
					lcd.setCursor(0,1);
					lcd.print(myTraj.dist_tot/100);
					lcd.print("m");
				break;
					
				} else if(menu == 211) { // see time of course
					lcd.setCursor(0,0);
					lcd.print("time:");
					lcd.setCursor(0,1);
					lcd.print(myTraj.time/60000);
					lcd.print("mins");
					break;

				} else if(menu == 212) { // see mean speed of course
					lcd.setCursor(0,0);
					lcd.print("speed:");
					lcd.setCursor(0,1);
					lcd.print((myTraj.dist_tot/myTraj.time)*36);
					lcd.print("km/h");
					break;
				}

			} else if(menu == 3) { // SERIAL menu
				lcd.setCursor(0,0);
				lcd.print("SERIAL");
				break;
			} else if(menu == 30) { // Get course X
				lcd.setCursor(0,0);
				lcd.print("Get course:");
				printNbCourse();

				break;
			} else {
				// find good menu if not in range
				if (last_menu < 10){
					if(last_menu == 0){
						menu = MAX_MENU_BASE;
					} else {
						menu = 0;
					}
				} else {
					if (last_menu/10 == 1){
						if (last_menu == 10){
							menu = 10 + MAX_MENU_1;
						}else{
							menu = 10;
						}
					} else if (last_menu/10 == 2){
						if (last_menu == 20){
							menu = 20 + MAX_MENU_2;
						}else{
							menu = 20;
						}
					} else if (last_menu/10 == 3){
						if (last_menu == 30){
							menu = 30 + MAX_MENU_3;
						}else{
							menu = 30;
						}
					} else if (last_menu/10 == 21){
						if (last_menu == 210){
							menu = 210 + MAX_MENU_21;
						}else{
							menu = 210;
						}
					}
				}
			}
		}
		last_menu = menu;
	}
}

/*
FUNCTION:	detect rising and falling edge of the save command
		 	and save the position in the appropriate file
return: 	NONE
*/
inline void saveData(){
	if (save_course){
		if (!last_save_course && save_course ){ //rising edge
			//init course
			course.dist_tot = 0;
			course.time = millis();
		} else {
			if (millis() - last_data_time > REFRESH_DATA){
				// save current position and different data related
				// data line : lon;lat;alt;cours;speed;nbsat,hdop;date;time\n
				last_data_time = millis();
				String str = String(nb_course) + ".csv";
				char __str[sizeof(str)];
    			str.toCharArray(__str, sizeof(__str));
				File file = SD.open(__str, FILE_WRITE );
				// if file opened
				if(file){
					long lon, lat; 
					long unsigned date, tim, age;
					gps.get_position(&lat, &lon ,&age);
					gps.get_datetime(&date, &tim, &age);
					file.print(lon);
					file.print(";");
					file.print(lat);
					file.print(";");
					file.print(gps.altitude());
					file.print(";");
					file.print(gps.course());
					file.print(";");
					if(gps.speed() == gps.GPS_INVALID_SPEED ){
						file.print(gps.GPS_INVALID_SPEED);
					} else {
						file.print(gps.speed()*_GPS_KMPH_PER_KNOT);
					}
					file.print(";");
					file.print(gps.satellites());
					file.print(";");
					file.print(gps.hdop());
					file.print(";");
					file.print(date);
					file.print(";");
					file.println(tim);
				}
				file.close();
			}
		}
	} else if(last_save_course){ //falling edge
		//save course and update nb_course
		course.time = millis() - course.time;
		EEPROM.put(sizeof(Trajet)*nb_course + 1, course);
		nb_course ++;
		EEPROM.write(0,nb_course);
	}
}

/*
FUNCTION: 	Default setup arduino function
return: 	NONE
*/
void setup()
{
	//setup LCD screen
	lcd.begin(8,2);
	delay(20); //Wait for LCD screen

	//Setup serial
	Serial.begin(57600);

	//Setup SoftwareSerial
	nss.begin(4800);

	//Setup buttons
	pinMode(BUT_ACTIVATE,INPUT_PULLUP);
	pinMode(BUT_BIT0,INPUT_PULLUP);
	pinMode(BUT_BIT1,INPUT_PULLUP);
	debounce.attach(BUT_ACTIVATE);
	debounce.interval(5);

	//Setup SD
	SD.begin(SD_CS_PIN);
	
	//Read nb of course saved
	nb_course = EEPROM.read(0);
}

/*
FUNCTION: 	Default loop arduino function
return: 	NONE
*/
void loop()
{
//UPDATE PHASE
	debounce.update();	


//LOOP PHASE
	processButton();

	//if gps is connected to the arduino
	if (nss.available())
	{
		saveData();

		//if new data
		int c = nss.read();
		if (gps.encode(c)){
			//update position
			float lat, lon;
			unsigned long dt;
			sats = gps.satellites();
			gps.f_get_position(&lat, &lon, &dt);
			last_alt = gps.f_altitude();
			//if save course, add dist to dist_tot
			if(save_course){
				course.dist_tot += gps.distance_between(last_lat, last_lon, lat, lon);
			}
			last_lat = lat;
			last_lon = lon;
		}

	}
	//show menu
	LcdMenu();

//SETTING PHASE
	last_save_course = save_course;
	//delay to slow down the code to give it time to setup his variables
	delay(500);
}
