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


EventManager gEM;



// Our listener will simply toggle the state of the pin
void listener( int event, int pin )
{
    gPins[pin].pinState = gPins[pin].pinState ? false : true;
    digitalWrite( gPins[pin].pinNbr, gPins[pin].pinState ? HIGH : LOW  );
    gPins[pin].lastToggled = millis();
    Serial.println("free function called");
}




void setup()
{
    // Setup
    Serial.begin( 115200 );
    pinMode( gPins[0].pinNbr, OUTPUT );
    pinMode( gPins[1].pinNbr, OUTPUT );

    // Add our listener
    gEM.addListener( EventManager::kEventUser0, listener );
    Serial.print( "Number of listeners: " );
    Serial.println( gEM.numListeners() );
}


void loop()
{
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



