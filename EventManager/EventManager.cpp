/*
 * EventManager.cpp
 *

 * An event handling system for Arduino.
 *
 * Author: igormt@alumni.caltech.edu
 * Copyright (c) 2013 Igor Mikolic-Torreira
 *
 * Inspired by and adapted from the
 * Arduino Event System library by
 * Author: mromani@ottotecnica.com
 * Copyright (c) 2010 OTTOTECNICA Italy
 *
 * This library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser
 * General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser
 * General Public License along with this library; if not,
 * write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */


#include "EventManager.h"

namespace
{
    // This class takes care of turning interrupts on and off.
    // There is a different implementation of this class for each architecture that
    // has a different interrupt model.  #if macros ensure only one version is defined.

#if defined( __AVR_ARCH__ )

    class SuppressInterrupts
    {
    public:

        // Record the current state and suppress interrupts when the object is instantiated.
        SuppressInterrupts()
        {
            mInterruptsWereOn = (SREG & (1<<SREG_I));
            cli();
        }

        // Restore whatever interrupt state was active before
        ~SuppressInterrupts()
        {
            // Turn on global interrupts, only if they were already on
            if ( mInterruptsWereOn )
            {
                sei();
            }
        }

    private:

        uint8_t     mInterruptsWereOn;
    };

#elif defined( SAM ) || defined( ARDUINO_ARCH_SAMD )

    class SuppressInterrupts
    {
    public:

        // Record the current state and suppress interrupts when the object is instantiated.
        SuppressInterrupts()
        {
            mInterruptsWereOn = (__get_PRIMASK() == 0);
            __disable_irq();
        }

        // Restore whatever interrupt state was active before
        ~SuppressInterrupts()
        {
            // Turn on interrupts, only if they were already on
            if ( mInterruptsWereOn )
            {
                __enable_irq();
            }
        }

    private:

        uint8_t     mInterruptsWereOn;
    };

#elif defined( ESP8266 )

    class SuppressInterrupts
    {
    public:

        // Record the current state and suppress interrupts when the object is instantiated.
        SuppressInterrupts()
        {
            // This turns off interrupts and gets the old state in one function call
            // See https://github.com/esp8266/Arduino/issues/615 for details
            // level 15 will disable ALL interrupts,
            // level 0 will enable ALL interrupts
            mSavedInterruptState = xt_rsil( 15 );
        }

        // Restore whatever interrupt state was active before
        ~SuppressInterrupts()
        {
            // Restore the old interrupt state
            xt_wsr_ps( mSavedInterruptState );
        }

    private:

        uint32_t    mSavedInterruptState;
    };

#elif defined( CORE_TEENSY )
    
    class SuppressInterrupts
    {
    public:
        
        //Reference: https://www.pjrc.com/teensy/interrupts.html
        //Backup the interrupt enable state and restore it
        SuppressInterrupts() 
        {
            mSregBackup = SREG;     /* save interrupt enable/disable state */
            cli();                  /* disable the global interrupt */
        }
        
        ~SuppressInterrupts() 
        {
            SREG = mSregBackup;     /* restore interrupt state */
        }
        
    private:
        
        uint8_t mSregBackup;
    };
    
#elif defined( ESP32 )
    
    #include <freertos/portmacro.h>

    class SuppressInterrupts
    {
    public:
        
        // Reference: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/freertos-smp.html#critical-sections-disabling-interrupts
        // Enter critical section
        SuppressInterrupts()
        {
            portENTER_CRITICAL(&gMux);
        }
        
        // Exit critical section
        ~SuppressInterrupts()
        {
            portEXIT_CRITICAL(&gMux);
        }

        static portMUX_TYPE gMux;
    };
    
    // gMux is globally accessible as a public static member variable
    portMUX_TYPE SuppressInterrupts::gMux = portMUX_INITIALIZER_UNLOCKED;

#else

#error "Unknown microcontroller:  Need to implement class SuppressInterrupts for this microcontroller."

#endif

}




#if EVENTMANAGER_DEBUG
#define EVTMGR_DEBUG_PRINT( x )		Serial.print( x );
#define EVTMGR_DEBUG_PRINTLN( x )	Serial.println( x );
#define EVTMGR_DEBUG_PRINT_PTR( x )	Serial.print( reinterpret_cast<unsigned long>( x ), HEX );
#define EVTMGR_DEBUG_PRINTLN_PTR( x )	Serial.println( reinterpret_cast<unsigned long>( x ), HEX );
#else
#define EVTMGR_DEBUG_PRINT( x )
#define EVTMGR_DEBUG_PRINTLN( x )
#define EVTMGR_DEBUG_PRINT_PTR( x )
#define EVTMGR_DEBUG_PRINTLN_PTR( x )
#endif


EventManager::EventManager()
{
}


int EventManager::processEvent()
{
    int eventCode;
    int param;
    int handledCount = 0;

    if ( mHighPriorityQueue.popEvent( &eventCode, &param ) )
    {
        handledCount = mListeners.sendEvent( eventCode, param );

        EVTMGR_DEBUG_PRINT( "processEvent() hi-pri event " )
        EVTMGR_DEBUG_PRINT( eventCode )
        EVTMGR_DEBUG_PRINT( ", " )
        EVTMGR_DEBUG_PRINT( param )
        EVTMGR_DEBUG_PRINT( " sent to " )
        EVTMGR_DEBUG_PRINTLN( handledCount )
    }

    // If no high-pri events handled (either because there are no high-pri events or
    // because there are no listeners for them), then try low-pri events
    if ( !handledCount && mLowPriorityQueue.popEvent( &eventCode, &param ) )
    {
        handledCount = mListeners.sendEvent( eventCode, param );

        EVTMGR_DEBUG_PRINT( "processEvent() lo-pri event " )
        EVTMGR_DEBUG_PRINT( eventCode )
        EVTMGR_DEBUG_PRINT( ", " )
        EVTMGR_DEBUG_PRINT( param )
        EVTMGR_DEBUG_PRINT( " sent to " )
        EVTMGR_DEBUG_PRINTLN( handledCount )
    }

    return handledCount;
}


int EventManager::processAllEvents()
{
    int eventCode;
    int param;
    int handledCount = 0;

    while ( mHighPriorityQueue.popEvent( &eventCode, &param ) )
    {
        handledCount += mListeners.sendEvent( eventCode, param );

        EVTMGR_DEBUG_PRINT( "processEvent() hi-pri event " )
        EVTMGR_DEBUG_PRINT( eventCode )
        EVTMGR_DEBUG_PRINT( ", " )
        EVTMGR_DEBUG_PRINT( param )
        EVTMGR_DEBUG_PRINT( " sent to " )
        EVTMGR_DEBUG_PRINTLN( handledCount )
    }

    while ( mLowPriorityQueue.popEvent( &eventCode, &param ) )
    {
        handledCount += mListeners.sendEvent( eventCode, param );

        EVTMGR_DEBUG_PRINT( "processEvent() lo-pri event " )
        EVTMGR_DEBUG_PRINT( eventCode )
        EVTMGR_DEBUG_PRINT( ", " )
        EVTMGR_DEBUG_PRINT( param )
        EVTMGR_DEBUG_PRINT( " sent to " )
        EVTMGR_DEBUG_PRINTLN( handledCount )
    }

    return handledCount;
}



/********************************************************************/



EventManager::ListenerList::ListenerList() :
mNumListeners( 0 ), mDefaultCallback( 0 )
{
}

int EventManager::ListenerList::numListeners()
{
    return mNumListeners;
};

int EventManager::numListeners()
{
    return mListeners.numListeners();
};

boolean EventManager::ListenerList::addListener( int eventCode, EventListener listener )
{
    EVTMGR_DEBUG_PRINT( "addListener() enter " )
    EVTMGR_DEBUG_PRINT( eventCode )
    EVTMGR_DEBUG_PRINT( ", " )
    EVTMGR_DEBUG_PRINTLN_PTR( listener )

    // Argument check
    if ( !listener )
    {
        return false;
    }

    // Check for full dispatch table
    if ( isFull() )
    {
        EVTMGR_DEBUG_PRINTLN( "addListener() list full" )
        return false;
    }

    mListeners[ mNumListeners ].callback = listener;
    mListeners[ mNumListeners ].eventCode = eventCode;
    mListeners[ mNumListeners ].enabled 	= true;
    mNumListeners++;

    EVTMGR_DEBUG_PRINTLN( "addListener() listener added" )

    return true;
}


boolean EventManager::ListenerList::removeListener( int eventCode, EventListener listener )
{
    EVTMGR_DEBUG_PRINT( "removeListener() enter " )
    EVTMGR_DEBUG_PRINT( eventCode )
    EVTMGR_DEBUG_PRINT( ", " )
    EVTMGR_DEBUG_PRINTLN_PTR( listener )

    if ( mNumListeners == 0 )
    {
        EVTMGR_DEBUG_PRINTLN( "removeListener() no listeners" )
        return false;
    }

    int k = searchListeners( eventCode, listener );
    if ( k < 0 )
    {
        EVTMGR_DEBUG_PRINTLN( "removeListener() not found" )
        return false;
    }

    for ( int i = k; i < mNumListeners - 1; i++ )
    {
        mListeners[ i ].callback  = mListeners[ i + 1 ].callback;
        mListeners[ i ].eventCode = mListeners[ i + 1 ].eventCode;
        mListeners[ i ].enabled   = mListeners[ i + 1 ].enabled;
    }
    mNumListeners--;

    EVTMGR_DEBUG_PRINTLN( "removeListener() removed" )

    return true;
}


int EventManager::ListenerList::removeListener( EventListener listener )
{
    EVTMGR_DEBUG_PRINT( "removeListener() enter " )
    EVTMGR_DEBUG_PRINTLN_PTR( listener )

    if ( mNumListeners == 0 )
    {
        EVTMGR_DEBUG_PRINTLN( "  removeListener() no listeners" )
        return 0;
    }

    int removed = 0;
    int k;
    while ((k = searchListeners( listener )) >= 0 )
    {
        for ( int i = k; i < mNumListeners - 1; i++ )
        {
            mListeners[ i ].callback  = mListeners[ i + 1 ].callback;
            mListeners[ i ].eventCode = mListeners[ i + 1 ].eventCode;
            mListeners[ i ].enabled   = mListeners[ i + 1 ].enabled;
        }
        mNumListeners--;
        removed++;
   }

    EVTMGR_DEBUG_PRINT( "  removeListener() removed " )
    EVTMGR_DEBUG_PRINTLN( removed )

    return removed;
}


boolean EventManager::ListenerList::enableListener( int eventCode, EventListener listener, boolean enable )
{
    EVTMGR_DEBUG_PRINT( "enableListener() enter " )
    EVTMGR_DEBUG_PRINT( eventCode )
    EVTMGR_DEBUG_PRINT( ", " )
    EVTMGR_DEBUG_PRINT_PTR( listener )
    EVTMGR_DEBUG_PRINT( ", " )
    EVTMGR_DEBUG_PRINTLN( enable )

    if ( mNumListeners == 0 )
    {
        EVTMGR_DEBUG_PRINTLN( "enableListener() no listeners" )
        return false;
    }

    int k = searchListeners( eventCode, listener );
    if ( k < 0 )
    {
        EVTMGR_DEBUG_PRINTLN( "enableListener() not found fail" )
        return false;
    }

    mListeners[ k ].enabled = enable;

    EVTMGR_DEBUG_PRINTLN( "enableListener() success" )
    return true;
}


boolean EventManager::ListenerList::isListenerEnabled( int eventCode, EventListener listener )
{
    if ( mNumListeners == 0 )
    {
        return false;
    }

    int k = searchListeners( eventCode, listener );
    if ( k < 0 )
    {
        return false;
    }

    return mListeners[ k ].enabled;
}


int EventManager::ListenerList::sendEvent( int eventCode, int param )
{
    EVTMGR_DEBUG_PRINT( "sendEvent() enter " )
    EVTMGR_DEBUG_PRINT( eventCode )
    EVTMGR_DEBUG_PRINT( ", " )
    EVTMGR_DEBUG_PRINTLN( param )

    int handlerCount = 0;
    for ( int i = 0; i < mNumListeners; i++ )
    {
        if ( ( mListeners[ i ].callback != 0 ) && ( mListeners[ i ].eventCode == eventCode ) && mListeners[ i ].enabled )
        {
            handlerCount++;
            (*mListeners[ i ].callback)( eventCode, param );
        }
    }

    EVTMGR_DEBUG_PRINT( "sendEvent() sent to " )
    EVTMGR_DEBUG_PRINTLN( handlerCount )

    if ( !handlerCount )
    {
        if ( ( mDefaultCallback != 0 ) && mDefaultCallbackEnabled )
        {
            handlerCount++;
            (*mDefaultCallback)( eventCode, param );

            EVTMGR_DEBUG_PRINTLN( "sendEvent() event sent to default" )
        }

#if EVENTMANAGER_DEBUG
        else
        {
            EVTMGR_DEBUG_PRINTLN( "sendEvent() no default" )
        }
#endif

    }

    return handlerCount;
}


boolean EventManager::ListenerList::setDefaultListener( EventListener listener )
{
    EVTMGR_DEBUG_PRINT( "setDefaultListener() enter " )
    EVTMGR_DEBUG_PRINTLN_PTR( listener )

    if ( listener == 0 )
    {
        return false;
    }

    mDefaultCallback = listener;
    mDefaultCallbackEnabled = true;
    return true;
}


void EventManager::ListenerList::removeDefaultListener()
{
    mDefaultCallback = 0;
    mDefaultCallbackEnabled = false;
}


void EventManager::ListenerList::enableDefaultListener( boolean enable )
{
    mDefaultCallbackEnabled = enable;
}


int EventManager::ListenerList::searchListeners( int eventCode, EventListener listener )
{

    for ( int i = 0; i < mNumListeners; i++ )
    {


        if ( ( mListeners[i].eventCode == eventCode ) && ( mListeners[i].callback == listener ) )
        {
            return i;
        }
    }

    return -1;
}


int EventManager::ListenerList::searchListeners( EventListener listener )
{
    for ( int i = 0; i < mNumListeners; i++ )
    {
        if ( mListeners[i].callback == listener )
        {
            return i;
        }
    }

    return -1;
}


int EventManager::ListenerList::searchEventCode( int eventCode )
{
    for ( int i = 0; i < mNumListeners; i++ )
    {
        if ( mListeners[i].eventCode == eventCode )
        {
            return i;
        }
    }

    return -1;
}



/******************************************************************************/




EventManager::EventQueue::EventQueue() :
mEventQueueHead( 0 ),
mEventQueueTail( 0 ),
mNumEvents( 0 )
{
    for ( int i = 0; i < kEventQueueSize; i++ )
    {
        mEventQueue[i].code = EventManager::kEventNone;
        mEventQueue[i].param = 0;
    }
}



boolean ISR_ATTR EventManager::EventQueue::queueEvent( int eventCode, int eventParam )
{
    /*
    * The call to noInterrupts() MUST come BEFORE the full queue check.
    *
    * If the call to isFull() returns FALSE but an asynchronous interrupt queues
    * an event, making the queue full, before we finish inserting here, we will then
    * corrupt the queue (we'll add an event to an already full queue). So the entire
    * operation, from the call to isFull() to completing the inserting (if not full)
    * must be atomic.
    *
    * Note that this race condition can only arise IF both interrupt and non-interrupt (normal)
    * code add events to the queue.  If only normal code adds events, this can't happen
    * because then there are no asynchronous additions to the queue.  If only interrupt
    * handlers add events to the queue, this can't happen because further interrupts are
    * blocked while an interrupt handler is executing.  This race condition can only happen
    * when an event is added to the queue by normal (non-interrupt) code and simultaneously
    * an interrupt handler tries to add an event to the queue.  This is the case that the
    * cli() (= noInterrupts()) call protects against.
    *
    * Contrast this with the logic in popEvent().
    *
    */

    SuppressInterrupts  interruptsOff;      // Interrupts automatically restored when exit block

    // ATOMIC BLOCK BEGIN
    boolean retVal = false;
    if ( !isFull() )
    {
        // Store the event at the tail of the queue
        mEventQueue[ mEventQueueTail ].code = eventCode;
        mEventQueue[ mEventQueueTail ].param = eventParam;

        // Update queue tail value
        mEventQueueTail = ( mEventQueueTail + 1 ) % kEventQueueSize;;

        // Update number of events in queue
        mNumEvents++;

        retVal = true;
    }
    // ATOMIC BLOCK END

    return retVal;
}


boolean EventManager::EventQueue::popEvent( int* eventCode, int* eventParam )
{
    /*
    * The call to noInterrupts() MUST come AFTER the empty queue check.
    *
    * There is no harm if the isEmpty() call returns an "incorrect" TRUE response because
    * an asynchronous interrupt queued an event after isEmpty() was called but before the
    * return is executed.  We'll pick up that asynchronously queued event the next time
    * popEvent() is called.
    *
    * If interrupts are suppressed before the isEmpty() check, we pretty much lock-up the Arduino.
    * This is because popEvent(), via processEvents(), is normally called inside loop(), which
    * means it is called VERY OFTEN.  Most of the time (>99%), the event queue will be empty.
    * But that means that we'll have interrupts turned off for a significant fraction of the
    * time.  We don't want to do that.  We only want interrupts turned off when we are
    * actually manipulating the queue.
    *
    * Contrast this with the logic in queueEvent().
    *
    */

    if ( isEmpty() )
    {
        return false;
    }

    SuppressInterrupts  interruptsOff;      // Interrupts automatically restored when exit block

    // Pop the event from the head of the queue
    // Store event code and event parameter into the user-supplied variables
    *eventCode  = mEventQueue[ mEventQueueHead ].code;
    *eventParam = mEventQueue[ mEventQueueHead ].param;

    // Clear the event (paranoia)
    mEventQueue[ mEventQueueHead ].code = EventManager::kEventNone;

    // Update the queue head value
    mEventQueueHead = ( mEventQueueHead + 1 ) % kEventQueueSize;

    // Update number of events in queue
    mNumEvents--;

    return true;
}
