/****************************************************************************
 Module
   ModeServiceFSM.c

 Description
   This Contains the Main State Machine which runs the different interaction modes

 Author(s)
   Mario Peraza, Peter Shih

****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
/* include header files for this state machine as well as any machines at the
   next lower level in the hierarchy that are sub-machines to this machine
*/
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "ModeServiceFSM.h"
#include "SensorService.h"
#include "LEDService.h"
#include "PWM_PIC32.h"
#include "InstructionService.h"
#include "VibrationFSM.h"
#include "terminal.h"
#include "dbprintf.h"
#include <stdlib.h>
#include <string.h>
/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this machine.They should be functions
   relevant to the behavior of this state machine
*/
static void PlayAudio(int Audio);
static void Light(Module_t Module);
static void LightSingle(Module_t Module, bool ON);
static int RandomModule(void);

/*---------------------------- Module Variables ---------------------------*/
// everybody needs a state variable, you may need others as well.
// type of state variable should match htat of enum in header file
static ModeState_t CurrentState;
static GameState_t CurrentGameState;

//Audio Setup
#define GameAudioPin LATAbits.LATA3
#define ZenAudioPin LATAbits.LATA4
static enum {
    GameAudio,
    ZenAudio
};
bool Flip;

//Game Vars
ES_Event_t LEDEvent;
Module_t CurrentModule;
uint32_t Points;
uint8_t IdleLight;
uint8_t IdleVibrationPWM;
bool VibrationPWMIncrease;

uint8_t ZenIdleBlinkCount;
uint8_t ZenVibrationCount;
Module_t ZenBlinkModule;
Module_t GameBlinkModule;
uint8_t LightBlinkCount;
uint8_t NoTrigBlinkCount;

//Light Pins
#define TouchLight LATBbits.LATB2
#define ShakeLight LATAbits.LATA2
#define SqueezeLight LATBbits.LATB4
#define WaveLight LATBbits.LATB5

// with the introduction of Gen2, we need a module level Priority var as well
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitModeServiceFSM

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, sets up the initial transition and does any
     other required initialization for this state machine

****************************************************************************/
bool InitModeServiceFSM(uint8_t Priority)
{
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  // put us into the Initial PseudoState
  CurrentState = IdleMode;
  //Set Initial Game State
  CurrentGameState = StartUp;
  
  //Setup Audio Pin
  TRISAbits.TRISA3 = 0; //output
  TRISAbits.TRISA4 = 0; 
  Flip = 1;
  GameAudioPin = Flip; //high
  ZenAudioPin = 1; 
  
  //Setup Light Pins
  TRISBbits.TRISB2 = 0;
  TRISAbits.TRISA2 = 0;
  TRISBbits.TRISB4 = 0;
  TRISBbits.TRISB5 = 0;
  TouchLight = 0;
  ShakeLight = 0;
  SqueezeLight = 0;
  WaveLight = 0;
  
  //Setup Game
  LEDEvent.EventType = ES_ADD_STRING;
  sprintf(Instruction, "WELCOME!        ");
  PostLEDService(LEDEvent);
  Points = 0;
  
  ES_Timer_InitTimer(VibrationTimer, 1000);
  ES_Timer_InitTimer(IdleLightTimer, 1000);
  IdleLight = 0;
  IdleVibrationPWM = 0;
  VibrationPWMIncrease = true;
  ZenIdleBlinkCount = 0;
  ZenVibrationCount = 0;
  NoTrigBlinkCount = 0;
  ZenBlinkModule = No_Module;
  GameBlinkModule = No_Module;
  
  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  InitSensorService(MyPriority);
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
     PostModeServiceFSM

 Parameters
     ES_Event_t ThisEvent , the event to post to the queue

 Returns
     boolean False if the Enqueue operation failed, True otherwise

 Description
     Posts an event to this state machine's queue

****************************************************************************/
bool PostModeServiceFSM(ES_Event_t ThisEvent)
{
  return ES_PostToService(MyPriority, ThisEvent);
}

/****************************************************************************
 Function
    RunModeServiceFSM

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event_t, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   State: IDLE
	Event: Game Button Pressed ? Go to State: GameMode
	Event: Nirvana Button Pressed ? Go to State: NirvanaMode

State: GameMode
	GameState: StartUp
		Send Instructions to Display
		Start Music
		Event: Instructions Over ? Start GameTimer go to GameState: ActivateModule
	GameState: ActivateModule
		Event: GameTimer TIMEOUT ? Go to GameState: EndGame
Activate a random module ? Post to LightUp Event with parameter of
corresponding LED to Light Service
		Go to GameState: WaitForTrigger
		Start ModuleTimer
	GameState: WaitForTrigger
		Event: GameTimer TIMEOUT ? Go to GameState: EndGame
Events: Each Sensor Module ? If the module matches active then add a point
   and go to GameState: ActivateModule
		Event: ModuleTimer TIMEOUT ? Lose a point and go to GameState:
        ActivateModule
	GameState: EndGame
		Display End Message
Clear any changed variables and go to State: IDLE
		
State: ZenMode
	Events: Each Sensor Module ? Depending on module, respond with different vibrations
					     and music

 Author
    Mario Peraza
****************************************************************************/
ES_Event_t RunModeServiceFSM(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  
  ES_Event_t InstructEvent;
  
  ES_Event_t VibrationEvent;

  switch (CurrentState)
  {
    case IdleMode:     
    {
        switch (ThisEvent.EventType)
        {
            //If Game Button Pressed
            case ES_GAME:
            {
                //Clear Instructions
                memset(Instruction, 0, sizeof(Instruction));
                PWMOperate_SetDutyOnChannel(0, 1);
                
                //Switch States and Set GameState
                CurrentState = GameMode;
                CurrentGameState = StartUp;
                //Post to Keep State Machine Running
                ES_PostToService(MyPriority, ReturnEvent);  
            }
            break;

            //If Zen Button Pressed
            case ES_ZEN:
            {
                //Initial Message
                sprintf(Instruction, "Relax   Enjoy   ", Points);
                PostLEDService(LEDEvent);

                //Switch State
                CurrentState = ZenMode;
                Light(No_Module);

                //Initialize 60s Timer
                ES_Timer_InitTimer(GameTimer, 60000);
                ES_Timer_InitTimer(IdleLightTimer, 100);
                ES_Timer_InitTimer(VibrationTimer, 100);
                //Post to Keep State Machine Running
                ES_PostToService(MyPriority, ReturnEvent);
            }
            break;
            
            case ES_TIMEOUT:
            {
                if (ThisEvent.EventParam == IdleLightTimer)
                {
                    ES_Timer_InitTimer(IdleLightTimer, 500);
                    IdleLight = (IdleLight+1)%4;
                    Light(IdleLight);
                } else if (ThisEvent.EventParam == VibrationTimer)
                {
                    if (IdleVibrationPWM < 50 && VibrationPWMIncrease)
                    {
                        IdleVibrationPWM++;
                        PWMOperate_SetDutyOnChannel(IdleVibrationPWM, 1);
                    } else if (IdleVibrationPWM == 50)
                    {
                        VibrationPWMIncrease = false;
                        IdleVibrationPWM--;
                        PWMOperate_SetDutyOnChannel(IdleVibrationPWM, 1);
                    } else if (!VibrationPWMIncrease && IdleVibrationPWM > 20)
                    {
                        IdleVibrationPWM--;
                        PWMOperate_SetDutyOnChannel(IdleVibrationPWM, 1);
                    } else 
                    {
                        IdleVibrationPWM++;
                        VibrationPWMIncrease = true;
                    }
                    ES_Timer_InitTimer(VibrationTimer, 20 + IdleVibrationPWM*2);
                    
                } else if (ThisEvent.EventParam == StateEndTimer)
                {
                    ES_Timer_InitTimer(VibrationTimer, 100);
                    LEDEvent.EventType = ES_ADD_STRING;
                    sprintf(Instruction, "WELCOME!        ");
                    PostLEDService(LEDEvent);
                }
                
            }
            break;

            default:
                ;
        }
    }
    break;

    case GameMode:
    {
        switch (CurrentGameState)
        {
            //Initial Game State
            case StartUp:
            {
                //Play Game Audio
                PlayAudio(GameAudio);

                //Start GameTimer 60s
                ES_Timer_InitTimer(GameTimer, 60000);

                //Switch Game States
                CurrentGameState = ActivateModule;
                //Post to Keep State Machine Running
                ES_PostToService(MyPriority, ReturnEvent);
            }
            break;
            
            //Activates a Random Module
            case ActivateModule:
            {
                //pick a random module 0-3
                CurrentModule = RandomModule();

                //Start Printing Module Instructions
                InstructEvent.EventType = ES_INSTRUCT;
                PostInstructionService(InstructEvent);

                //Start ModuleTimer 8s
                ES_Timer_InitTimer(ModuleTimer, 8000);

                //Switch Game State
                CurrentGameState = WaitForTrigger;
                //Post to Keep State Machine Running
                ES_PostToService(MyPriority, ReturnEvent);
                
            }
            break;
            
            //Waits for Sensor Data to be sent, 
            //checks if it matches the current active module, 
            //gives points if correct
            case WaitForTrigger:
            {
                switch (ThisEvent.EventType)
                {
                    //accelerometer
                    case ES_Shake:
                    {
                        if (CurrentModule == Shake)
                        {
                            Points++;
                            LightBlinkCount = 0;
                            GameBlinkModule = Shake;
                            ES_Timer_InitTimer(BlinkLightTimer, 100);
                            CurrentGameState = ActivateModule;
                            ES_PostToService(MyPriority, ReturnEvent);   
                        }
                    }
                    break;

                    //force
                    case ES_Squeeze:
                    {
                        if (CurrentModule == Squeeze)
                        {
                            Points++;
                            LightBlinkCount = 0;
                            GameBlinkModule = Squeeze;
                            ES_Timer_InitTimer(BlinkLightTimer, 100);
                            CurrentGameState = ActivateModule;
                            ES_PostToService(MyPriority, ReturnEvent); 
                        }
                    }
                    break;

                    //touch
                    case ES_Touch:
                    {
                        if (CurrentModule == Touch)
                        {
                            Points++;
                            LightBlinkCount = 0;
                            GameBlinkModule = Touch;
                            ES_Timer_InitTimer(BlinkLightTimer, 300);
                            CurrentGameState = ActivateModule;
                            ES_PostToService(MyPriority, ReturnEvent); 
                        }
                    }
                    break;

                    //IR
                    case ES_Wave:
                    {
                        if (CurrentModule == Wave)
                        {
                            Points++;
                            LightBlinkCount = 0;
                            GameBlinkModule = Wave;
                            ES_Timer_InitTimer(BlinkLightTimer, 100);
                            CurrentGameState = ActivateModule;
                            ES_PostToService(MyPriority, ReturnEvent); 
                        }
                    }
                    break;
                    
                    //If all sensors inactive for 30s reset and end game
                    case ES_NOTRIG:
                    {
                        //Stop Instructions
                        InstructEvent.EventType = ES_STOPINSTRUCT;
                        PostInstructionService(InstructEvent);
                        sprintf(Instruction, "Inactive        ");
                        PostLEDService(LEDEvent);
                        
                        ES_Timer_InitTimer(NoTriggerLightTimer, 3000);
                        ES_Timer_InitTimer(NoTrigBlinkLight, 10);

                        //Stop Timers
                        ES_Timer_StopTimer(GameTimer);
                        ES_Timer_StopTimer(ModuleTimer);
                        ES_Timer_StopTimer(BlinkLightTimer);
                        ES_PostToService(MyPriority, ReturnEvent); 
                    }
                    break;
                    
                    case ES_TIMEOUT:
                    {
                        if (ThisEvent.EventParam == GameTimer)
                        {
                            Light(No_Module);
                            CurrentGameState = EndGame;
                            ES_PostToService(MyPriority, ReturnEvent);
                        }
                        if (ThisEvent.EventParam == NoTrigBlinkLight)
                        {
                            if (NoTrigBlinkCount < 50)
                            {
                                if ((NoTrigBlinkCount % 2) == 0)
                                {
                                    TouchLight = 1;
                                    ShakeLight = 1; 
                                    SqueezeLight = 1;
                                    WaveLight = 1;
                                    ES_Timer_InitTimer(NoTrigBlinkLight, 5);
                                } else   
                                {
                                    Light(No_Module);
                                    ES_Timer_InitTimer(NoTrigBlinkLight, 5);
                                }
                                NoTrigBlinkCount++;
                            } else 
                            {
                                NoTrigBlinkCount = 0;
                            }
                   
                        }
                        if (ThisEvent.EventParam == NoTriggerLightTimer)
                        {
                            Light(No_Module);
                            CurrentGameState = EndGame;
                            ES_PostToService(MyPriority, ReturnEvent);
                        }
                        if (ThisEvent.EventParam == ModuleTimer)
                        {
                            CurrentGameState = ActivateModule;
                            ES_PostToService(MyPriority, ReturnEvent);
                        }
                        if (ThisEvent.EventParam == BlinkLightTimer)
                        {
                            if (LightBlinkCount < 10)
                            {
                                if ((LightBlinkCount % 2) == 0)
                                {
                                    LightSingle(GameBlinkModule, true);
                                    ES_Timer_InitTimer(BlinkLightTimer, 100);
                                } else   
                                {
                                    LightSingle(GameBlinkModule, false);
                                    ES_Timer_InitTimer(BlinkLightTimer, 100);
                                }
                                LightBlinkCount++;
                            }  
                        }
                    }

                    default:
                        ;
                }
            }
            break;
            
            //Game Over, Reset Variables and go to welcome
            case EndGame:
            {
                //Stop Printing Module Instructions
                InstructEvent.EventType = ES_STOPINSTRUCT;
                PostInstructionService(InstructEvent);

                //Accounts for double digit spacing
                if (Points >= 10)
                {
                    sprintf(Instruction, "GAMEOVER   %dpts", Points);
                }
                else
                {
                    sprintf(Instruction, "GAMEOVER    %dpts", Points);
                }
                PostLEDService(LEDEvent);

                //Stop Game Audio
                PlayAudio(GameAudio);

                //Reset
                Points = 0;
                CurrentState = IdleMode;
                ES_Timer_InitTimer(IdleLightTimer, 500);
                ES_Timer_InitTimer(StateEndTimer, 15000);
                //Post to Keep State Machine Running
                ES_PostToService(MyPriority, ReturnEvent);        
            }
            break;
        }
    }
    break;

    //Waits for Sensor Input and displays different message/ triggers audio for each sensor
    case ZenMode:
    {
        switch (ThisEvent.EventType)
        {
            case ES_Shake:
            {
                sprintf(Instruction, "Nice To Meet You");
                PostLEDService(LEDEvent);
                PlayAudio(ZenAudio);
                
                ZenBlinkModule = Shake;
                LightBlinkCount = 0;
                ES_Timer_StopTimer(IdleLightTimer);
                ES_Timer_InitTimer(BlinkLightTimer, 100);
            }
            break;

            case ES_Squeeze:
            {
                sprintf(Instruction, "OUCH");
                PostLEDService(LEDEvent);
                PlayAudio(ZenAudio);
                
                ZenBlinkModule = Squeeze;
                LightBlinkCount = 0;
                ES_Timer_StopTimer(IdleLightTimer);
                ES_Timer_InitTimer(BlinkLightTimer, 100);
            }
            break;

            case ES_Touch:
            {
                sprintf(Instruction, "Boop");
                PostLEDService(LEDEvent);
                PlayAudio(ZenAudio);
                
                ZenBlinkModule = Touch;
                LightBlinkCount = 0;
                ES_Timer_StopTimer(IdleLightTimer);
                ES_Timer_InitTimer(BlinkLightTimer, 100);
            }
            break;

            case ES_Wave:
            {
                sprintf(Instruction, "Hello");
                PostLEDService(LEDEvent);
                PlayAudio(ZenAudio);
                
                ZenBlinkModule = Wave;
                LightBlinkCount = 0;
                ES_Timer_StopTimer(IdleLightTimer);
                ES_Timer_InitTimer(BlinkLightTimer, 100);
            }
            break;
            
            //If all sensors inactive for 30s reset
            case ES_NOTRIG:
            {
                //Stop Instructions
                InstructEvent.EventType = ES_STOPINSTRUCT;
                PostInstructionService(InstructEvent);
                sprintf(Instruction, "Inactive        ");
                PostLEDService(LEDEvent);

                ES_Timer_InitTimer(NoTriggerLightTimer, 3000);
                ES_Timer_InitTimer(NoTrigBlinkLight, 10);

                //Reset
                ES_Timer_StopTimer(GameTimer);
                ES_Timer_StopTimer(IdleLightTimer);
                ES_PostToService(MyPriority, ReturnEvent); 
            }
            break;
            
            case ES_TIMEOUT:
            {
                if (ThisEvent.EventParam == BlinkLightTimer)
                {
                    if (LightBlinkCount < 13)
                    {
                        if ((LightBlinkCount % 2) == 0)
                        {
                            Light(ZenBlinkModule);
                            ES_Timer_InitTimer(BlinkLightTimer, 300);
                        } else   
                        {
                            Light(No_Module);
                            ES_Timer_InitTimer(BlinkLightTimer, 150);
                        }
                        LightBlinkCount++;
                    } else 
                    {
                        Light(No_Module);
                        ES_Timer_InitTimer(IdleLightTimer, 500);
                    }
                } else if (ThisEvent.EventParam == IdleLightTimer)
                {
                    if ((ZenIdleBlinkCount % 2) == 0)
                    {
                        TouchLight = 1;
                        ShakeLight = 1; 
                        SqueezeLight = 1;
                        WaveLight = 1;
                        ES_Timer_InitTimer(IdleLightTimer,600);
                    } else 
                    {
                        Light(No_Module);
                        ES_Timer_InitTimer(IdleLightTimer, 250);
                    }
                    ZenIdleBlinkCount++;
                } else if (ThisEvent.EventParam == VibrationTimer)
                {
                    if ((ZenVibrationCount % 2) == 0)
                    {
                        VibrationEvent.EventType = StartMotor;
                        PostVibrationFSM(VibrationEvent);
                        ES_Timer_InitTimer(VibrationTimer,1400);
                    } else 
                    {
                        VibrationEvent.EventType = StopMotor;
                        PostVibrationFSM(VibrationEvent);
                        ES_Timer_InitTimer(VibrationTimer, 600);
                    }
                    ZenVibrationCount++;
                } else if (ThisEvent.EventParam == GameTimer)
                {
                    InstructEvent.EventType = ES_STOPINSTRUCT;
                    PostInstructionService(InstructEvent);
                    sprintf(Instruction, "NIRVANA!        ");
                    PostLEDService(LEDEvent);
                    VibrationEvent.EventType = StopMotor;
                    PostVibrationFSM(VibrationEvent);
                    
                    CurrentState = IdleMode;
                    ES_Timer_InitTimer(StateEndTimer, 15000);
                    ES_PostToService(MyPriority, ReturnEvent); 
                    
                } else if (ThisEvent.EventParam == NoTriggerLightTimer)
                {
                    InstructEvent.EventType = ES_STOPINSTRUCT;
                    PostInstructionService(InstructEvent);
                    sprintf(Instruction, "WELCOME!        ");
                    PostLEDService(LEDEvent);
                    VibrationEvent.EventType = StopMotor;
                    PostVibrationFSM(VibrationEvent);
                        
                    CurrentState = IdleMode;
                    ES_Timer_InitTimer(IdleLightTimer, 500);
                    ES_Timer_InitTimer(StateEndTimer, 100);
                    ES_PostToService(MyPriority, ReturnEvent); 
                } else if (ThisEvent.EventParam == NoTrigBlinkLight)
                {
                    if (NoTrigBlinkCount < 50)
                    {
                        if ((NoTrigBlinkCount % 2) == 0)
                        {
                            TouchLight = 1;
                            ShakeLight = 1; 
                            SqueezeLight = 1;
                            WaveLight = 1;
                            ES_Timer_InitTimer(NoTrigBlinkLight, 5);
                        } else   
                        {
                            Light(No_Module);
                            ES_Timer_InitTimer(NoTrigBlinkLight, 5);
                        }
                        NoTrigBlinkCount++;
                    } else 
                    {
                        NoTrigBlinkCount = 0;
                    }
                } 
                
            }
            break;
            
            default:
            ;
        }
    }
    // repeat state pattern as required for other states
    default:
      ;
  }                                   // end switch on Current State
  return ReturnEvent;
}

/****************************************************************************
 Function
     QueryModeServiceFSM

 Parameters
     None

 Returns
     TemplateState_t The current state of the Template state machine

 Description
     returns the current state of the Template state machine

****************************************************************************/
ModeState_t QueryModeServiceFSM(void)
{
  return CurrentState;
}

/***************************************************************************
 private functions
 ***************************************************************************/
/****************************************************************************
 Function
    PlayAudio

 Parameters
    int, The type of Audio you want to play

 Returns
    nothing

 Description
    Pulses the pin for the desired audio that you want to play

 Author
    Mario Peraza
****************************************************************************/
static void PlayAudio(int Audio)
{
    if (Audio == GameAudio)
    {
        DB_printf("FLIP %d\n", !Flip);
        Flip = !Flip;
        GameAudioPin = Flip;
    }
    if (Audio == ZenAudio)
    {
        ZenAudioPin = 0;
        for (int i = 0; i<1000000; i++) {}
        ZenAudioPin = 1;
    }
}

/****************************************************************************
 Function
    Light

 Parameters
    int, The current active module

 Returns
    nothing

 Description
    Lights the corresponding Light for the current active module and shuts the rest off

 Author
    Mario Peraza
****************************************************************************/
static void Light(Module_t Module)
{
    TouchLight = 0;
    SqueezeLight = 0;
    ShakeLight = 0;
    WaveLight = 0;
    if (Module == Touch)
    {
        TouchLight = 1;
    }
    if (Module == Squeeze)
    {
        SqueezeLight = 1;
    }
    if (Module == Shake)
    {
        ShakeLight = 1;
    }
    if (Module == Wave)
    {
        WaveLight = 1;
    }
}

/****************************************************************************
 Function
    LightSingle

 Parameters
    int, The light of the module to be turned on/off
    bool, Turn on light if true, turn off light if false

 Returns
    nothing

 Description
    Turns on or off the lights of the specified module 

 Author
    Peter Shih
****************************************************************************/
static void LightSingle(Module_t Module, bool ON)
{ 
    if (ON)
    {
        if (Module == Touch)
        {
            TouchLight = 1;
        } else if (Module == Squeeze)
        {
            SqueezeLight = 1;
        } else if (Module == Shake)
        {
            ShakeLight = 1;
        } else if (Module == Wave)
        {
            WaveLight = 1;
        }
    } else
    {
        if (Module == Touch)
        {
            TouchLight = 0;
        } else if (Module == Squeeze)
        {
            SqueezeLight = 0;
        } else if (Module == Shake)
        {
            ShakeLight = 0;
        } else if (Module == Wave)
        {
            WaveLight = 0;
        }
    }
    
}

/****************************************************************************
 Function
    RandomModule

 Parameters
    none

 Returns
    int NewModule, the next active module

 Description
    Gets a pseudorandom module to make active

 Author
    Mario Peraza
****************************************************************************/   
static int RandomModule(void)
{
    Module_t NewModule = rand() % 4;
    while (NewModule == CurrentModule)
    {
        NewModule = rand() % 4;
    }
    Light(NewModule);
    return NewModule;
}
