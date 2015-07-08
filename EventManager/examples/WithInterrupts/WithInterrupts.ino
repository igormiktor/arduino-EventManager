/*
  This sketch assumes an LED on pin 13 (built-in on Arduino Uno) and
  an LED on pin 8.  It blinks boths LED using timer interrupts.

  Timer interrupts are generated using the MsTimer2 library available at
  http://playground.arduino.cc/Main/FlexiTimer2

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


#include <MsTimer2.h>

#include <EventManager.h>


struct Pins
{
    int pinNbr;
    int pinState;
};

Pins gPins[2] = { { 13, LOW }, { 8, LOW } };

EventManager gEM;



// Our interrupt generates a toggle event for pin 13 (gPins[0]) every
// time called (set to be every 1s) and and a toggle event for
// pin 8 (gPins[1]) every third call (equivalent to every 3s)
void interruptHandler()
{
    static int oddEven = 0;

    gEM.queueEvent( EventManager::kEventUser0, 0 );

    if ( !oddEven )
    {
        gEM.queueEvent( EventManager::kEventUser0, 1 );
    }

    ++oddEven;
    oddEven %= 3;
}



// Our listener will simply toggle the state of the pin
void listener( int event, int pin )
{
    gPins[pin].pinState = gPins[pin].pinState ? false : true;
    digitalWrite( gPins[pin].pinNbr, gPins[pin].pinState ? HIGH : LOW );
}


void setup()
{
    // Setup
    Serial.begin( 115200 );
    pinMode( gPins[0].pinNbr, OUTPUT );
    pinMode( gPins[1].pinNbr, OUTPUT );

    // Add our listener
    gEM.addListener( EventManager::kEventUser0, listener );

    // Set up interrupts every second
    MsTimer2::set( 1000, interruptHandler ); // 1 sec period
    MsTimer2::start();
}


void loop()
{
    // Handle any events that are in the queue
    gEM.processEvent();
}

