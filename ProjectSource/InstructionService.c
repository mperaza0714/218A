/****************************************************************************
 Module
   InstructionService.c

 Revision
   1.0.1

 Description
   State Machine that updates the current instruction shown on the screen for 
   Game Mode

  Author
    Mario Peraza
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include <string.h>
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "InstructionService.h"
#include "ModeServiceFSM.h"
#include "LEDService.h"
#include "terminal.h"
#include "dbprintf.h"

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/
static void GetInstruction(Module_t Module);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static InstructionState_t CurrentState;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

unsigned char Instruction[50];

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitInstructionService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine

****************************************************************************/
bool InitInstructionService(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = WaitForGameStart;
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
     PostInstructionService

 Parameters
     EF_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue

****************************************************************************/
bool PostInstructionService(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunTemplateFSM

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   Updates the instructions while in game mode

****************************************************************************/
ES_Event_t RunInstructionService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  ES_Event_t LEDEvent;
  LEDEvent.EventType = ES_ADD_STRING;

  switch (CurrentState)
  {
    case WaitForGameStart:        
    {
      if (ThisEvent.EventType == ES_INSTRUCT)    
      {
        ES_PostToService(MyPriority, ThisEvent);
        CurrentState = StartInstructions;
      }
    }
    break;

    case StartInstructions:        
    {
      switch (ThisEvent.EventType)
      {
        case ES_INSTRUCT:  
        {   
          ES_Timer_InitTimer(InstructTimer, 1000);
          GetInstruction(CurrentModule);
          PostLEDService(LEDEvent);
        }
        break; 

        case ES_TIMEOUT:
        {
          if (ThisEvent.EventParam == InstructTimer)
          {
            GetInstruction(CurrentModule);
            ES_Timer_InitTimer(InstructTimer, 1000); 
            PostLEDService(LEDEvent);
          }
        }
        break;

        case ES_STOPINSTRUCT:
        {
          CurrentState = WaitForGameStart;
        }
        break;

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

/****************************************************************************
 Function
     QueryInstructionService

 Parameters
     None

 Returns
     TemplateState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine
****************************************************************************/
InstructionState_t QueryInstructionService(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/

/****************************************************************************
 Function
     GetInstruction

 Parameters
     Module_t, the Current Active Module

 Returns
     nothing

 Description
     Updates the Instruction String depending on the current active module and 
     point total
****************************************************************************/
static void GetInstruction(Module_t Module)
{
    if (Module == Touch)
    {
        if (Points >= 10)
        {
            snprintf(Instruction, 50, "Touch      %dpt", Points);
        }
        else
        {
            snprintf(Instruction, 50, "Touch       %dpt", Points);
        }
    }
    if (Module == Squeeze)
    {
        if (Points >= 10)
        {
            snprintf(Instruction, 50, "Squeeze     %dpt", Points);
        }
        else
        {
            snprintf(Instruction, 50, "Squeeze      %dpt", Points);
        }
    }
    if (Module == Shake)
    {
        if (Points >= 10)
        {
            snprintf(Instruction, 50, "Shake      %dpt", Points);
        }
        else
        {
            snprintf(Instruction, 50, "Shake       %dpt", Points);
        }
    }
    if (Module == Wave)
    {
        if (Points >= 10)
        {
            snprintf(Instruction, 50, "Wave      %dpt", Points);
        }
        else
        {
            snprintf(Instruction, 50, "Wave       %dpt", Points);
        }
    }
}
