#include <stdint.h>
#include <stdbool.h>

#include "gfx.h"
#include "helpers.h"
#include "forms_manager.h"

#include "gesture_engine.h"

// Assuming g_ui32SysClock is 120,000,000 (120MHz)
extern const uint32_t g_ui32SysClock;

static TouchStatus g_sCurTouchStatus;
static TouchStatus g_sPastTouchStatus;
static gesture_type_e g_eGestureType;

static uint32_t ui32PressStartTicks = 0;
static uint32_t ui32LastPollTicks = 0;

void gestureEngineInit(void) {
    g_sPastTouchStatus.state = false;
    g_eGestureType = GESTURE_EMPTY;
    ui32LastPollTicks = DWTGetCycleCounter();
}

/*
void gestureEngineTask(void) {
    uint32_t ui32CurrentTicks = DWTGetCycleCounter();

    // 1. Non-Blocking Sleep: Execute at 30Hz (~33ms)
    // uint32_t ui32CyclesPerMs = g_ui32SysClock / 1000;
    // if ((ui32CurrentTicks - ui32LastPollTicks) < (ui32CyclesPerMs * 3)) {
    //     return; // Not time yet, return instantly to the main loop
    // }
    
    // Update poll timer for the next execution
    ui32LastPollTicks = ui32CurrentTicks;
    
    // 2. Read hardware
    g_sCurTouchStatus = gfx_touchReadRegion();

    // 3. Simple Gesture State Machine (Click Only)
    if (g_sCurTouchStatus.state == true) {
        
        if (g_sPastTouchStatus.state == false) {
            // RISING EDGE: Finger just touched the screen
            // (We do nothing here for a basic click, just wait for release)
        } else {
            // HOLDING: Finger is down. 
            // (Drag and duration logic removed for now)
        }
        
    } else {
        
        // FALLING EDGE: Finger just left the screen
        if (g_sPastTouchStatus.state == true) {
            
            g_eGestureType = GESTURE_CLICK;
            
            // CRITICAL: Pass the PAST status so the Form Manager knows 
            // the last valid X/Y coordinates of the finger!
            formManagerHandleGesture(g_sPastTouchStatus, g_eGestureType);
            
            g_eGestureType = GESTURE_EMPTY;
        }
    }

    g_sPastTouchStatus = g_sCurTouchStatus;
}*/

void gestureEngineTask(void) {
    uint32_t ui32CurrentTicks = DWTGetCycleCounter();

    // 1. Non-Blocking Sleep: Execute at 30Hz (~33ms)
    // 1 ms = (g_ui32SysClock / 1000) cycles
    uint32_t ui32CyclesPerMs = g_ui32SysClock / 1000;
    if ((ui32CurrentTicks - ui32LastPollTicks) < (ui32CyclesPerMs * 33)) {
        return; // Not time yet, return instantly to the main loop
    }
    
    // Update poll timer for the next execution
    ui32LastPollTicks = ui32CurrentTicks;
	
    // 2. Read hardware (Ensure your SPI guard from earlier is inside this function!)
    g_sCurTouchStatus = gfx_touchReadRegion();

    // 3. The Gesture State Machine
    if (g_sCurTouchStatus.state == true) {
        if (g_sPastTouchStatus.state == false) {
            // First touch detected
            ui32PressStartTicks = ui32CurrentTicks;
        } else {
            // Finger is holding. Calculate duration in milliseconds.
            uint32_t ui32TouchDurationMs = (ui32CurrentTicks - ui32PressStartTicks) / ui32CyclesPerMs;

            // Lock Phase
            if (ui32TouchDurationMs >= 30 && ui32TouchDurationMs < 120) {
                g_eGestureType = GESTURE_LOCK_OBJ;
				formManagerHandleGesture(g_sCurTouchStatus, g_eGestureType);
            }
            // Drag Phase
            else if (ui32TouchDurationMs >= 120) {
                g_eGestureType = GESTURE_DRAG;
				formManagerHandleGesture(g_sCurTouchStatus, g_eGestureType);
            }
        }
    } else {
        // Finger released
        if (g_eGestureType == GESTURE_DRAG) {
            g_eGestureType = GESTURE_RELEASE;
            ui32PressStartTicks = ui32CurrentTicks; // Restart count logic if needed
			formManagerHandleGesture(g_sPastTouchStatus, g_eGestureType);
        } else if (g_sPastTouchStatus.state == true) {
            // It was a quick tap
            g_eGestureType = GESTURE_CLICK;
			formManagerHandleGesture(g_sPastTouchStatus, g_eGestureType);
            g_eGestureType = GESTURE_EMPTY;
        }
    }

    g_sPastTouchStatus = g_sCurTouchStatus;
}

gesture_type_e gestureEngineGetGesture(void)
{
	return g_eGestureType;
}

TouchStatus gestureEngineGetGestureStatus(void)
{
	return g_sCurTouchStatus;
}

