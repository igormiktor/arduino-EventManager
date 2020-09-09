#include <Arduino.h>

#ifndef ESP32
#error This code is designed to run on the ESP32 Arduino platform
#endif

// Using the master branch of arduino-EventManager
// https://github.com/igormiktor/arduino-EventManager/tree/master
#include <EventManager\EventManager.h>

// Using the ESP32TimerInterrupt library
// https://github.com/khoih-prog/ESP32TimerInterrupt
#include <ESP32TimerInterrupt.h>

EventManager eventManager;

struct Pins
{
    int pinNumber;
    int pinState;
};

Pins pins[2] = { { LED_BUILTIN, LOW }, { 4, LOW } };

#define TIMER0_INTERVAL_MS 100UL
ESP32Timer interruptTimer0( 0 );

// IRAM_ATTR is required to place the handler in the right area of memory
void IRAM_ATTR interruptHandler( void ) 
{
    static int oddEven = 0;

    eventManager.queueEvent( EventManager::kEventUser0, 0 );

    if ( !oddEven ) 
    {
        eventManager.queueEvent( EventManager::kEventUser0, 1 );
    }

    ++oddEven;
    oddEven %= 3;
}

void listener( int event, int pin )
{
    pins[pin].pinState = pins[pin].pinState ? 0 : 1;
    digitalWrite( pins[pin].pinNumber, pins[pin].pinState ? HIGH : LOW );
}

void setup() 
{
    Serial.begin( 115200 );
    Serial.println( "Entering setup()" );

    pinMode( pins[0].pinNumber, OUTPUT );
    pinMode( pins[1].pinNumber, OUTPUT );

    eventManager.addListener( EventManager::kEventUser0, listener );

    interruptTimer0.attachInterruptInterval( TIMER0_INTERVAL_MS * 1000UL, interruptHandler );
}

void loop() 
{
    eventManager.processEvent();
}
