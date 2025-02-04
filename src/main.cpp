#include <Arduino.h>
#include "PinAssignments.h"
#include <Wire.h>
#include <Adafruit_MAX31865.h>
#include <SPI.h>
#include <pt100rtd.h>

// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 thermo = Adafruit_MAX31865(29, 30, 31, 32);

// The value of the Rref resistor. Use 430.0!
#define RREF 430.0

// Like, duh. Celceius to farenheit
#define C2F(c) ((9 * c / 5) + 32)

// init the Pt100 table lookup module
pt100rtd PT100 = pt100rtd() ;

// put function declarations here:

void setup() {
  Serial.begin(115200);
  pinMode(TEMP, INPUT);
  thermo.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary

}


void checkFault(void)
{
	// Check and print any faults
	uint8_t fault = thermo.readFault();
	if (fault)
	{
		Serial.print("Fault 0x"); Serial.println(fault, HEX);
		if (fault & MAX31865_FAULT_HIGHTHRESH)
		{
			Serial.println("RTD High Threshold"); 
		}
		if (fault & MAX31865_FAULT_LOWTHRESH)
		{
			Serial.println("RTD Low Threshold"); 
		}
		if (fault & MAX31865_FAULT_REFINLOW)
		{
			Serial.println("REFIN- > 0.85 x Bias"); 
		}
		if (fault & MAX31865_FAULT_REFINHIGH)
		{
			Serial.println("REFIN- < 0.85 x Bias - FORCE- open"); 
		}
		if (fault & MAX31865_FAULT_RTDINLOW)
		{
			Serial.println("RTDIN- < 0.85 x Bias - FORCE- open"); 
		}
		if (fault & MAX31865_FAULT_OVUV)
		{
			Serial.println("Under/Over voltage"); 
		}
		thermo.clearFault();
	}
}


void loop() {
  //Custom code for k-type thermocouple on the AD8495 Amplifier board
  Serial.println("Thermocouple Sensor");

  int tempVolt = analogRead(TEMP);
  float tempCelcius = ((((tempVolt/1023.0)*3.3) - 1.25)/0.005); //Temperature = (Vout - 1.25) / 0.005 V. So for example, if the voltage is 1.5VDC, the temperature is (1.5 - 1.25) / 0.005 = 50°C
  float tempFarenheit = (((tempCelcius)*(9/5)) + 32); //(0°C × 9/5) + 32 = 32°F

  Serial.println(tempCelcius); Serial.println("C");
  //Serial.println(tempFarenheit); Serial.println("F");

  
  Serial.println();
  Serial.println();

  //Example code from the library for the MAX31865 Converter
  Serial.println("Standard RTD Temp Sensor Algorithnm ");
  uint16_t rtd, ohmsx100 ;
  rtd = thermo.readRTD();

  //Serial.print("RTD value: "); Serial.println(rtd);
  float ratio = rtd;
  ratio /= 32768;
  //Serial.print("Ratio = "); Serial.println(ratio,8);
  Serial.print("Resistance = "); Serial.println(RREF*ratio,8);
  Serial.print("Temperature = "); Serial.println(thermo.temperature(RNOMINAL, RREF));

  // Check and print any faults
  checkFault();
  Serial.println();
  Serial.println();

  //Open-source custom code from some guy for MAX31865 RTD connverter


/*************************************************** 
  This is a library for the Adafruit PT100/P1000 RTD Sensor w/MAX31865

  Designed specifically to work with the Adafruit RTD Sensor
  ----> https://www.adafruit.com/products/3328

  This sensor uses SPI to communicate, 4 pins are required to  
  interface
  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/
	uint32_t dummy ;
	float ohms, Tlut ;  
	float Tcvd, Tcube, Tpoly, Trpoly ;


	// fast integer math:
	// fits in 32 bits as long as (100 * RREF) <= 2^16,
	//  i.e., RREF must not exceed 655.35 ohms (heh).
	// TO DO: revise for 4000 ohm reference resistor needed by Pt1000 RTDs
 
	// Use uint16_t (ohms * 100) since it matches data type in lookup table.
	dummy = ((uint32_t)(rtd << 1)) * 100 * ((uint32_t) floor(RREF)) ;
	dummy >>= 16 ;
	ohmsx100 = (uint16_t) (dummy & 0xFFFF) ;

	// or use exact ohms floating point value.
	ohms = (float)(ohmsx100 / 100) + ((float)(ohmsx100 % 100) / 100.0) ;

	Serial.print("rtd: 0x") ; Serial.print(rtd,HEX) ;
	Serial.print(", ohms: ") ; Serial.println(ohms,2) ;
 
  // compare lookup table and common computational methods
  
	Tlut	= PT100.celsius(ohmsx100) ;			// NoobNote: LUT== LookUp Table
	Tcvd	= PT100.celsius_cvd(ohms) ; 		  	// Callendar-Van Dusen calc
	Tcube	= PT100.celsius_cubic(ohms) ;		  	// Cubic eqn calc
	Tpoly	= PT100.celsius_polynomial(ohms) ;      	// 5th order polynomial
	Trpoly	= PT100.celsius_rationalpolynomial(ohms) ;	// ugly rational polynomial quotient
	
	// report temperatures at 0.001C resolution to highlight methodological differences
	Serial.println("LUT== LookUp Table");
  Serial.print("Tlut   = ") ; Serial.print(Tlut  ,3) ; Serial.print(" C (exact)") ;
  Serial.println();

 	Serial.println("Callendar-Van Dusen calc");
	Serial.print("Tcvd   = ") ; Serial.print(Tcvd  ,3) ; Serial.print(" C") ;
  Serial.println();

  Serial.println("Cubic eqn calc");
	Serial.print("Tcube  = ") ; Serial.print(Tcube ,3) ; Serial.print(" C") ;
  Serial.println();

  Serial.println("5th order polynomial");
	Serial.print("Tpoly  = ") ; Serial.print(Tpoly ,3) ; Serial.print(" C") ;
  Serial.println();

  Serial.println("ugly rational polynomial quotient");
	Serial.print("Trpoly = ") ; Serial.print(Trpoly,3) ; Serial.print(" C") ;
  Serial.println();
  	Serial.println("////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////");
  
	checkFault() ;

	delay(5000) ;

  //98.91 degrees celceius at Sdelc elevation of 1101 ft

}

