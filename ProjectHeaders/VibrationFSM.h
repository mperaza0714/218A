/****************************************************************************

  Header file for template Flat Sate Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef VibrationFSM_H
#define VibrationFSM_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  InitMotorState, MotorON, MotorOFF
}VibrationState_t;

// Public Function Prototypes

bool InitVibrationFSM(uint8_t Priority);
bool PostVibrationFSM(ES_Event_t ThisEvent);
ES_Event_t RunVibrationFSM(ES_Event_t ThisEvent);
bool CheckAnalogValue();


#endif /* FSMTemplate_H */

