/****************************************************************************

  Header file for Instruction Flat Sate Machine
  based on the Gen2 Events and Services Framework

 ****************************************************************************/

#ifndef InstructionService_H
#define InstructionService_H

// Event Definitions
#include "ES_Configure.h" /* gets us event definitions */
#include "ES_Types.h"     /* gets bool type for returns */

// typedefs for the states
// State definitions for use with the query function
typedef enum
{
  WaitForGameStart, StartInstructions
}InstructionState_t;

extern unsigned char Instruction[50];

// Public Function Prototypes

bool InitInstructionService(uint8_t Priority);
bool PostInstructionService(ES_Event_t ThisEvent);
ES_Event_t RunInstructionService(ES_Event_t ThisEvent);
InstructionState_t QueryInstructionService(void);

#endif /* FSMTemplate_H */

