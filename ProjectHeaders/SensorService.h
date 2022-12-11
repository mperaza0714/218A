/****************************************************************************

  Header file for sensor service
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef SensorService_H
#define SensorService_H

#include "ES_Types.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum{
    NoTrig, Triggered
}TriggerState_t;



// Public Function Prototypes

bool InitSensorService(uint8_t Priority);
bool PostSensorService(ES_Event_t ThisEvent);
ES_Event_t RunSensorService(ES_Event_t ThisEvent);
bool CheckTouchEvents(void);
bool CheckShakeEvents(void);
bool CheckSqueezeEvents(void);
bool CheckWaveEvents(void);
bool CheckGameButton(void);
bool CheckZenButton(void);

#endif /* ButtonService_H */