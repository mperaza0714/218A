/****************************************************************************

  Header file for LED Service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef ScrollService_H
#define ScrollService_H

#include <stdint.h>
#include <stdbool.h>

#include "ES_Events.h"
#include "ES_Port.h"                // needed for definition of REENTRANT
// Public Function Prototypes

bool InitScrollService(uint8_t Priority);
bool PostScrollService(ES_Event_t ThisEvent);
ES_Event_t RunScrollService(ES_Event_t ThisEvent);

#endif /* ServTemplate_H */

