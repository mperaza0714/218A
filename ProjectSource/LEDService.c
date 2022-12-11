/****************************************************************************
 Module
 LEDService.c

 Revision
   1.0.1

 Description
   This is the first service for the LED service under the
   Gen2 Events and Services Framework.

 Notes

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
// This module
#include "LEDService.h"

// debugging printf()

// Hardware
#include <xc.h>
//#include <proc/p32mx170f256b.h>

// Event & Services Framework
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ES_DeferRecall.h"
#include "ES_Events.h"
#include "ES_Port.h"
#include "terminal.h"
#include "dbprintf.h"

#include "DM_Display.h"
#include "FontStuff.h"
#include "PIC32_SPI_HAL.h"
#include "InstructionService.h"

/*----------------------------- Module Defines ----------------------------*/

#define ENTER_POST     ((MyPriority<<3)|0)
#define ENTER_RUN      ((MyPriority<<3)|1)
#define ENTER_TIMEOUT  ((MyPriority<<3)|2)

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/

/*---------------------------- Module Variables ---------------------------*/
static TemplateState_t CurrentState;
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;
// add a deferral queue for up to 3 pending deferrals +1 to allow for overhead
static ES_Event_t DeferralQueue[7 + 1];

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitLEDService

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
bool InitLEDService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;

  // When doing testing, it is useful to announce just which program
  // is running.
  
  clrScrn();
  puts("\rStarting LED Service for \r");
  DB_printf( "the 2nd Generation Events & Services Framework V2.4\r\n");
  DB_printf( "compiled at %s on %s\n", __TIME__, __DATE__);
  DB_printf( "\n\r\n");
  DB_printf( "Press any key to post key-stroke events to Service 0\n\r");

  /********************************************
   in here you write your initialization code
   *******************************************/
  // initialize deferral queue for testing Deferal function
  ES_InitDeferralQueueWith(DeferralQueue, ARRAY_SIZE(DeferralQueue));
  
  //Initialize SPI
  #define WhichModule SPI_SPI1
  #define WhichPhase SPI_SMP_MID
  #define SDO_Pin  SPI_RPA1
  #define SS_Pin  SPI_RPA0
  bool isEnhanced = true;
  uint32_t Period_ns = 1e4;
  #define DataWidth SPI_16BIT
  #define WhichEdge SPI_FIRST_EDGE
  #define WhichState SPI_CLK_LO
    
    
  //SPI Setup
  SPISetup_BasicConfig(WhichModule);
  SPISetup_SetLeader(WhichModule, WhichPhase);
  SPISetup_MapSDOutput(WhichModule, SDO_Pin);
  SPISetup_MapSSOutput(WhichModule, SS_Pin);
  SPISetup_DisableSPI(WhichModule);
  SPISetEnhancedBuffer(WhichModule, isEnhanced);
  SPISetup_SetBitTime(WhichModule, Period_ns);
  SPISetup_SetActiveEdge(WhichModule, WhichEdge);
  SPISetup_SetClockIdleState(WhichModule, WhichState);
  SPISetup_SetXferWidth(WhichModule, DataWidth);
  SPISetup_EnableSPI(WhichModule);
  
  IFS0CLR = _IFS0_INT4IF_MASK; //clear flags
  
  //Initialize Screen
  while (false == DM_TakeInitDisplayStep()){}
  
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
     PostLEDService

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
 bool PostLEDService(ES_Event_t ThisEvent)
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
ES_Event_t RunLEDService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  static char DeferredChar = '1';

  switch (CurrentState)
  {
    case InitPState:      
    {
      if (ThisEvent.EventType == ES_INIT)    
      {
        CurrentState = Idle;
      }
    }
    break;

    case Idle:
    {
        switch (ThisEvent.EventType)
        {
            //Adds Char to Display
            case ES_ADD_CHAR:
            {
                CurrentState = Display;
                DM_ScrollDisplayBuffer(4);
                DM_AddChar2Display(ThisEvent.EventParam);
            }
            break;
            
            //Adds String to Display
            case ES_ADD_STRING:
            {
                CurrentState = Display;
                DM_AddString2Display(Instruction);
            }
            break;
            
            defaul:
                ;
        }  
    }

    case Display:        
    {
      switch (ThisEvent.EventType)
      {
        case ES_ADD_CHAR: 

        { 
            if (false == DM_TakeDisplayUpdateStep()){
                ES_PostToService(MyPriority, ThisEvent);
            } else {
                CurrentState = Idle;
            }
        }
        break;
        
        case ES_ADD_STRING:
        {
            if (false == DM_TakeDisplayUpdateStep()){
                ES_PostToService(MyPriority, ThisEvent);
            } else {
                DM_ClearDisplayBuffer();
                CurrentState = Idle;
            }
        }

        // repeat cases as required for relevant events
        default:
          ;
      }  // end switch on CurrentEvent
    }
    break;
    // repeat state pattern as required for other states
    default:
      ;
  }                                   // end switch on Current State
  return ReturnEvent;
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

