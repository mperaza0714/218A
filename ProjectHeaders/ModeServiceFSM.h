/****************************************************************************

  Header file for template Flat Sate Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef ModeServiceFSM_H
#define ModeServiceFSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  IdleMode, GameMode, ZenMode
}ModeState_t;

typedef enum
{
    StartUp, ActivateModule, WaitForTrigger, EndGame
}GameState_t;

typedef enum{
  Touch, Shake, Squeeze, Wave, No_Module
}Module_t;

//Game Vars
extern Module_t CurrentModule;
extern uint32_t Points;


// Public Function Prototypes

bool InitModeServiceFSM(uint8_t Priority);
bool PostModeServiceFSM(ES_Event_t ThisEvent);
ES_Event_t RunModeServiceFSM(ES_Event_t ThisEvent);
ModeState_t QueryModeServiceFSM(void);

#endif /* FSMTemplate_H */

