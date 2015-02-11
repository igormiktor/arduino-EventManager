/*
  This sketch assumes an LED on pin 13 (built-in on Arduino Uno) and 
  an LED on pin 8.  It blinks boths LED using events generated 
  by checking elasped time against millis() -- no interrupts involved.
  
  Author: igormt@alumni.caltech.edu
  Copyright (c) 2013 Igor Mikolic-Torreira
  
  This software is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser
  General Public License as published by the Free Software
  Foundation; either version 2.1 of the License, or (at
  your option) any later version.
 
  This software is distributed in the hope that it will
  be useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public
  License for more details.
 
  You should have received a copy of the GNU Lesser
  General Public License along with this library; if not,
  write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

*/


#include "EventManager.h"


struct Pins
{
    int pinNbr;
    int pinState;
    unsigned long lastToggled;
};

Pins gPins[2] = { { 13, LOW, 0 }, { 8, LOW, 0 } };


// No interrupts, so can use the non-interrupt safe mode
EventManager gEM( EventManager::kNotInterruptSafe );



// Our listener will simply toggle the state of the pin
void listener( int event, int pin )
{
    gPins[pin].pinState = gPins[pin].pinState ? false : true;
    digitalWrite( gPins[pin].pinNbr, gPins[pin].pinState ? HIGH : LOW  );
    gPins[pin].lastToggled = millis();
    Serial.println("free function called");
}

class C
{
  int v;
public:
  C(int _v):v(_v) {};
  void f(int a, int b){
    Serial.print("member function called, value is ");
    Serial.println(v);
  };
};

C c(2);
C d(1);
MemberFunctionCallable<C> listenerMemberFunction1(&c,&C::f);
MemberFunctionCallable<C> listenerMemberFunction2;
GenericCallable<void(int,int)> listenerFreeFunction(listener);



void setup() 
{                
  // Setup
  Serial.begin( 115200 );
  pinMode( gPins[0].pinNbr, OUTPUT );
  pinMode( gPins[1].pinNbr, OUTPUT );

  listenerMemberFunction2.obj=&d;
  listenerMemberFunction2.f=&C::f;


  // Add our listener
  gEM.addListener( EventManager::kEventUser0, &listenerMemberFunction1 );
  gEM.addListener( EventManager::kEventUser0, &listenerFreeFunction );
  gEM.addListener( EventManager::kEventUser0, &listenerMemberFunction2 );
  Serial.print("listeners number:");
  Serial.println(gEM.numListeners());
}


uint32_t loopCount=0;

void loop() 
{
  loopCount++;



  switch (loopCount)
  {
    case 100000:
      gEM.removeListener( &listenerMemberFunction1);
      Serial.print("(case 1000000)listeners number:");
      Serial.println(gEM.numListeners());
      break;
    // case 20000:
    //   gEM.removeListener( &listenerMemberFunction2);
    //   break;
    // case 30000:
    //   gEM.removeListener( &listenerMemberFunction1);
    //   break;


  }

    // Handle any events that are in the queue
    gEM.processEvent();
    
    // Add events into the queue
    addPin0Events();
    addPin1Events();
}


// Add events to toggle pin 13 (gPins[0]) every second
// NOTE:  doesn't handle millis() turnover
void addPin0Events()
{
    if ( ( millis() - gPins[0].lastToggled ) > 1000 )
    {
        gEM.queueEvent( EventManager::kEventUser0, 0 );
    }
}

// Add events to toggle pin 8 (gPins[1]) every second
// NOTE:  doesn't handle millis() turnover
void addPin1Events()
{
    if ( ( millis() - gPins[1].lastToggled ) > 3000 )
    {
	     gEM.queueEvent( EventManager::kEventUser0, 1 );
         Serial.print("(addPin1Events)listeners number:");
         Serial.println(gEM.numListeners());

    }
}



