/****************************************************************************

  Header file for LED Service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef LEDService_H
#define LEDService_H

#include <stdint.h>
#include <stdbool.h>

#include "ES_Events.h"
#include "ES_Port.h"                // needed for definition of REENTRANT
// Public Function Prototypes
#include "ES_Events.h"
#include "ES_Port.h" 

typedef enum
{
  InitPState, Idle, Display
}TemplateState_t;

bool InitLEDService(uint8_t Priority);
bool PostLEDService(ES_Event_t ThisEvent);
ES_Event_t RunLEDService(ES_Event_t ThisEvent);
TemplateState_t QueryDisplayCharFSM(void);

#endif /* ServTemplate_H */

