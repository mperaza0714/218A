/****************************************************************************
 Module
   ScrollService.c

 Revision
   1.0.1

 Description
   This is the first service for the Test Harness under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 10/26/17 18:26 jec     moves definition of ALL_BITS to ES_Port.h
 10/19/17 21:28 jec     meaningless change to test updating
 10/19/17 18:42 jec     removed referennces to driverlib and programmed the
                        ports directly
 08/21/17 21:44 jec     modified LED blink routine to only modify bit 3 so that
                        I can test the new new framework debugging lines on PF1-2
 08/16/17 14:13 jec      corrected ONE_SEC constant to match Tiva tick rate
 11/02/13 17:21 jec      added exercise of the event deferral/recall module
 08/05/13 20:33 jec      converted to test harness service
 01/16/12 09:58 jec      began conversion from TemplateFSM.c
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// This module
#include "ScrollService.h"

// debugging printf()

// Hardware
#include <xc.h>
#include <string.h>
//#include <proc/p32mx170f256b.h>

// Event & Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_Port.h"
#include "terminal.h"
#include "dbprintf.h"
#include "LEDService.h"

/*----------------------------- Module Defines ----------------------------*/
// these times assume a 10.000mS/tick timing
#define ONE_SEC 1000
#define HALF_SEC (ONE_SEC / 2)
#define TWO_SEC (ONE_SEC * 2)
#define FIVE_SEC (ONE_SEC * 5)

#define ENTER_POST     ((MyPriority<<3)|0)
#define ENTER_RUN      ((MyPriority<<3)|1)
#define ENTER_TIMEOUT  ((MyPriority<<3)|2)

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
// add a deferral queue for up to 3 pending deferrals +1 to allow for overhead
static ES_Event_t DeferralQueue[3 + 1];

static unsigned char message[50] = "This is my message, it's cool.";
static int i = 0;


/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitScrollService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 01/16/12, 10:00
****************************************************************************/
bool InitScrollService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;

  // When doing testing, it is useful to announce just which program
  // is running.
  clrScrn();
  puts("\rStarting Test Harness for \r");
  DB_printf( "the 2nd Generation Events & Services Framework V2.4\r\n");
  DB_printf( "compiled at %s on %s\n", __TIME__, __DATE__);
  DB_printf( "\n\r\n");
  DB_printf( "Press any key to post key-stroke events to Scroll Service\n\r");

  /********************************************
   in here you write your initialization code
   *******************************************/
  // initialize deferral queue for testing Deferal function
  ES_InitDeferralQueueWith(DeferralQueue, ARRAY_SIZE(DeferralQueue));
  
  static unsigned char message[50] = "This is my message, it's cool.";
  const int i = 0;

  // initialize the Short timer system for channel A
  //ES_ShortTimerInit(MyPriority, SHORT_TIMER_UNUSED);

  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  if (ES_PostToService(MyPriority, ThisEvent) == true)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/****************************************************************************
 Function
     PostScrollService

 Parameters
     ES_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostScrollService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunScrollService

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   add your description here
 Notes

 Author
   J. Edward Carryer, 01/15/12, 15:23
****************************************************************************/
ES_Event_t RunScrollService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  static char DeferredChar = '1';

#ifdef _INCLUDE_BYTE_DEBUG_
  _HW_ByteDebug_SetValueWithStrobe( ENTER_RUN );
#endif  
  switch (ThisEvent.EventType)
  {
    case ES_INIT:
    {
        ES_Timer_InitTimer(SERVICE2_TIMER, 1000);
        
    }
    break;
    case ES_TIMEOUT:
    {
        if (ThisEvent.EventParam == SERVICE2_TIMER){
            ES_Event_t Display_Event = {ES_ADD_CHAR, message[i]};
            PostLEDService(Display_Event);
            i++;
            if (i > strlen(message)-1) {
                i = 0;
            }
            ES_Timer_InitTimer(SERVICE2_TIMER, 1000);
        }
    }
    default:
    {}
     break;
  }

  return ReturnEvent;
}

/***************************************************************************
 private functions
 ***************************************************************************/
/*
#include <sys/attribs.h> // for ISR macors

// for testing posting from interrupts.
// Intializes TMR2 to gerenate an interrupt at 1s
static void InitTMR2(void)
{
  // turn timer off
  T2CONbits.ON = 0;
  // Use internal peripheral clock
  T2CONbits.TCS = 0;
  // setup for 16 bit mode
  T2CONbits.T32 = 0;
  // set prescale to 1:1
  T2CONbits.TCKPS = 0;
  // load period value
  PR2 = (2000-1)*10; // creates a 100ms period with a 20MHz peripheral clock
  // set priority
  IPC2bits.T2IP = 2;
  // clear interrupt flag
  IFS0bits.T2IF = 0;
  // enable the timer interrupt
  IEC0bits.T2IE = 1;
}

// Clears and Starts TMR2
static void StartTMR2(void)
{
  // clear timer
  TMR2 = 0;
  // start timer
  //LATBbits.LATB14 = 0;
  T2CONbits.ON = 1;
}

void __ISR(_TIMER_2_VECTOR, IPL2AUTO) Timer2ISR(void)
{
  // clear flag
  IFS0bits.T2IF = 0;
  // post event
  static ES_Event_t interruptEvent = {ES_SHORT_TIMEOUT, 0};
  PostScrollService(interruptEvent);
  
  // stop timer
  T2CONbits.ON = 0;
  return;
}
 */
/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

