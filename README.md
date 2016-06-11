# Arduino EventManager

Using an event-driven design is a common way to code Arduino projects that
interact with the environment around them.  **EventManager** is
a single C++ class that provides an event handling system for Arduino.  With
**EventManager** you can register functions that "listen"
for particular events and when things happen you can "post" events to
**EventManager**.  You then use the loop() function to regularly tell
**EventManager** to process  events and the appropriate listeners will be
called.

**EventManger** is designed to be interrupt safe, so that you can post events
from interrupt handlers.  The corresponding listeners will be
called from outside the interrupt handler in your loop() function when you tell
**EventManager** to process events.

In keeping with the limited resources of an Arduino system, **EventManager** is
light-weight.  There is no dynamic memory allocation.  Event
queuing is very fast (so you can be comfortable queuing events from interrupt
handlers).  To keep the footprint minimal, the event queue and
the listener list are both small (although you can make them bigger if needed).

NOTE:  There are two versions of **EventManager**.  The master branch has a version
of **EventManager** that uses functions as listeners (also known as event handlers).
Most users will find that this version meets their needs.  The `GenericListeners`
branch contains a version of **EventManager** that accepts more general types of
listeners such as callable member functions and callable objects.  If you don't
know what these are, stick to the master branch.


## Installation 

Copy the folder `EventManager` into your Arduino `Libraries` folder, as
described in the
[Arduino documentation](<http://arduino.cc/en/Guide/Libraries>).


## Usage 

At the top of your sketch you must include the **EventManager** header file

```C
    #include <EventManager.h>
```

And then at global scope you should instantiate an **EventManager** object

```C
    EventManager gMyEventManager;
```

You can safely instantiate more than one **EventManager** object, if so desired,
but the two objects will be completely independent.  This might be useful if
perhaps you need to have separate event processes in different components of
your code.


## Events

**EventManager** `Events` consist of an event code and an event parameter.  Both
of these are integer values.  The event code identifies the type of event.  For
your convenience, `EventManager.h` provides a set of constants you can use to
identify events

```C
    EventManager::kEventKeyPress
    EventManager::kEventKeyRelease
    EventManager::kEventChar
    EventManager::kEventTime
    EventManager::kEventTimer0
    EventManager::kEventTimer1
    EventManager::kEventTimer2
    EventManager::kEventTimer3
    EventManager::kEventAnalog0
    EventManager::kEventAnalog1
    EventManager::kEventAnalog2
    EventManager::kEventAnalog3
    EventManager::kEventAnalog4
    EventManager::kEventAnalog5
    EventManager::kEventMenu0
    EventManager::kEventMenu1
    EventManager::kEventMenu2
    EventManager::kEventMenu3
    EventManager::kEventMenu4
    EventManager::kEventMenu5
    EventManager::kEventMenu6
    EventManager::kEventMenu7
    EventManager::kEventMenu8
    EventManager::kEventMenu9
    EventManager::kEventSerial
    EventManager::kEventPaint
    EventManager::kEventUser0
    EventManager::kEventUser1
    EventManager::kEventUser2
    EventManager::kEventUser3
    EventManager::kEventUser4
    EventManager::kEventUser5
    EventManager::kEventUser6
    EventManager::kEventUser7
    EventManager::kEventUser8
    EventManager::kEventUser9
```

These are purely for your convenience; **EventManager** only uses the value to
match events to listeners, so you are free to use any event codes you wish.  The
event parameter is also whatever you want it to be: for a key press event it
could be the corresponding key code.  For an analog event it could be the value
read from that analog pin or a pin number.  The event parameter will be passed
to every listener that is associated with that event code.

You post events using the `queueEvent()` function

```C++
    gMyEventManager.queueEvent( EventManager::kEventUser0, 1234 );
```

The `queueEvent()` function is lightweight and interrupt safe, so you can call
it from inside an interrupt handler.

By default the event queue holds 8 events, but you can make the queue any size
you want by defining the macro `EVENTMANAGER_EVENT_QUEUE_SIZE` to whatever
value you desire (see [Increase Event Queue Size](#increase-event-queue-size) below).


## Listeners

Listeners are functions of type

```C++
    typedef void ( *EventListener )( int eventCode, int eventParam );
```

You add listeners using the `addListener()` function

```C++
    void myListener( int eventCode, int eventParam )
    {
        // Do something with the event
    }

    void setup()
    {
        gMyEventManager.addListener( EventManager::kEventUser0, myListener );

        // Do more set up
    }
```

Do *not* add listeners from within an interrupt routine.

By default the list of
listeners holds 8 listeners, but you can make the list any size you want by
defining the macro `EVENTMANAGER_LISTENER_LIST_SIZE` to whatever
value you desire (see [Increase Listener List Size](#increase-listener-list-size) below).


## Processing Events

To actually process events in the event queue and dispatch them to listeners you
call the `processEvent()` function

```C++
    void loop()
    {
        gMyEventManager.processEvent();
    }
```

This call processes one event from the event queue every time it is called.
The standard usage is to call `processEvent()` once in your `loop()`
function so that one event is handled every time through the loop. This is
usually more than adequate to keep up with incoming events.  Events are
normally processed in a first-in, first-out fashion (but see the section on
`Event Priority` below).


## Example

Here is a simple example illustrating how to blink the LED on pin 13 using
**EventManager**

```C++
    #include <Arduino.h>
    #include <EventManager.h>

    boolean pin13State;
    unsigned long lastToggled;

    EventManager gEM;

    // Our listener will simply toggle the state of pin 13
    void listener( int event, int param )
    {
        // event and param are not used in this example function
        pin13State = pin13State ? false : true;
        digitalWrite( 13, pin13State ? HIGH : LOW  );
        lastToggled = millis();
    }

    void setup()
    {
        // Setup
        pinMode( 13, OUTPUT );
        digitalWrite( 13, HIGH );
        pin13State = true;
        lastToggled = millis();

        // Add our listener
        gEM.addListener( EventManager::kEventUser0, listener );
    }

    void loop()
    {
        // Handle any events that are in the queue
        gEM.processEvent();

        // Add events into the queue
        addPinEvents();
     }

    // Add events to toggle pin 13 every second
    // NOTE:  doesn't handle millis() turnover
    void addPinEvents()
    {
        if ( ( millis() - lastToggled ) > 1000 )
        {
            gEM.queueEvent( EventManager::kEventUser0, 0 );
        }
    }
```

The examples that come with the **EventManager** library (accessible via the
Arduino `File/Examples` menu) provide more sophisticated illustrations of how
you can use **EventManager**.


## Advanced Details

### Event Priority

**EventManager** recognizes high and low priority events.  You can specify the
priority when you queue the event.  By default, events are considered low
priority.  You indicate an event is high priority by passing an additional
constant to `queueEvent()`, like so

```C++
    gMyEventManager.queueEvent( EventManager::kEventUser0, 1234, EventManager::kHighPriority );
```

The difference between high and low priority events is that `processEvent()`
will process a high priority event ahead of any low priority
events.  In effect, high priority events jump to the front of the queue
(multiple high priority events are processed first-in,
first-out, but all of them are processed before any low priority events).

Note that if high priority events are queued faster than low priority events,
EventManager may never get to processing any of the low priority
events.  So use high priority events judiciously.


### Interrupt Safety

**EventManager** was designed to be interrupt safe, so that you can queue events
both from within interrupt handlers and also from normal functions without
having to worry about queue corruption.  However, this safety comes at the price
of slightly slower `queueEvent()` and `processEvent()` functions and the
need to globally disable interrupts while certain small snippets of code are
executing.


### Processing All Events

Normally calling `processEvent()` once every time through the `loop()`
function is more than adequate to service incoming events.  However, there may
be times when you want to process all the events in the queue.  For this purpose
you can call `processAllEvents()`.  Note that if you call this function at the
same time that a series of events are being rapidly added to the queue
asynchronously (via interrupt handlers), the `processAllEvents()` function
might not return until the series of additions to the event queue stops.


### Increase Event Queue Size

Define `EVENTMANAGER_EVENT_QUEUE_SIZE` to whatever size you need at 
the very beginning of `EventManager.h` like so

```C++
    #ifndef EventManager_h
    #define EventManager_h

    #define EVENTMANAGER_EVENT_QUEUE_SIZE   16

    #include <Arduino.h>
```

If you are using the Arduino IDE, it is not enough to define this constant the 
usual C/C++ way by defining the constant *before* including `EventManager.h` 
in your own files.  This is because the Arduino IDE has no way to pass the 
definition to the library code unless you actually edit `EventManager.h`. The 
Arduino IDE lacks a way to pass precompile constants to all the files in the 
project.  Given that the underlying compiler is GCC, it would be nice if the 
Arduino IDE had a dialog to set things like `-D EVENTMANAGER_EVENT_QUEUE_SIZE=16` 
and have this constant definition passed directly to the compiler.

The event queue requires `4*sizeof(int) = 8` bytes for each unit of size.
There is a factor of 4 (instead of 2) because internally **EventManager**
maintains two separate queues: a high-priority queue and a low-priority queue.


### Increase Listener List Size

Define `EVENTMANAGER_LISTENER_LIST_SIZE` to whatever size you need at 
the very beginning of `EventManager.h` like so

```C++
    #ifndef EventManager_h
    #define EventManager_h

    #define EVENTMANAGER_LISTENER_LIST_SIZE   16

    #include <Arduino.h>
```

If you are using the Arduino IDE, it is not enough to define this constant the 
usual C/C++ way by defining the constant *before* including `EventManager.h` 
in your own files.  This is because the Arduino IDE has no way to pass the 
definition to the library code unless you actually edit `EventManager.h`. The 
Arduino IDE lacks a way to pass precompile constants to all the files in the 
project.  Given that the underlying compiler is GCC, it would be nice if the 
Arduino IDE had a dialog to set things like `-D EVENTMANAGER_LISTENER_LIST_SIZE=16` 
and have this constant definition passed directly to the compiler.

The listener list requires `sizeof(*f()) + sizeof(int) + sizeof(boolean) = 5`
bytes for each unit of size.


### Additional Features

There are various class functions for managing the listeners:

* You can remove listeners (`removeListener()`),
* Disable and enable specific listeners (`enableListener()`),
* Set a default listener that will handle any events not handled by other listeners and manipulate the default listener just like any other listener (`setDefaultListener()`, `removeDefaultListener()`, and `enableDefaultListener()`)
* Check the status of the listener list (`isListenerListEmpty()`, `isListenerListFull()`)

There are various class functions that provide information about the event
queue:

* Check the status of the event queue (`isEventQueueEmpty()`, `isEventQueueFull()`)
* See how many events are in the queue (`getNumEventsInQueue()`)

For details on these functions you should review *EventManager.h*.


## Feedback

If you find a bug or if you would like a specific feature, please report it at:

https://github.com/igormiktor/arduino-EventManager/issues

If you would like to hack on this project, don't hesitate to fork it on GitHub.
If you would like me to incorporate changes you made, don't hesitate to send me
a Pull Request.


## Credits

**EventManager** was inspired by and adapted from the `Arduino Event System
library` created by mromani@ottotecnica.com of OTTOTECNICA Italy, which was
kindly released under a LGPL 2.1 license.


## License

This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation; either version 2.1 of the License, or (at your option) any
later version.

This library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

A copy of the license is included in the **EventManager** package.


## Copyright

Copyright (c) 2016 Igor Mikolic-Torreira

Portions are Copyright (c) 2010 OTTOTECNICA Italy


