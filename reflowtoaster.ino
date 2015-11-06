#include <max6675.h>
#include <LiquidCrystal.h>
#include <Wire.h>

#define PIN_HEAT_ON  7 

#define PIN_START_BUTTON  A0
#define PIN_STOP_BUTTON   A1
#define PIN_BUZER     A2
#define PIN_NA        A3

#define PIN_LCD_RS      8
#define PIN_LCD_EN      9
#define PIN_LCD_D4      10
#define PIN_LCD_R5      11
#define PIN_LCD_R6      12
#define PIN_LCD_R7      13


#define PIN_THERMO_GND    2
#define PIN_THERMO_VSS    3

#define PIN_THERMO_DO   4
#define PIN_THERMO_CS   5
#define PIN_THERMO_CLK    6

MAX6675 thermocouple(PIN_THERMO_CLK, PIN_THERMO_CS, PIN_THERMO_DO);



LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_R5, PIN_LCD_R6, PIN_LCD_R7);




// make a cute degree symbol
//uint8_t degree[8] = { 140,146,146,140,128,128,128,128 };

enum PROFILENAME { PR_OFF, PR_PREHEAT, PR_SOAK, PR_PREFLOW, PR_FLOW, PR_COOL_DOWN };


/*

Reflow profile for Sn63/Pb37 solder assembly, designed as a starting point for process optimization.
pre HEAT
0         25
0-30  30  75    100 <2.5 sec  2.5 per sec
31-120  90  50    150 <2.5 sec  0.55 per sec SOAK

121-150 30  33    183  .9 per sec
151-210 60  52    235  .86 per sec
211-240 183


*/

double arrayprofile_Time[6] = { 0, 30, 120, 150, 210, 240 };
double  arrayprofile_TempTargate[6] = { 25, 100, 150, 183, 235, 0 };


unsigned long last_temp_checktime = 0;
double last_temp_check = 0;
unsigned long starttime = 0;
unsigned long Last_time = 0;
double Last_temp = 0;

PROFILENAME current_step = PR_OFF;
PROFILENAME last_step = PR_OFF;

void setup() {
	pinMode(PIN_HEAT_ON, OUTPUT);
	digitalWrite(PIN_HEAT_ON, LOW);

	pinMode(PIN_BUZER, OUTPUT);
	digitalWrite(PIN_BUZER, LOW);

	pinMode(PIN_START_BUTTON, INPUT_PULLUP);
	pinMode(PIN_STOP_BUTTON, INPUT_PULLUP);

	// use Arduino pins 
	pinMode(PIN_THERMO_VSS, OUTPUT);
	digitalWrite(PIN_THERMO_VSS, HIGH);
	pinMode(PIN_THERMO_GND, OUTPUT);
	digitalWrite(PIN_THERMO_GND, LOW);

	lcd.begin(16, 2);
	//  lcd.createChar(0, degree);

	// wait for MAX chip to stabilize
	delay(500);
	lcd.clear();
	lcd.print(F("OFF     "));
	lcd.print(F("Reflow Toster  1.0"));
	delay(1000);
	pinMode(PIN_BUZER, OUTPUT);
	digitalWrite(PIN_BUZER, HIGH);
	delay(500);
	digitalWrite(PIN_BUZER, LOW);

}
void heaterandshow(int offon){
	digitalWrite(PIN_HEAT_ON, offon);
	lcd.setCursor(14, 0);
	if (offon == LOW){
		lcd.print(" ");
	}
	else{
		lcd.print("H");
	}
}
void setheater(unsigned long   lcurenttime, double lcurrent_temp){



	if (lcurrent_temp > arrayprofile_TempTargate[current_step]) {
		heaterandshow(LOW);
		return;
	}
	else
	{
		// are we heating to fast ...

		if (Last_time == 0)
			Last_time = lcurenttime;
		if (Last_temp == 0)
			Last_temp = lcurrent_temp;



		if (((lcurenttime - Last_time) > 501))
		{

			float profile_max = (
				float(arrayprofile_TempTargate[current_step] - arrayprofile_TempTargate[current_step - 1]) /
				float(arrayprofile_Time[current_step] - arrayprofile_Time[current_step - 1])
				);



			float testtemp = (lcurrent_temp - Last_temp);
			float testtime = (lcurenttime - Last_time);
			float testsec = (testtime / 1000);

			if (float(testtemp / testsec) > profile_max) {
				if (Last_temp > lcurrent_temp) {
					heaterandshow(HIGH);

				}
				else{

					heaterandshow(LOW);
				}
			}
			else

			{


				heaterandshow(HIGH);

			}

			Last_temp = lcurrent_temp;
			Last_time = lcurenttime;
		}

	}

}

void loop() {
	unsigned long   curenttime = millis();
	double  cur_temp = last_temp_check;

	// check temp every 500 ms
	if (millis() > (last_temp_checktime + 500)){
		cur_temp = thermocouple.readCelsius();
		last_temp_check = cur_temp;

		last_temp_checktime = millis();

	}




	if ((digitalRead(PIN_START_BUTTON) == LOW) && (current_step == PR_OFF)) {

		starttime = millis();
		current_step = PR_PREHEAT;
		Last_temp = cur_temp;
		Last_time = curenttime;


	}
	if (digitalRead(PIN_STOP_BUTTON) == LOW) {

		current_step = PR_OFF;

		heaterandshow(LOW);


	}
	float   totalrunseconds = 0;

	if (current_step != PR_OFF)
	{
		totalrunseconds = float(float(curenttime - starttime) / 1000);
	}




	/*

	Reflow profile for Sn63/Pb37 solder assembly, designed as a starting point for process optimization.
	pre HEAT
	0         25
	0-30  30  75    100 <2.5 sec  2.5 per sec
	31-120  90  50    150 <2.5 sec  0.55 per sec SOAK

	121-150 30  33    183  .9 per sec
	151-210 60  52    235  .86 per sec
	211-240 183


	*/

	switch (current_step)
	{
	case PR_OFF:


		heaterandshow(LOW);

		break;
	case PR_PREHEAT:
		if (totalrunseconds >= arrayprofile_Time[PR_PREHEAT]) {
			current_step = PR_SOAK;

		}
		else
		{
			setheater(curenttime, cur_temp);


		}
		break;
	case PR_SOAK:
		if (totalrunseconds >= arrayprofile_Time[PR_SOAK]) {
			current_step = PR_PREFLOW;
		}
		else
		{
			setheater(curenttime, cur_temp);
		}
		break;
	case PR_PREFLOW:
		if (totalrunseconds >= arrayprofile_Time[PR_PREFLOW]) {
			current_step = PR_FLOW;
		}
		else
		{
			setheater(curenttime, cur_temp);
		}
		break;
	case PR_FLOW:
		if (totalrunseconds >= arrayprofile_Time[PR_FLOW]) {
			current_step = PR_COOL_DOWN;
		}
		else
		{
			setheater(curenttime, cur_temp);
		}
		break;
	case PR_COOL_DOWN:
		if (totalrunseconds >= arrayprofile_Time[PR_COOL_DOWN]) {

			current_step = PR_OFF;
			heaterandshow(LOW);

		}
		else
		{
			setheater(curenttime, cur_temp);
		}
		break;
	default:

		break;
	}






	/////////////////////////////////////////////// DISPLAY CANDY

	// basic readout test, just print the current temp
	if (last_step != current_step) {

		last_step = current_step;
		lcd.clear();
		lcd.setCursor(0, 0);

		switch (current_step)
		{
		case PR_OFF:
			lcd.print(F("OFF     "));
			break;
		case PR_PREHEAT:
			Last_temp = 0;
			lcd.print(F("PREHEAT "));
			break;
		case PR_SOAK:
			lcd.print(F("SOAK    "));
			break;
		case PR_PREFLOW:
			lcd.print(F("PREFLOW "));
			break;
		case PR_FLOW:
			lcd.print(F("FLOW    "));
			break;
		case PR_COOL_DOWN:
			lcd.print(F("COOL    "));
			break;
		default:
			break;
		}

		// lcd.print(F("Reflow Toster  1.0"));
	}
	if (totalrunseconds > 0) {
		lcd.setCursor(8, 0);
		lcd.print("     ");
		lcd.setCursor(8, 0);
		lcd.print(totalrunseconds, 2);
	}

	// go to line #1
	lcd.setCursor(0, 1);
	lcd.print(cur_temp);

	//  lcd.write((byte)0);

	lcd.print(F("C "));

	lcd.print(F(" T "));

	lcd.print(arrayprofile_TempTargate[current_step]);
	lcd.print(F("C "));






	// delay(500);
}
