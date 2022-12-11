/****************************************************************************
 Module
   TemplateFSM.c

 Revision
   1.0.1

 Description
   This is a template file for implementing flat state machines under the
   Gen2 Events and Services Framework.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 01/15/12 11:12 jec      revisions for Gen2 framework
 11/07/11 11:26 jec      made the queue static
 10/30/11 17:59 jec      fixed references to CurrentEvent in RunTemplateSM()
 10/23/11 18:20 jec      began conversion from SMTemplate.c (02/20/07 rev)
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "VibrationFSM.h"
#include "PIC32_AD_Lib.h"
#include "PWM_PIC32.h"

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/

#define CHANNEL_PWM 1

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static VibrationState_t CurrentState;

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;
//static uint32_t AnalogValue[1];

static uint32_t LastAnalogValue[1];
static uint8_t CurrentDutyCycle;

/*------------------------------ Module Code ------------------------------*/

bool InitVibrationFSM(uint8_t Priority)
{
    ES_Event_t ThisEvent;

    MyPriority = Priority;
  
    // RB12 as analog input from POT
    TRISBbits.TRISB12 = 1;
    ANSELBbits.ANSB12 = 1;
    ADC_ConfigAutoScan(BIT12HI, 1); // Set A/D converter to convert on AN12 (RB12)
    
    // RB3 as PWM
    TRISBbits.TRISB3 = 0;
    PWMSetup_BasicConfig(1);
    PWMSetup_AssignChannelToTimer(CHANNEL_PWM, _Timer3_);
    PWMSetup_SetFreqOnTimer(200, _Timer3_);
    PWMSetup_MapChannelToOutputPin(CHANNEL_PWM, PWM_RPB3);
    
    clrScrn();
    printf("\rVibration FSM \r\n");
  
    // put us into the Initial PseudoState
    CurrentState = InitMotorState;
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


bool PostVibrationFSM(ES_Event_t ThisEvent)
{
    return ES_PostToService(MyPriority, ThisEvent);
}


ES_Event_t RunVibrationFSM(ES_Event_t ThisEvent)
{
    ES_Event_t ReturnEvent;
    ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

    switch (CurrentState)
    {
      
        case InitMotorState:        // If current state is initial Psedudo State
        {
            if (ThisEvent.EventType == ES_INIT)    // only respond to ES_Init
            {
                printf("\rMotor init\r\n");
                PWMOperate_SetDutyOnChannel(0, CHANNEL_PWM);
                ADC_MultiRead(LastAnalogValue);
                CurrentState = MotorOFF;
                ES_PostToService(MyPriority, ReturnEvent);
            }
        }
        break;

        case MotorON:        
        {
            switch (ThisEvent.EventType)
            {
                case StartMotor:  

                {   
                    ADC_MultiRead(LastAnalogValue);
                    // Motor could be felt between 15~100%
                    // Map analog input range 0~1023 to duty cycle range 15~100%
                    CurrentDutyCycle = (15.0 + LastAnalogValue[0]*85.0/1023.0);
                    PWMOperate_SetDutyOnChannel(CurrentDutyCycle, CHANNEL_PWM); 
                }
                break;
                
                case StopMotor:  
                {   
                    PWMOperate_SetDutyOnChannel(0, CHANNEL_PWM);
                    CurrentState = MotorOFF; 
                    ThisEvent.EventType = StopMotor;
                    ES_PostToService(MyPriority, ThisEvent);
                }
                break;
                default:
                ;
            }  
        }
        break;
        
        case MotorOFF:      
        {
            switch (ThisEvent.EventType)
            {
                case StartMotor:  
                {
                    CurrentState = MotorON;
                    ThisEvent.EventType = StartMotor;
                    ES_PostToService(MyPriority, ThisEvent);
                }
                break;
                default:
                ;
            }  
        }
        break;
        default:
        ;
    }                                   // end switch on Current State
    return ReturnEvent;
}


/****************************************************************************
 Function
    CheckAnalogValue

 Parameters
    None

 Returns
    bool, true if analog value has been changed, false otherwise

 Description
    Event check to check if the analog value has been changed, if true and the 
 *  motor is currently ON, then re-post StartMotor event to adjust the motor PWM
 *  to the new analog value
****************************************************************************/
bool CheckAnalogValue()
{
    bool ReturnVal = false;
    uint32_t CurrentAnalogValue[1];
    ES_Event_t Event2Post;

    ADC_MultiRead(CurrentAnalogValue);
    
    // analog value changed
    if ( CurrentAnalogValue[0] != LastAnalogValue[0] )
    {
        ReturnVal = true;
        
        if (CurrentState == MotorON)
        {
            //Post StartMotor event to adjust motor PWM to the new analog value
            Event2Post.EventType = StartMotor;
            PostVibrationFSM(Event2Post);
        }
    }
    LastAnalogValue[0] = CurrentAnalogValue[0];
    return ReturnVal;
}