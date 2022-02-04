/*
This sketch is made for arduino to control TCMuxV5 shields to create an interlock for high current supplies by checking
temperatures of coils and monitoring flow of cooling water
In V1, the transistor bank monitor was added as Channel 16
// Based on TCMux shield demo by Ocean Controls
*/

// Code sends the data to a serial terminal and displays readings on an LCD screen
// Type @NS, 1 to set the number of sensor to 1                            Note: This functionality is from an old build but should not be used with the interlock
// Type @NS, 8 to set the number of sensors to 8                           Note: This functionality is from an old build but should not be used with the interlock
// Type @STog or @toggle to toggle between all sensors and active sensors for print statements
// Type @UD, 1 to set the update delay to 1 second (default is 0 second delay, can set up to 60 second delay)
// Type @SV to save the number of sensors and update delay variables to EEPROM



#include <string.h>                          //Use the string Library
#include <ctype.h>                           
#include <EEPROM.h>                          //EEPROM for memory
#include <LiquidCrystal.h>                   //LiquidCrystal for LCD display

//#define SHOWMEYOURBITS // Display the raw 32bit binary data from the MAX31855      - For debug only

#define PINEN 7       //Mux Enable pin
#define PINA0 4       //Mux Address 0 pin
#define PINA1 5       //Mux Address 1 pin
#define PINA2 6       //Mux Address 2 pin
#define PINSO 12      //TCAmp Slave Out pin (MISO)
#define PINSC 13      //TCAmp Serial Clock (SCK)
#define PINCS 8       //TCAmp Chip Select Link for Shield 1
#define PINCS2 9      //TCAmp Chip Select Link for Shield 2


//-----------------------------------------------------------------------------------------------------------------------------

//defines the variables for serial input

const byte numChars = 32;                       //defines the maximum length of a response
char recvChars[numChars];                       //an array to store the received data
char tempChars[numChars];                       //another array to store the received data

char variableName[numChars] = {0};              //defines an array that stores a char string of the named variable

boolean newData = false;                        //signals if there is new data on the serial input (initially no new data, so false)

//----------------------------------------------------------------------------------------------------------------------------

//variable to toggle between all channels and active channels (i.e. channels without open circuit error)
boolean sensorToggle = false;  

//----------------------------------------------------------------------------------------------------------------------------

//defines the variables for using the LCD display
float temp = 21.2;
float tempRand = 0;
float Time = 0;
float lastTime = 0;

//This sets a custom character for the degrees symbol on the LCD display
byte customChar0[8] = {
  0b00100,
  0b01010,
  0b00100,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

int k = 0;

// initializes the library with the numbers of the interface pins
LiquidCrystal lcd(A0, A1, A2, A3, A4, A5);


//----------------------------------------------------------------------------------------------------------------------------

//defines the variables for the interlock

boolean intlockTrigger = false;            //will toggle whether the interlock is tripped
boolean checkTrigger = false;

int intlockSupply = 11;                   // defines interlock output to power supply as pin D11
int flowmeter = 10;                       // defines flowmeter as pin D10    

float tempCheck = 0.0;                    // a value that is checked in the interlock() function
int flowCheck = 1;                        // the digital read output of the flowmeter
int flowLast = 0;                         // a backup check to ensure there is no noise in the flow system before restart
int flowLastLast = 0;                     // a second backup check to ensure there is no noise in the flow system before restart
int enableCheck = 0;

int pushButton = 2;                       //defines pin D2 as the pushbutton signal
int enable = 3;                           //defines pin D3 as the enable input of the interlock 
 
//----------------------------------------------------------------------------------------------------------------------------

//Defines variables for the TCMUX shields
int Temp[16], SensorFail[16];                         //stores the temperature readings and error messages,respectively, for each channel during one full loop
float floatTemp, floatInternalTemp;                   //provides a variable to serve as a floating temperature for printouts and monitoring, same for internal chip temp.
char failMode[16];                                    //
int internalTemp, intTempFrac;
int internalTemp2, intTempFrac2;
unsigned int Mask;
char i, j, x, NumSensors = 16, UpdateDelay;
int Param;     
unsigned long time;

//----------------------------------------------------------------------------------------------------------------------------

//the initial setup script that will begin serial communication, recall values from non-volatile memory, and set and write to arduino pins
void setup()   
{     
  Serial.begin(9600);                     //Begins serial communication
  Serial.println("TCMUXV3");
  if (EEPROM.read(511)==1)                // checks for EEPROm values, and if they exist, writes them to set Number of Sensors, delay, and toggle, respectively
  {
    NumSensors = EEPROM.read(0);          //
    UpdateDelay = EEPROM.read(1);         // sets UpdateDelay (speed of print statements) from EEPROM
    sensorToggle = EEPROM.read(2);        // adds a toggle to switch between all sensors or active sensors
  }
  pinMode(PINEN, OUTPUT);     
  pinMode(PINA0, OUTPUT);    
  pinMode(PINA1, OUTPUT);    
  pinMode(PINA2, OUTPUT);    
  pinMode(PINSO, INPUT);    
  pinMode(PINCS, OUTPUT);   
  pinMode(PINCS2, OUTPUT);    
  pinMode(PINSC, OUTPUT);    
  
  pinMode(intlockSupply, OUTPUT);
  pinMode(flowmeter, INPUT);
  pinMode(pushButton, OUTPUT);
  pinMode(enable, INPUT);
  
  digitalWrite(intlockSupply, HIGH);
  digitalWrite(PINEN, HIGH);   // enable on
  digitalWrite(PINA0, LOW); // low, low, low = channel 1
  digitalWrite(PINA1, LOW); 
  digitalWrite(PINA2, LOW); 
  digitalWrite(PINSC, LOW); //put clock in low
  


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  
  // create a new custom character
  lcd.createChar(0, customChar0); 

}

void loop()                     
{
  if (millis() > (time + ((unsigned int)UpdateDelay*1000)))
  {
    time = millis();

    if (j<(NumSensors-1)) j++;
    else j=0;
      
    switch (j) //select channel   - gives pin assignments for each channel of communication with the shields and switches between the channels    ********
      {
        case 0:
          digitalWrite(PINA0, LOW); 
          digitalWrite(PINA1, LOW); 
          digitalWrite(PINA2, LOW);
        break;
        case 1:
          digitalWrite(PINA0, HIGH); 
          digitalWrite(PINA1, LOW); 
          digitalWrite(PINA2, LOW);
        break;
        case 2:
          digitalWrite(PINA0, LOW); 
          digitalWrite(PINA1, HIGH); 
          digitalWrite(PINA2, LOW);
        break;
        case 3:
          digitalWrite(PINA0, HIGH); 
          digitalWrite(PINA1, HIGH); 
          digitalWrite(PINA2, LOW);
        break;
        case 4:
          digitalWrite(PINA0, LOW); 
          digitalWrite(PINA1, LOW); 
          digitalWrite(PINA2, HIGH);
        break;
        case 5:
          digitalWrite(PINA0, HIGH); 
          digitalWrite(PINA1, LOW); 
          digitalWrite(PINA2, HIGH);
        break;
        case 6:
          digitalWrite(PINA0, LOW); 
          digitalWrite(PINA1, HIGH); 
          digitalWrite(PINA2, HIGH);
        break;
        case 7:
          digitalWrite(PINA0, HIGH); 
          digitalWrite(PINA1, HIGH); 
          digitalWrite(PINA2, HIGH);
        break;
        case 8:
          digitalWrite(PINA0, LOW); 
          digitalWrite(PINA1, LOW); 
          digitalWrite(PINA2, LOW);
        break;
        case 9:
          digitalWrite(PINA0, HIGH); 
          digitalWrite(PINA1, LOW); 
          digitalWrite(PINA2, LOW);
        break;
        case 10:
          digitalWrite(PINA0, LOW); 
          digitalWrite(PINA1, HIGH); 
          digitalWrite(PINA2, LOW);
        break;
        case 11:
          digitalWrite(PINA0, HIGH); 
          digitalWrite(PINA1, HIGH); 
          digitalWrite(PINA2, LOW);
        break;
        case 12:
          digitalWrite(PINA0, LOW); 
          digitalWrite(PINA1, LOW); 
          digitalWrite(PINA2, HIGH);
        break;
        case 13:
          digitalWrite(PINA0, HIGH); 
          digitalWrite(PINA1, LOW); 
          digitalWrite(PINA2, HIGH);
        break;
        case 14:
          digitalWrite(PINA0, LOW); 
          digitalWrite(PINA1, HIGH); 
          digitalWrite(PINA2, HIGH);
        break;
        case 15:
          digitalWrite(PINA0, HIGH); 
          digitalWrite(PINA1, HIGH); 
          digitalWrite(PINA2, HIGH);
        break;
      }
      if (j<5){    //communication with shield 1 for channels 1-5
        digitalWrite(PINCS2, HIGH);     //begin conversion
        delay(5);
        digitalWrite(PINCS, LOW);     //stop conversion
        delay(5);
        digitalWrite(PINCS, HIGH);    //begin conversion
        delay(100);     //wait 100 ms for conversion to complete
        digitalWrite(PINCS, LOW);     //stop conversion, start serial interface
        delay(1);
      } else if (j>4 && j<8){               //skips communication with channels 5, 6, 7
        return;
      } else if (j>7 && j<13){    //communication with shield 2 for channels 9-13
        digitalWrite(PINCS, HIGH); //begin conversion
        delay(5);
        digitalWrite(PINCS2, LOW); //stop conversion
        delay(5);
        digitalWrite(PINCS2, HIGH); //begin conversion
        delay(100);  //wait 100 ms for conversion to complete
        digitalWrite(PINCS2, LOW); //stop conversion, start serial interface
        delay(1);
      } else if (j>12 && j<15){             //skips communication with channels 13, 14
        return;
      } else if (j>14){     //communication with shield 2 for channels 16
        digitalWrite(PINCS, HIGH); //begin conversion
        delay(5);
        digitalWrite(PINCS2, LOW); //stop conversion
        delay(5);
        digitalWrite(PINCS2, HIGH); //begin conversion
        delay(100);  //wait 100 ms for conversion to complete
        digitalWrite(PINCS2, LOW); //stop conversion, start serial interface
        delay(1);
      }
      
      //zeros out the prior values for the channel from the last interation
      Temp[j] = 0;                 
      failMode[j] = 0;
      SensorFail[j] = 0;
      internalTemp = 0;
      
      for (i=31;i>=0;i--) {
          //start data communication
          digitalWrite(PINSC, HIGH);
          delay(1);
          
           //print out bits, if set to show
          #ifdef SHOWMEYOURBITS
          if (digitalRead(PINSO)==1)
          {
            Serial.print("1");
          }
          else
          {
            Serial.print("0");
          }
          #endif
          
        if ((i<=31) && (i>=18))
        {
          // these 14 bits are the thermocouple temperature data
          // bit 31 sign
          // bit 30 MSB = 2^10
          // bit 18 LSB = 2^-2 (0.25 degC)
          
          Mask = 1<<(i-18);
          if (digitalRead(PINSO)==1)
          {
            if (i == 31)
            {
              Temp[j] += (0b11<<14);//pad the temp with the bit 31 value so we can read negative values correctly
            }
            Temp[j] += Mask;
            
          }
          else
          {
           
          }
        }
        //bit 17 is reserved
        //bit 16 is sensor fault
        if (i==16)
        {
          SensorFail[j] = digitalRead(PINSO);
        }
        
        if ((i<=15) && (i>=4))
        {
          //these 12 bits are the internal temp of the chip
          //bit 15 sign
          //bit 14 MSB = 2^6
          //bit 4 LSB = 2^-4 (0.0625 degC)
          Mask = 1<<(i-4);
          if (digitalRead(PINSO)==1)
          {
            if (i == 15)
            {
              internalTemp += (0b1111<<12);  // pad the temp with the bit 31 value so we can read negative values correctly
            }
            
            internalTemp += Mask;  // should probably pad the temp with the bit 15 value so we can read negative values correctly
            
          }
          else
          {
      
          }
          
        }
        //bit 3 is reserved
        if (i==2)
        {
          failMode[j] += digitalRead(PINSO)<<2;//bit 2 is set if shorted to VCC
        }
        if (i==1)
        {
          failMode[j] += digitalRead(PINSO)<<1;//bit 1 is set if shorted to GND
        }
        if (i==0)
        {
          failMode[j] += digitalRead(PINSO)<<0;//bit 0 is set if open circuit
        }
        
        //stop communication for data
        digitalWrite(PINSC, LOW);
        delay(1);
      }
      
      //check if the interlock trigger is active and proceed
    if (intlockTrigger == false) {     //no interlock action needed
        
        digitalWrite(intlockSupply, HIGH);     //writes the output pin the to Agilent supplies as HIGH - meaning no interlock trigger)
        
        //a check for the sensorToggle logic from EEPROM
        if (sensorToggle == false) {   //if sensor toggle is off
          //In the serial monitor, prints the channel number and temperature reading value or error - if applicable
          Serial.print("#");                              
          Serial.print(j+1,DEC);
          Serial.print(": ");
          if (SensorFail[j] == 1)
          {    // if there is some error, ptony the reason for error
            Serial.print("FAIL");
            if ((failMode[j] & 0b0100) == 0b0100)
            {
              Serial.print(" SHORT TO VCC");
            
            //Leaving on purpose for now, 1/26/21
            /*  lcd.setCursor(0, 0);
              lcd.print("FLAG: SHORT VCC ");
              lcd.setCursor(0, 1);
              lcd.print("Check Channel ");
              lcd.print(j+1);
              lcd.print(" ");
              */
            }
            if ((failMode[j] & 0b0010) == 0b0010)
            {
              Serial.print(" SHORT TO GND");
            
            //Leaving on purpose for now, 1/26/21  
            /*  lcd.setCursor(0, 0);
              lcd.print("FLAG: SHORT GND ");
              lcd.setCursor(0, 1);
              lcd.print("Check Channel ");
              lcd.print(j+1);
              lcd.print(" ");
              */
            }
            if ((failMode[j] & 0b0001) == 0b0001)
            {
              Serial.print(" OPN CRCT");
              
            //Leaving on purpose for now, 1/26/21
            /*  lcd.setCursor(0, 0);
              lcd.print("FLAG: OPEN CRCT ");
              lcd.setCursor(0, 1);
              lcd.print("Check Channel ");
              lcd.print(j+1);
              lcd.print(" ");
              */
            }
          }
          else {
            floatTemp = (float)Temp[j] * 0.25;        //temporary value of the temperature for channel j to print
            Serial.print(floatTemp,2);
            
            Serial.print(" degC");
          }//end reading sensors
      
      
        //this sequence gives an internal temperature reading of the TCMux's chip
        Serial.print(" Int: ");
        floatInternalTemp = (float)internalTemp * 0.0625;
        Serial.print(floatInternalTemp,4);
  
        Serial.print(" degC");
        Serial.println("");
     
        delay(5); 
        
      }
      else if (sensorToggle == true) {  // if sensor toggle is active, will skip over sensors will errors
        
          if (SensorFail[j] == 1)  {
            return;
          }
          else  {
            Serial.print("#");
            Serial.print(j+1,DEC);
            Serial.print(": ");
            floatTemp = (float)Temp[j] * 0.25;
            Serial.print(floatTemp,2);
            Serial.print(" degC");
        
            //this sequence gives an internal temperature reading of the TCMux's chip
            Serial.print(" Int: ");
            floatInternalTemp = (float)internalTemp * 0.0625;
            Serial.print(floatInternalTemp,4);
            
            Serial.print(" degC");
            Serial.println("");
            delay(5);
            
            //--------------------------------------------------
        }
      }
      // prints a distinguishable line in the serial monitor after a full iteration of all channels
      if (j==(15)) {
        Serial.println("-------------------");
      }
    }
  
    //if the interlockTrigger is active, check for the conditions to reactivate the system in the function "enableSystem()"
    else if (intlockTrigger == true) {
      enableSystem();
    }
  
  // check the flow and temperature value to see if the interlock should activate
  interlockCheck();
  
  //check to see if a new command came over the serial monitor input
  recvWithEndMarker();
  parseData();
  showNewData();
  
  
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//This section of the loop deals with the printing of temperature readings on the LCD display
  Time = millis();  // set the time to the most current value for millis() - time in millisceonds
  
  lcd.setCursor(0,0);      //set the LCD cursor to the starting position
    if ( (Time - lastTime) > 1500){             //check to see if more than 1500ms has passed before printing a new temperature
      
      //use variable k to track the channel for the LCD - as the clock/timing is different for the serial monitor and LCD, k is seperate from j
      if (k<(NumSensors-1)) k++; 
      else k=0;
      
        switch (k) //select channel
        delay(5);
        
        
        if (k<5){     //print values for active channels 1-5
          lcd.setCursor(0, 0);    //set the LCD cursor to the starting position
          if (intlockTrigger == false) {    //check to see if the interlock trigger is inactive
            if (sensorToggle == false) {    //check to see if the sensor toggle command is inactive
              
              //prints to the lcd screen
              lcd.setCursor(0, 0);      //starts cursor at position of first row and first column
              lcd.print("Channel ");
              lcd.print(k+1);
              lcd.print(" :     ");
              
              lcd.setCursor(0, 1);
            
              if (SensorFail[k] == 1)          //if an error exists, print the error
              {
                lcd.print("FAIL");
                if ((failMode[k] & 0b0100) == 0b0100)
                {
                  lcd.print(" SHORT VCC  ");
                }
                else if ((failMode[k] & 0b0010) == 0b0010)
                {
                  lcd.print(" SHORT GND  ");
                }
                else if ((failMode[k] & 0b0001) == 0b0001)
                {
                  lcd.print(" OPEN CRCT  ");
                }
                else{
                  lcd.print("            ");
                }
              }
              else {             //otherwise print the temeprature reading
                floatTemp = (float)Temp[k] * 0.25;
                lcd.print(floatTemp);
                lcd.print(" ");
                lcd.write(byte(0));
                lcd.print("C        ");
              }
            lastTime = Time; 
            }
            else{   //if the sensor toggle is active, skip over displaying errors
              if (SensorFail[k] == 1) {
               k=k;
              }
              else {   //print the channel and temeprature on the LCD
                lcd.setCursor(0, 0);        //starts cursor at position of first row and first column
                lcd.print("Channel ");
                
                lcd.print(k+1);
                lcd.print(" :     ");
                
                lcd.setCursor(0, 1);
            
                floatTemp = (float)Temp[k] * 0.25;
                lcd.print(floatTemp);
                lcd.print(" ");
                lcd.write(byte(0));
                lcd.print("C        ");
                lastTime = Time;
              }
            }
          }
        }
        else if (k>4 && k<8){       //skip unconnected channels 6, 7, 8
          return;
        }
        else if (k>7 && k<13){    //print values for active channels 9-13
          lcd.setCursor(0, 0);    //set the LCD cursor to the starting position
          if (intlockTrigger == false) {    //check to see if the interlock trigger is inactive
            if (sensorToggle == false) {    //check to see if the sensor toggle command is inactive
              
              //prints to the lcd screen
              lcd.setCursor(0, 0);      //starts cursor at position of first row and first column
              lcd.print("Channel ");
              lcd.print(k+1);
              lcd.print(" :     ");
              
              lcd.setCursor(0, 1);
            
              if (SensorFail[k] == 1)          //if an error exists, print the error
              {
                lcd.print("FAIL");
                if ((failMode[k] & 0b0100) == 0b0100)
                {
                  lcd.print(" SHORT VCC  ");
                }
                else if ((failMode[k] & 0b0010) == 0b0010)
                {
                  lcd.print(" SHORT GND  ");
                }
                else if ((failMode[k] & 0b0001) == 0b0001)
                {
                  lcd.print(" OPEN CRCT  ");
                }
                else{
                  lcd.print("            ");
                }
              }
              else {             //otherwise print the temeprature reading
                floatTemp = (float)Temp[k] * 0.25;
                lcd.print(floatTemp);
                lcd.print(" ");
                lcd.write(byte(0));
                lcd.print("C        ");
              }
            lastTime = Time; 
            }
            else{   //if the sensor toggle is active, skip over displaying errors
              if (SensorFail[k] == 1) {
               k=k;
              }
              else {   //print the channel and temeprature on the LCD
                lcd.setCursor(0, 0);        //starts cursor at position of first row and first column
                lcd.print("Channel ");
                
                lcd.print(k+1);
                lcd.print(" :     ");
                
                lcd.setCursor(0, 1);
            
                floatTemp = (float)Temp[k] * 0.25;
                lcd.print(floatTemp);
                lcd.print(" ");
                lcd.write(byte(0));
                lcd.print("C        ");
                lastTime = Time;
              }
            }
          }
        }
        else if (k>12 && k<15){   //skip unconnected channels 14, 15
          return;
        }
        else if (k>14){       //print values for active channel 16
          lcd.setCursor(0, 0);    //set the LCD cursor to the starting position
          if (intlockTrigger == false) {    //check to see if the interlock trigger is inactive
            if (sensorToggle == false) {    //check to see if the sensor toggle command is inactive
              
              //prints to the lcd screen
              lcd.setCursor(0, 0);      //starts cursor at position of first row and first column
              lcd.print("Channel ");
              lcd.print(k+1);
              lcd.print(" :     ");
              
              lcd.setCursor(0, 1);
            
              if (SensorFail[k] == 1)          //if an error exists, print the error
              {
                lcd.print("FAIL");
                if ((failMode[k] & 0b0100) == 0b0100)
                {
                  lcd.print(" SHORT VCC  ");
                }
                else if ((failMode[k] & 0b0010) == 0b0010)
                {
                  lcd.print(" SHORT GND  ");
                }
                else if ((failMode[k] & 0b0001) == 0b0001)
                {
                  lcd.print(" OPEN CRCT  ");
                }
                else{
                  lcd.print("            ");
                }
              }
              else {             //otherwise print the temeprature reading
                floatTemp = (float)Temp[k] * 0.25;
                lcd.print(floatTemp);
                lcd.print(" ");
                lcd.write(byte(0));
                lcd.print("C        ");
              }
            lastTime = Time; 
            }
            else{   //if the sensor toggle is active, skip over displaying errors
              if (SensorFail[k] == 1) {
               k=k;
              }
              else {   //print the channel and temeprature on the LCD
                lcd.setCursor(0, 0);        //starts cursor at position of first row and first column
                lcd.print("Channel ");
                
                lcd.print(k+1);
                lcd.print(" :     ");
                
                lcd.setCursor(0, 1);
            
                floatTemp = (float)Temp[k] * 0.25;
                lcd.print(floatTemp);
                lcd.print(" ");
                lcd.write(byte(0));
                lcd.print("C        ");
                lastTime = Time;
              }
            }
          }
        }
        else{       //Give a serial monitor warning if the LCD script doe snot meet any of the above conditions
          Serial.println("Something is wrong in the lcd script");
        }
    }
  }
}

//------------------------------------------------------------------------------------------------------------  
 

/*
This function monitors the channels of the thermcouples / flowmeter to determine if the interlock should activate
*/
void interlockCheck() {
  if (x<(NumSensors-1)) x++;              //switch between different channels - seperate from j since this is a standalone function
  else x=0;
  
  float tempCheck = (float)Temp[x] * 0.25;                  //set tempCheck to the value for channel x from the Temp[] array
  int flowCheck = digitalRead(flowmeter) ;                  //set flowCheck to the binary value of the flowmeter reading
  if (SensorFail[x] == 1){       //if the sensor has an error that is not temperature related, omit it from monitoring the interlock
    x=x;
  }  
  else {
    if (intlockTrigger == true) {            //if the interlock is already triggered, don't change anything
      x=x;
    }
    else{
      if (x<15){     //interlock check for channels 1-15 (thermocouples on the coils)
        if (tempCheck > 24.0) {   //check if the temperature value for channel x is above 24.0 degC and if so, activate interlock
            
            intlockTrigger = true;                 //set interlock trigger boolean to true
            digitalWrite(intlockSupply, LOW);         //stop providing interlock supply (set to 0V so interlock triggers at power supply)
            
            //print a warning message that the interlock was triggered to the serial monitor
            Serial.println("");
            Serial.print("The temperature for Channel ");
            Serial.print(x+1);
            Serial.println(" is too high!");
            
            //print a warning message that the interlock was triggered on the LCD with channel number
            lcd.setCursor(0, 0);
            lcd.print("!*INTERLOCK ON*!");
            
            lcd.setCursor(0, 1);
        
            lcd.print("Chan");
            lcd.print(x+1);
            lcd.print(" HIGH TEMP!");
            
            //wait 1500 milliseconds 
            delay(1500);
          }
      }
      else if (x=15){   //interlock check for channel 16 (thermocouple on the transistors)
        if (tempCheck > 80.0) {    //check if the temperature value for the transistor is above 80.0 degC and if so, activate interlock
            
            intlockTrigger = true;                 //set interlock trigger boolean to true
            digitalWrite(intlockSupply, LOW);         //stop providing interlock supply (set to 0V so interlock triggers at power supply)
            
            //print a warning message that the interlock was triggered to the serial monitor
            Serial.println("");
            Serial.print("The temperature for Channel ");
            Serial.print(x+1);
            Serial.println(" is too high!");
            
            //print a warning message that the interlock was triggered on the LCD with channel number
            lcd.setCursor(0, 0);
            lcd.print("!*INTERLOCK ON*!");
            
            lcd.setCursor(0, 1);
        
            lcd.print("Chan");
            lcd.print(x+1);
            lcd.print(" HIGH TEMP!");
            
            //wait 1500 milliseconds 
            delay(1500);
          }
      }
      else if (flowCheck == 1) {//check if the flowmeter returns a high (1) value and if so, activate interlock
          
          intlockTrigger = true;                 //set interlock trigger boolean to true
          digitalWrite(intlockSupply, LOW);         //stop providing interlock supply (set to 0V so interlock triggers at power supply)
          
           //print a warning message to the serial monitor that the interlock was triggered due to insufficient at flowmeter  
          Serial.println("");
          Serial.println("No water flow detected!");
          
           //print a warning message to the LCD screen that the interlock was triggered due to insufficient at flowmeter
          lcd.setCursor(0, 0);
          lcd.print("!*INTERLOCK ON*!");
          
          lcd.setCursor(0, 1);
      
          lcd.print("NO COOLANT FLOW!");
          
          //wait 1500 milliseconds 
          delay(1500);
        }
      else { //if no interlock conditions are met, no activation is required
          x=x; //Note: a seperate function reactivates the system
        }
    }
      
  }
 }


//------------------------------------------------------------------------------------------------------------------------ 


/*
This function monitors the channels of the thermcouples / flowmeter to determine if the interlock should activate
//This is meant to run only once the interlock trigger has been set to true (shown above with an if statement)
*/
void enableSystem() {
  
    //digitalWrite(pushButton, HIGH);
  
    checkTrigger = true;     //default checkTrigger to true - checkTrigger will determine if the system can be reactivated
    
    int flowCheck = digitalRead(flowmeter);           //set flowCheck to the binary value of the flowmeter reading
    delay(50);
    Serial.print("flowCheck value is ");
    Serial.println(flowCheck);                //print the flowcheck value to the serial monitor to determine if water flow is occurring
    
    
    for (int y = 0; y<16; y++) {               //another new interative channel variable for another new function
      float tempCheck = (float)Temp[y] * 0.25;                //tempCheck becomes the value of the latest temperature stored for channel y 
      if (SensorFail[y] == 1){             //skip channel y if an error with thermocuple reading occurs
        y=y;
      }
      else if (tempCheck > 26) {         //if the temperature for channel y remains high, continue with the interlock triggered
      
        intlockTrigger = true;                    //keep interlock trigger true
        digitalWrite(intlockSupply, LOW);                //keep interlock supply output at 0V
        
        checkTrigger = false;               //set checkTrigger to false so the enable button cannot turn off the interlocking
        
        //reprint warning message for channel y
        Serial.println("");
        Serial.print("The temperature for Channel ");
        Serial.print(y+1);
        Serial.println(" is too high!");
        
        lcd.setCursor(0, 0);
        lcd.print("!*INTERLOCK ON*!");
        
        lcd.setCursor(0, 1);
    
        lcd.print("Chan");
        lcd.print(y+1);
        lcd.print(" HIGH TEMP!");
        
        delay(1000);
        
      }
    }
    if (flowCheck == 1) { //if the flow remains insufficient (return value 1), continue with the interlock triggered
      
        intlockTrigger = true;                    //keep interlock trigger true
        digitalWrite(intlockSupply, LOW);                //keep interlock supply output at 0V
        
        checkTrigger = false;               //set checkTrigger to false so the enable button cannot turn off the interlocking
        
        //reprint warning message for flow
        Serial.println("");
        Serial.println("No water flow detected!");
      
        lcd.setCursor(0, 0);
        lcd.print("!*INTERLOCK ON*!");
      
        lcd.setCursor(0, 1);
  
        lcd.print("NO COOLANT FLOW!");
      
        delay(1000);
      }
    else if (flowLast + flowLastLast != 0) { //check to verify that the flow value has been low for the passed two interations 
    //Note: This occurs after the current interation flow check, giving an effective range of 3 flow interations to be checked
        
        intlockTrigger = true;                    //keep interlock trigger true
        digitalWrite(intlockSupply, LOW);                //keep interlock supply output at 0V
        
        checkTrigger = false;               //set checkTrigger to false so the enable button cannot turn off the interlocking
        
        //reprint warning message for flow
        Serial.println("");
        Serial.println("No water flow detected!");
      
        lcd.setCursor(0, 0);
        lcd.print("!*INTERLOCK ON*!");
      
        lcd.setCursor(0, 1);
  
        lcd.print("NO COOLANT FLOW!");
      
        delay(1000);
    }
    //update the new values for the last two iterations
    flowLastLast = flowLast;
    flowLast = flowCheck;
    
    if (checkTrigger == true){      //see if checkTrigger is true, and if so allow system to be reactivated if the enable button is pressed
   
      digitalWrite(pushButton, HIGH);  //write pushButton to High (allows enable if button is physically pressed)
      delay(500);
      int enableCheck = digitalRead(enable);    //save enableCheck as the digital value of the enable pin
      if (enableCheck == 1){       //if enable value is HIGH (meaning button was physically pressed), reactivate the system
        Serial.println("System reactiviated!");
       
       //print reactivation message, with 2500 milliseconds of delay
        lcd.setCursor(0, 0);
        lcd.print("Reactivating....");
        lcd.setCursor(0, 1);
        lcd.print("                ");
        
        delay(2500);
        
        //actually reactivate system
        intlockTrigger = false;        //set interlock trigger to false so system can check resume normal operation
        digitalWrite(enable, LOW);      // resume 0V output to power supplies so current can be provided once more
        
        //print message that the system is now active
        lcd.setCursor(0, 0);
        lcd.print("Reactivating....");
        lcd.setCursor(0, 1);
        lcd.print(" System is live ");
        delay(1000);
        
        //stop providing the pushbutton voltage - turns off LED
        digitalWrite(pushButton, LOW);
      }
      else {         //print a message that the system is ready to go live again, once the button is pressed and enable triggered
        Serial.println("");
        Serial.println("Must push enable button to reactivate!");
        
        lcd.setCursor(0, 0);
        lcd.print("To Reactivate : ");
        lcd.setCursor(0, 1);
        lcd.print("PRESS RED BUTTON");
        
        delay(1500);
      }
     }
    
 }

//------------------------------------------------------------------------------------------------------------  

/*
This function searches for new data received by the serial monitor, and if it exists, it stores the data in a char array
*/
void recvWithEndMarker() {
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;
           while (Serial.available() > 0 && newData == false) {     
    rc = Serial.read();

    if (rc != endMarker) {
      recvChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    }
    else {
      recvChars[ndx] = '\0'; // terminates the string
      ndx = 0;
      newData = true;
    }
 }
}

//------------------------------------------------------------------------------------------------------------------------  


/*
This function splits the input data into a variable and value that can then be used to rewrite the variable setpoint
*/
void parseData() {
    //will only operate if there is new input
    if (newData == true) { 
      char * strtokIndx;                              // this is used by strtok() as an index
    
      strtokIndx = strtok(recvChars,",");             // this grabs the first part - the string for the variable name
      strcpy(variableName, strtokIndx);               // this copies the string to variableName
//      Serial.println(variableName);                   //for testing purposes, prints the string that was added to variableName
    
      strtokIndx = strtok(NULL, ",");                 // this continues from the "," to find a numbeer
      Param = atoi(strtokIndx);               // this converts the remaining part to an integer
    }
  
}
 
//---------------------------------------------------------------------------------------------------- 
 
 /*
This function searches for new data, and if it exists, will alert the user and change the variable value or give formatting 
templates if the given input is given improperly
*/
void showNewData() {
    if (newData == true) {
      //This sets the temperature setpoint to the user provided value from the serial input
      if (strcmp(variableName, "@NS") == 0 ) {
        Serial.println("");
        Serial.print("Recieved new number of sensors: ");
        Serial.println(Param);
        //'Print "command was ON"
        if ((Param <= 16) && (Param > 0)) {
          NumSensors = Param;
          Serial.println("New number of sensors accepted");
          delay(500);
        }
        else{
          Serial.println("Number of sensors out of range: 0 to 16");
          delay(500);
        }           
      }
      //This sets the reference voltage setpoint to the user provided value from the serial input
      else if (strcmp(variableName, "@UD") == 0) {
        //'Print "command was ON"
        if ((Param <= 60) && (Param >= 0)) {
          UpdateDelay = Param;
          Serial.println("New update delay accepted");
          delay(500);
        }
        else{
          Serial.println("Update delay value out of range: 0 to 60");
          delay(500);
        }
      }
      //This sets the reference voltage setpoint to the user provided value from the serial input
      else if (strcmp(variableName, "@SV") == 0) {
        EEPROM.write(0,NumSensors);
        EEPROM.write(1,UpdateDelay);
        EEPROM.write(2, sensorToggle);
        EEPROM.write(511,1);
        Serial.println("New NumSensors and UpdateDelay saved to memory");
        delay(500);
      }
      //This sets the reference voltage setpoint to the user provided value from the serial input
      else if (strcmp(variableName, "@STog") == 0 || strcmp(variableName, "@toggle") == 0 ) {
        if (sensorToggle == false) {
          sensorToggle = true;
          Serial.println("Sensor Toggle On: Only active sensors shown");
          delay(500);
        }
        else if (sensorToggle == true) {
          sensorToggle = false;
          Serial.println("Sensor Toggle Off: All sensors shown");
          delay(500);
        }
        delay(500);
      }
      //This gives a message to the user if the serial input was unable to be used for setting a value
      else {
        Serial.println("");
        Serial.println("The recieved data was ");
        Serial.println(recvChars);
        Serial.println("");
        Serial.println("This is NOT formatted properly. Please send one of the following:");
        Serial.println("@NS, [VALUE]");
        Serial.println("@UD, [VALUE]");
        Serial.println("@SV");
        
        delay(500);              // give the user time to read the message
      }
    variableName[numChars] = {0};
      newData = false;
    }
} 
      

      
      
      


