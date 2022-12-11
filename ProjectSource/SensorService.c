/****************************************************************************
 Module
   SensorService.c

 Description
   This is the file that contains all of the event checkers for our sensors

 Author
    Mario Peraza
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "SensorService.h"
#include "EventCheckers.h"
#include "ModeServiceFSM.h"
#include "terminal.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/
static bool InputStateCheck(ES_Event_t InputEvent, TriggerState_t *LastInputState, uint32_t InputPin);

/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
//IMPORTANT: ONLY USE PORTB FOR GAME INPUTS
//Touch Sensor Vars
#define TouchPin PORTBbits.RB13
static TriggerState_t LastTouchState;
TriggerState_t CurrentTouchState;

//Accelerometer Sensor Vars
#define ShakePin PORTBbits.RB10
static TriggerState_t LastShakeState;
TriggerState_t CurrentShakeState;

//Squeeze Sensor Vars
  #define SqueezePin PORTBbits.RB11
static TriggerState_t LastSqueezeState;
TriggerState_t CurrentSqueezeState;

//IR Sensor Vars
#define WavePin PORTBbits.RB15
static TriggerState_t LastWaveState;
TriggerState_t CurrentWaveState;
static int NumWaves;

//GameButton
#define GamePin PORTBbits.RB9
static TriggerState_t LastGameState;
TriggerState_t CurrentGameState;

//ZenButton
#define ZenPin PORTBbits.RB8
static TriggerState_t LastZenState;
TriggerState_t CurrentZenState;

static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitSensorService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes

****************************************************************************/
bool InitSensorService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  
   //Use RB13 as Touch Sensor
  TRISBbits.TRISB13 = 1; // input
  ANSELBbits.ANSB13 = 0; //Digital
  LastTouchState = TouchPin;
  
  //Init Accelerometer
  TRISBbits.TRISB10 = 1; //input
  LastShakeState = ShakePin;
  
  //Init Squeeze Sensor
  TRISBbits.TRISB11 = 1; //input
  LastSqueezeState = SqueezePin;
  
  //INIT IR Sensor
  TRISBbits.TRISB15 = 1;
  ANSELBbits.ANSB15 = 0;
  LastWaveState = WavePin;
  NumWaves = 0;
  
  //Init Game Button
  TRISBbits.TRISB9 = 1; //input
  LastGameState = GamePin;
  
  //Init Zen Button
  TRISBbits.TRISB8 = 1; //input
  LastZenState = ZenPin;
  
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
     PostSensorService

 Parameters
     ES_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this service

****************************************************************************/
bool PostSensorService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunSensorService

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   Keeps track of how long all sensors have been inactive, posts ES_NOTRIG
   if sensors inactive for longer than 30s

 Author
    Mario Peraza
****************************************************************************/
ES_Event_t RunSensorService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  
  ES_Event_t NoTrigEvent;
  NoTrigEvent.EventType = ES_NOTRIG;
  
  switch (ThisEvent.EventType)
  {
      case ES_TRIGGER:
      {
          ES_Timer_InitTimer(NoTriggerTimer, 30000);
      }
      break;
      
      case ES_TIMEOUT:
      {
          if (ThisEvent.EventParam == NoTriggerTimer)
          {
              PostModeServiceFSM(NoTrigEvent);
          }
      }
      break;
      
      default:
          ;
  }
  
  
  return ReturnEvent;
}

/****************************************************************************
 Function
    CheckTouchEvents

 Parameters
    None

 Returns
    bool, True if module has been triggered

 Description
    Helper function to easily post a ES_Touch event when the touch sensor has been 
 *  touched 

 Author
    Mario Peraza
****************************************************************************/
bool CheckTouchEvents(void)
{
    ES_Event_t ThisEvent;
    ThisEvent.EventType = ES_Touch;
    bool ReturnVal = InputStateCheck(ThisEvent, &LastTouchState, TouchPin);
    return ReturnVal;
}

/****************************************************************************
 Function
    CheckShakeEvents

 Parameters
    None

 Returns
    bool, True if module has been triggered

 Description
    Helper function to easily post a ES_Shake event when the accelerometer
 *  output exceeds a certain threshold voltage

 Author
    Mario Peraza
****************************************************************************/
bool CheckShakeEvents(void)
{
    ES_Event_t ThisEvent;
    ThisEvent.EventType = ES_Shake;
    bool ReturnVal = InputStateCheck(ThisEvent, &LastShakeState, ShakePin);
    return ReturnVal;
}

/****************************************************************************
 Function
    CheckWaveEvents

 Parameters
    None

 Returns
    bool, True if module has been triggered

 Description
    Helper function to easily post a ES_Squeeze event when the force sensor has been 
 *  triggered 

 Author
    Mario Peraza
****************************************************************************/
bool CheckSqueezeEvents(void)
{
    ES_Event_t ThisEvent;
    ThisEvent.EventType = ES_Squeeze;
    bool ReturnVal = InputStateCheck(ThisEvent, &LastSqueezeState, SqueezePin);
    return ReturnVal;
}

/****************************************************************************
 Function
    CheckWaveEvents

 Parameters
    None

 Returns
    bool, True if module has been triggered

 Description
    Helper function to easily post a ES_Wave event when the IR sensor has been 
 *  triggered more than 6 times

 Author
    Mario Peraza
****************************************************************************/
bool CheckWaveEvents(void)
{
    ES_Event_t ThisEvent;
    ThisEvent.EventType = ES_NO_EVENT;
    bool ReturnVal = InputStateCheck(ThisEvent, &LastWaveState, WavePin);
    //Only Send when it detects a certain number of waves
    if (ReturnVal == true)
    {
        NumWaves++;
        if (NumWaves>=6)
        {  
            NumWaves = 0;
            ThisEvent.EventType = ES_Wave;
            ThisEvent.EventParam = LastWaveState;
            PostModeServiceFSM(ThisEvent);
        }
    }
    return ReturnVal;
}

/****************************************************************************
 Function
    CheckGameButton

 Parameters
    None

 Returns
    bool, True if module has been triggered

 Description
    Helper function to easily post a ES_GAME event when the Game button has been 
 *  pressed 

 Author
    Mario Peraza
****************************************************************************/
bool CheckGameButton(void)
{
    ES_Event_t ThisEvent;
    ThisEvent.EventType = ES_GAME;
    bool ReturnVal = InputStateCheck(ThisEvent, &LastGameState, GamePin);
    return ReturnVal;
}


/****************************************************************************
 Function
    CheckZenButton

 Parameters
    None

 Returns
    bool, True if module has been triggered

 Description
    Helper function to easily post a ES_ZEN event when the Zen button has been 
 *  pressed 

 Author
    Mario Peraza
****************************************************************************/
bool CheckZenButton(void)
{
    ES_Event_t ThisEvent;
    ThisEvent.EventType = ES_ZEN;
    bool ReturnVal = InputStateCheck(ThisEvent, &LastZenState, ZenPin);
    return ReturnVal;
}

/***************************************************************************
 private functions
 ***************************************************************************/
/****************************************************************************
 Function
     InputStateCheck

 Parameters
    ES_Event_t, the event with type corresponding to input
 *  TriggerState_t, The Last Input state pointer
 *  uint32_t The pijn corresponding to the input state

 Returns
     bool, True if module has been triggered

 Description
     helper function to easily post a triggered event for the given input

Author
    Mario Peraza
****************************************************************************/
static bool InputStateCheck(ES_Event_t InputEvent, TriggerState_t *LastInputState, uint32_t InputPin)
{
    bool ReturnVal = false;
    TriggerState_t CurrentInputState;
    
    ES_Event_t TriggerEvent;

    CurrentInputState = InputPin;
    if (CurrentInputState != *LastInputState)
    {
        *LastInputState = CurrentInputState;
        ReturnVal = true;
        if (CurrentInputState == Triggered)
        {
            InputEvent.EventParam = CurrentInputState;
            PostModeServiceFSM(InputEvent);
            TriggerEvent.EventType = ES_TRIGGER;
            ES_PostToService(MyPriority, TriggerEvent);
        }
    }

    return ReturnVal;
}
/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/