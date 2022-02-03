/*
This sketch is made for arduino to control TCMuxV5 shields to create an interlock for high current supplies by checking
temperatures of coils and monitoring flow of cooling water
// Based on TCMux shield demo by Ocean Controls
*/

// Sends the data to a serial terminal
// Type @NS, 1 to set the number of sensor to 1
// Type @NS, 8 to set the number of sensors to 8
// Type @STog or @toggle to toggle between all sensors and active sensors for print statements
// Type @UD, 1 to set the update delay to 1 second (default is 0 second delay, can set up to 60 second delay)
// Type @SV to save the number of sensors and update delay variables to EEPROM



#include <string.h>                          //Use the string Library
#include <ctype.h>                           
#include <EEPROM.h>                          //EEPROM for memory
#include <LiquidCrystal.h>                   //for LCD display

//#define SHOWMEYOURBITS // Display the raw 32bit binary data from the MAX31855

#define PINEN 7       //Mux Enable pin
#define PINA0 4       //Mux Address 0 pin
#define PINA1 5       //Mux Address 1 pin
#define PINA2 6       //Mux Address 2 pin
#define PINSO 12      //TCAmp Slave Out pin (MISO)
#define PINSC 13      //TCAmp Serial Clock (SCK)
#define PINCS 8       //TCAmp Chip Select Change this to match the position of the Chip Select Link
#define PINCS2 9      //TCAmp Chip Select


//-----------------------------------------------------------------------------------------------------------------------------

//defines the variables for serial input

const byte numChars = 32;                       // defines the maximum length of a response
char recvChars[numChars];                       // an array to store the received data
char tempChars[numChars];                       // an array to store the received data


//float newValue = 0;                             //the new value found in the code

char variableName[numChars] = {0};              //defines an array that stores a char string of the named variable
//int Param = 0;                      //defines the stored floating point number to be set

boolean newData = false;                        //signals if there is new data on the serial input (initially no new data, so false)

//----------------------------------------------------------------------------------------------------------------------------

//variable to toggle between all channels and active channels
boolean sensorToggle = false;  

//----------------------------------------------------------------------------------------------------------------------------

float temp = 21.2;
float tempRand = 0;
float Time = 0;
float lastTime = 0;

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

// variables for the interlock

boolean intlockTrigger = false;            //will toggle whether the interlock is tripped
boolean checkTrigger = false;

int intlockSupply = 11;                   // defines interlock output to power supply as pin D11
int flowmeter = 10;                       // defines flowmeter as pin D10    

float tempCheck = 0.0;                    // a value that is checked in the interlock() function
int flowCheck = 1;                        // the digital read output of the flowmeter
//int flowDoubleCheck = 0;
int flowLast = 0;
int flowLastLast = 0;
int enableCheck = 0;

int pushButton = 2;                       //defines pin D2 as the pushbutton signal
int enable = 3;                           //defines pin D3 as the enable input of the interlock 
 
//----------------------------------------------------------------------------------------------------------------------------


int Temp[16], SensorFail[16];
float floatTemp, floatInternalTemp;
char failMode[16];
int internalTemp, intTempFrac;
int internalTemp2, intTempFrac2;
unsigned int Mask;
char i, j, x, NumSensors = 16, UpdateDelay;
int Param;     
unsigned long time;


void setup()   
{     
  Serial.begin(9600);  
  Serial.println("TCMUXV3");
  if (EEPROM.read(511)==1)
  {
    NumSensors = EEPROM.read(0);
    UpdateDelay = EEPROM.read(1);
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
      
    switch (j) //select channel                     ********
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
      if (j<5){
        digitalWrite(PINCS2, HIGH); //begin conversion
        delay(5);
        digitalWrite(PINCS, LOW); //stop conversion
        delay(5);
        digitalWrite(PINCS, HIGH); //begin conversion
        delay(100);  //wait 100 ms for conversion to complete
        digitalWrite(PINCS, LOW); //stop conversion, start serial interface
        delay(1);
      } else if (j>4 && j<8){
        return;
      } else if (j>7 && j<13){
        digitalWrite(PINCS, HIGH); //begin conversion
        delay(5);
        digitalWrite(PINCS2, LOW); //stop conversion
        delay(5);
        digitalWrite(PINCS2, HIGH); //begin conversion
        delay(100);  //wait 100 ms for conversion to complete
        digitalWrite(PINCS2, LOW); //stop conversion, start serial interface
        delay(1);
      } else if (j>12 && j<15){
        return;
      } else if (j>14){
        digitalWrite(PINCS, HIGH); //begin conversion
        delay(5);
        digitalWrite(PINCS2, LOW); //stop conversion
        delay(5);
        digitalWrite(PINCS2, HIGH); //begin conversion
        delay(100);  //wait 100 ms for conversion to complete
        digitalWrite(PINCS2, LOW); //stop conversion, start serial interface
        delay(1);
      }
      
      Temp[j] = 0;
      failMode[j] = 0;
      SensorFail[j] = 0;
      internalTemp = 0;
      for (i=31;i>=0;i--) {
          digitalWrite(PINSC, HIGH);
          delay(1);
          
           //print out bits
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
              internalTemp += (0b1111<<12);//pad the temp with the bit 31 value so we can read negative values correctly
            }
            
            internalTemp += Mask;//should probably pad the temp with the bit 15 value so we can read negative values correctly
            
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
        
        
        digitalWrite(PINSC, LOW);
        delay(1);
      }
      
      
    if (intlockTrigger == false) {
        
        digitalWrite(intlockSupply, HIGH);
        
        if (sensorToggle == false) {
          //Serial.println(Temp,BIN);
          Serial.print("#");
          Serial.print(j+1,DEC);
          Serial.print(": ");
          if (SensorFail[j] == 1)
          {
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
            floatTemp = (float)Temp[j] * 0.25;
            Serial.print(floatTemp,2);
            
            Serial.print(" degC");
          }//end reading sensors
      
        Serial.print(" Int: ");
        floatInternalTemp = (float)internalTemp * 0.0625;
        Serial.print(floatInternalTemp,4);
  
        Serial.print(" degC");
        Serial.println("");
     
        delay(5); 
        
      }
      else if (sensorToggle == true) {
        
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
        
  
            Serial.print(" Int: ");
            floatInternalTemp = (float)internalTemp * 0.0625;
            Serial.print(floatInternalTemp,4);
            
            Serial.print(" degC");
            Serial.println("");
            delay(5);
            
            //--------------------------------------------------
        }
      }
      if (j==(15)) {
        Serial.println("-------------------");
      }
    }
  
    else if (intlockTrigger == true) {
      enableSystem();
    }
  
  interlockCheck();
  
  recvWithEndMarker();
  parseData();
  showNewData();
  
  
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Time = millis();  
  // gives a temperature value
  lcd.setCursor(0,0);
    if ( (Time - lastTime) > 1500){
      if (k<(NumSensors-1)) k++;
      else k=0;
      
        switch (k) //select channel
        delay(5);
        
        
        if (k<5){
          lcd.setCursor(0, 0);
          if (intlockTrigger == false) {
            if (sensorToggle == false) {
              //starts cursor at position of first row and first column
              lcd.setCursor(0, 0);
              lcd.print("Channel ");
              lcd.print(k+1);
              lcd.print(" :     ");
              
              lcd.setCursor(0, 1);
            
              if (SensorFail[k] == 1)
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
              else {
                floatTemp = (float)Temp[k] * 0.25;
                lcd.print(floatTemp);
                lcd.print(" ");
                lcd.write(byte(0));
                lcd.print("C        ");
              }
            lastTime = Time;
            }
            else{
              if (SensorFail[k] == 1) {
               k=k;
              }
              else {
                 //starts cursor at position of first row and first column
                lcd.setCursor(0, 0);
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
        else if (k>4 && k<8){
          return;
        }
        else if (k>7 && k<13){
          lcd.setCursor(0, 0);
          if (intlockTrigger == false) {
            if (sensorToggle == false) {
              //starts cursor at position of first row and first column
              lcd.setCursor(0, 0);
              lcd.print("Channel ");
              lcd.print(k+1);
              lcd.print(" :     ");
              
              lcd.setCursor(0, 1);
            
              if (SensorFail[k] == 1)
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
              else {
                floatTemp = (float)Temp[k] * 0.25;
                lcd.print(floatTemp);
                lcd.print(" ");
                lcd.write(byte(0));
                lcd.print("C        ");
              }
            lastTime = Time;
            }
            else{
              if (SensorFail[k] == 1) {
               k=k;
              }
              else {
                 //starts cursor at position of first row and first column
                lcd.setCursor(0, 0);
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
        else if (k>12 && k<15){
          return;
        }
        else if (k>14){
          lcd.setCursor(0, 0);
          if (intlockTrigger == false) {
            if (sensorToggle == false) {
              //starts cursor at position of first row and first column
              lcd.setCursor(0, 0);
              lcd.print("Channel ");
              lcd.print(k+1);
              lcd.print(" :     ");
              
              lcd.setCursor(0, 1);
            
              if (SensorFail[k] == 1)
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
              else {
                floatTemp = (float)Temp[k] * 0.25;
                lcd.print(floatTemp);
                lcd.print(" ");
                lcd.write(byte(0));
                lcd.print("C        ");
              }
            lastTime = Time;
            }
            else{
              if (SensorFail[k] == 1) {
               k=k;
              }
              else {
                 //starts cursor at position of first row and first column
                lcd.setCursor(0, 0);
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
        else{
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
  if (x<(NumSensors-1)) x++;
  else x=0;
  
  float tempCheck = (float)Temp[x] * 0.25;
  int flowCheck = digitalRead(flowmeter) ;
  if (SensorFail[x] == 1){
    x=x;
  }
  else {
    if (intlockTrigger == true) {
      x=x;
    }
    else{
      if (x<15){
        if (tempCheck > 24.0) {
            
            intlockTrigger = true;
            digitalWrite(intlockSupply, LOW);
            
            Serial.println("");
            Serial.print("The temperature for Channel ");
            Serial.print(x+1);
            Serial.println(" is too high!");
            
            lcd.setCursor(0, 0);
            lcd.print("!*INTERLOCK ON*!");
            
            lcd.setCursor(0, 1);
        
            lcd.print("Chan");
            lcd.print(x+1);
            lcd.print(" HIGH TEMP!");
            
            delay(1500);
          }
      }
      else if (x=15){
        if (tempCheck > 27.0) {
            
            intlockTrigger = true;
            digitalWrite(intlockSupply, LOW);
            
            Serial.println("");
            Serial.print("The temperature for Channel ");
            Serial.print(x+1);
            Serial.println(" is too high!");
            
            lcd.setCursor(0, 0);
            lcd.print("!*INTERLOCK ON*!");
            
            lcd.setCursor(0, 1);
        
            lcd.print("Chan");
            lcd.print(x+1);
            lcd.print(" HIGH TEMP!");
            
            delay(1500);
          }
      }
      else if (flowCheck == 1) {//changed to 1 for new flowcheck circuit
          
          intlockTrigger = true;
          digitalWrite(intlockSupply, LOW);
          
          Serial.println("");
          Serial.println("No water flow detected!");
          
          lcd.setCursor(0, 0);
          lcd.print("!*INTERLOCK ON*!");
          
          lcd.setCursor(0, 1);
      
          lcd.print("NO COOLANT FLOW!");
          
          delay(1500);
        }
      else {
          x=x; 
        }
    }
      
  }
 }


//------------------------------------------------------------------------------------------------------------------------ 


/*
This function monitors the channels of the thermcouples / flowmeter to determine if the interlock should activate
*/
void enableSystem() {
  
    //digitalWrite(pushButton, HIGH);
  
    checkTrigger = true;
    
    int flowCheck = digitalRead(flowmeter);
    delay(50);
    Serial.print("flowCheck value is ");
    Serial.println(flowCheck);
    
    
    for (int y = 0; y<16; y++) {
      float tempCheck = (float)Temp[y] * 0.25;
      if (SensorFail[y] == 1){
        y=y;
      }
      else if (tempCheck > 26) {
      
        intlockTrigger = true;
        digitalWrite(intlockSupply, LOW);
        
        checkTrigger = false;
        
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
    if (flowCheck == 1) { //changed flowcheck to 1 to work with new circuit
      
        intlockTrigger = true;
        digitalWrite(intlockSupply, LOW);
        
        checkTrigger = false;
      
        Serial.println("");
        Serial.println("No water flow detected!");
      
        lcd.setCursor(0, 0);
        lcd.print("!*INTERLOCK ON*!");
      
        lcd.setCursor(0, 1);
  
        lcd.print("NO COOLANT FLOW!");
      
        delay(1000);
      }
    else if (flowLast + flowLastLast != 0) { //changed to 0
        
        intlockTrigger = true;
        digitalWrite(intlockSupply, LOW);
        
        checkTrigger = false;
      
        Serial.println("");
        Serial.println("No water flow detected!");
      
        lcd.setCursor(0, 0);
        lcd.print("!*INTERLOCK ON*!");
      
        lcd.setCursor(0, 1);
  
        lcd.print("NO COOLANT FLOW!");
      
        delay(1000);
    }
    flowLastLast = flowLast;
    flowLast = flowCheck;
    
    if (checkTrigger == true){
   
      digitalWrite(pushButton, HIGH);
      delay(500);
      int enableCheck = digitalRead(enable);
      if (enableCheck == 1){
        Serial.println("System reactiviated!");
       
        lcd.setCursor(0, 0);
        lcd.print("Reactivating....");
        lcd.setCursor(0, 1);
        lcd.print("                ");
        
        delay(2500);
        
        intlockTrigger = false;
        digitalWrite(enable, LOW);
        
        lcd.setCursor(0, 0);
        lcd.print("Reactivating....");
        lcd.setCursor(0, 1);
        lcd.print(" System is live ");
        delay(1000);
        
        digitalWrite(pushButton, LOW);
      }
      else {
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
      

      
      
      


