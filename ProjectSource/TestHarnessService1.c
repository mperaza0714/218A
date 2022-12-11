/****************************************************************************
 Module
   TestHarnessService0.c

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
#include "../ProjectHeaders/TestHarnessService1.h"

// debugging printf()

// Hardware
#include <xc.h>
//#include <proc/p32mx170f256b.h>

// Event & Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_Port.h"
#include "terminal.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/
// these times assume a 10.000mS/tick timing
#define ONE_SEC 1000
#define HALF_SEC (ONE_SEC / 2)
#define TWO_SEC (ONE_SEC * 2)
#define FIVE_SEC (ONE_SEC * 5)

#define ENTER_POST     ((MyPriority<<3)|0)
#define ENTER_RUN      ((MyPriority<<3)|1)
#define ENTER_TIMEOUT  ((MyPriority<<3)|2)

#define TEST_INT_POST
/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

static void InitLED(void);
static void BlinkLED(void);
#ifdef TEST_INT_POST
static void InitTMR2(void);
static void StartTMR2(void);
#endif
/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
// add a deferral queue for up to 3 pending deferrals +1 to allow for overhead
static ES_Event_t DeferralQueue[3 + 1];

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTestHarnessService0

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
bool InitTestHarnessService1(uint8_t Priority)
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
  DB_printf( "Press any key to post key-stroke events to Service 0\n\r");
  DB_printf( "Press 'd' to test event deferral \n\r");
  DB_printf( "Press 'r' to test event recall \n\r");
  DB_printf( "Press 'p' to test posting from an interrupt \n\r");

  /********************************************
   in here you write your initialization code
   *******************************************/
  // initialize deferral queue for testing Deferal function
  ES_InitDeferralQueueWith(DeferralQueue, ARRAY_SIZE(DeferralQueue));
  // initialize LED drive for testing/debug output
  // InitLED();
#ifdef TEST_INT_POST
  InitTMR2();
#endif
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
     PostTestHarnessService0

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
bool PostTestHarnessService0(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTestHarnessService0

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
ES_Event_t RunTestHarnessService0(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

#ifdef _INCLUDE_BYTE_DEBUG_
  _HW_ByteDebug_SetValueWithStrobe( ENTER_RUN );
#endif  
  switch (ThisEvent.EventType)
  {
    case ES_NEW_KEY:   // announce
    {
      printf("ES_NEW_KEY received with -> %c <- in Service 1\r\n",
          (char)ThisEvent.EventParam);
      if ('x' == ThisEvent.EventParam)
      {
        PostTestHarnessService1(MyPriority, {ES_EVENT1, ThisEvent.EventParam});
      }
      if ('y' == ThisEvent.EventParam)
      {
        PostTestHarnessService1(MyPriority, {ES_EVENT2, ThisEvent.EventParam});
      }
    }
    break;

    case ES_EVENT1:
    {
      printf("This event is -> %c",
      (char)ThisEvent.EventParam);
    }
    break;

    case ES_EVENT2:
    {
      printf("This event is -> %c",
      (char)ThisEvent.EventParam);
    }
    break;

    default:
    {}
     break;
  }

  return ReturnEvent;
}

