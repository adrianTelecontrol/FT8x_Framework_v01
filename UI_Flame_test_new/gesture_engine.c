#include <stdbool.h>
#include <stdint.h>


#include "forms_manager.h"
#include "gfx.h"
#include "helpers.h"


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
    uint32_t ui32CyclesPerMs = g_ui32SysClock / 1000;
    if ((ui32CurrentTicks - ui32LastPollTicks) < (ui32CyclesPerMs * 33)) {
        return; 
    }
    ui32LastPollTicks = ui32CurrentTicks;

    // 2. Read hardware
    g_sCurTouchStatus = gfx_touchReadRegion();

    // 3. The Gesture State Machine
    if (g_sCurTouchStatus.state == true) {
        
        // --- FLANCO DE SUBIDA (Rising Edge) ---
        if (g_sPastTouchStatus.state == false) {
            ui32PressStartTicks = ui32CurrentTicks;
            g_eGestureType = GESTURE_PRESSED;
            formManagerHandleGesture(g_sCurTouchStatus, GESTURE_PRESSED);
        } 
        // --- FASE DE MANTENIMIENTO (Holding / Dragging) ---
        else {
            uint32_t ui32TouchDurationMs = (ui32CurrentTicks - ui32PressStartTicks) / ui32CyclesPerMs;

            if (ui32TouchDurationMs >= 120) {
                g_eGestureType = GESTURE_DRAG;
                // DRAG hace "spam" intencionalmente para actualizar coordenadas X,Y continuas
                formManagerHandleGesture(g_sCurTouchStatus, GESTURE_DRAG);
            } 
            else if (ui32TouchDurationMs >= 30) {
                if (g_eGestureType != GESTURE_LOCK_OBJ) {
                    g_eGestureType = GESTURE_LOCK_OBJ;
                    formManagerHandleGesture(g_sCurTouchStatus, GESTURE_LOCK_OBJ);
                }
            }
        }
    } 
    else {
        // --- FLANCO DE BAJADA (Falling Edge) ---
        if (g_sPastTouchStatus.state == true) {
            // El dedo acaba de soltar la pantalla.
            // Mandamos un RELEASE universal. El Form Manager o el Widget decidirán 
            // si esto fue un click exitoso o si simplemente se soltó un arrastre.
            g_eGestureType = GESTURE_RELEASE;
            formManagerHandleGesture(g_sPastTouchStatus, GESTURE_RELEASE);
            
            // Reiniciamos al estado vacío
            g_eGestureType = GESTURE_EMPTY; 
        }
    }

    g_sPastTouchStatus = g_sCurTouchStatus;
}

gesture_type_e gestureEngineGetGesture(void) { return g_eGestureType; }

TouchStatus gestureEngineGetGestureStatus(void) { return g_sCurTouchStatus; }
