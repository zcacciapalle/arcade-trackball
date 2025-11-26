//HID compliant mouse library
#include "Mouse.h"

//Digital input pins to put the x and y direction photointerruptors on
#define X_DIR_PIN 4
#define Y_DIR_PIN 5

//Rate at which mouse data is sent to computer (every POLL_RATE milliseconds)
#define POLL_RATE 10

//Define the mouse acceleration parameters (scale up the accelerated component and define the exponent of the accelerated component
#define ACCELERATION_CONSTANT 2
#define ACCELERATION_POWER 1.5

//Define the slapping acceleration threshold and cooldown time (milliseconds) for slap to click
#define SLAP_ACCEL_THRESHOLD 10
#define SLAPTOCLICK_COOLDOWN_TIME 50

//Store differential positional increments from the ISRs until the next time we send them to the computer
int16_t dx=0, dy=0;

//Store the last millis() time we sent data to the computer and the last time a slap to click was registered
uint32_t lastUpdate=0, lastSlapToClick=0;

//Store the average x, y, and z accelerometer values (taken each loop() iteration using weighted 80% existing average values and 20% current values)
int16_t avgX, avgY, avgZ;

//Interrupt service routine for X axis
void isrX()
{
  //When the x interrupt photointerruptor goes low, check the state of the x direction photointerruptor to determine direction
  if (digitalRead(X_DIR_PIN))
    dx--;
  else dx++;
}

//Interrupt service routine for Y axis
void isrY()
{
  //When the y interrupt photointerruptor goes low, check the state of the y direction photointerruptor to determine direction
  if (digitalRead(Y_DIR_PIN))
    dy++;
  else dy--;
}

void setup()
{ 
  //Hardware interrupts for X and Y axes
  pinMode(2, INPUT);
  pinMode(3, INPUT);

  //Directions for X and Y axes
  pinMode(X_DIR_PIN, INPUT);
  pinMode(Y_DIR_PIN, INPUT);

  //Attach interrupts
  attachInterrupt(digitalPinToInterrupt(2), isrX, FALLING);
  attachInterrupt(digitalPinToInterrupt(3), isrY, FALLING);

  Serial.begin(115200);

  //Read in initial values for accelerometer upon power on as initial averages
  avgX=analogRead(A0);
  avgY=analogRead(A1);
  avgZ=analogRead(A2);

  //Begin mouse
  Mouse.begin();
}

void loop()
{
  if (millis()-lastUpdate > POLL_RATE)
  {
    //Calculate mouse movement
    int8_t moveX=dx+((int8_t)(ACCELERATION_CONSTANT*pow(dx < 0 ? -dx : dx, ACCELERATION_POWER)*(dx < 0 ? -1 : 1))),
      moveY=dy+((int8_t)(ACCELERATION_CONSTANT*pow(dy < 0 ? -dy : dy, ACCELERATION_POWER)*(dy < 0 ? -1 : 1)));

    //Move the mouse
    Mouse.move(moveX, moveY, 0);

    //Reset differential x and y variables
    dx=0;
    dy=0;

    //Set next time to update mouse position
    lastUpdate=millis();
  }

  //Read in current accelerometer values
  int16_t x=analogRead(A0), y=analogRead(A1), z=analogRead(A2);

  //Register a click when every axis exceeds the threshold difference from average values and the cooldown time has elapsed
  if (abs(avgX-x) > SLAP_ACCEL_THRESHOLD && abs(avgY-y) > SLAP_ACCEL_THRESHOLD && abs(avgZ-z) > SLAP_ACCEL_THRESHOLD && millis()-lastSlapToClick > SLAPTOCLICK_COOLDOWN_TIME)
  {
    Mouse.click(MOUSE_LEFT);

    //Debug slap to click if needed
    #ifdef __SLAPTOCLICK_DEBUG
    Serial.print(x);
    Serial.print("\t");
    Serial.print(y);
    Serial.print("\t");
    Serial.println(z);
    #endif
    
    //Update last time a click was registered for cooldown purposes
    lastSlapToClick=millis();
  }

  //Take the averages for this iteration of loop()
  avgX=(avgX*4+x)/5;
  avgY=(avgY*4+y)/5;
  avgZ=(avgZ*4+z)/5;
}
