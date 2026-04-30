#include <stdbool.h>
#include <stdint.h>

#include "EVE.h"
#include "hal_eeprom.h"
#include "eeprom_map.h"
#include "forms_manager.h"
#include "hal_tft_spi.h"
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

static char *TASK_NAME = "gestureEngine";

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

	if(g_sCurTouchStatus.x == 0 && g_sCurTouchStatus.y == 0)
		g_sCurTouchStatus.state = 0;

    // 3. Simple Gesture State Machine (Click Only)
    if (g_sCurTouchStatus.state == true) {

        if (g_sPastTouchStatus.state == false) {
            // RISING EDGE: Finger just touched the screen
            // (We do nothing here for a basic click, just wait for release)
            g_eGestureType = GESTURE_PRESSED;
            formManagerHandleGesture(g_sCurTouchStatus, g_eGestureType);
        } else {
            // HOLDING: Finger is down.
            // (Drag and duration logic removed for now)
        }

    } else {

        // FALLING EDGE: Finger just left the screen
        if (g_sPastTouchStatus.state == true) {

            g_eGestureType = GESTURE_RELEASE;

            // CRITICAL: Pass the PAST status so the Form Manager knows
            // the last valid X/Y coordinates of the finger!
            formManagerHandleGesture(g_sPastTouchStatus, g_eGestureType);

            g_eGestureType = GESTURE_EMPTY;
        }
    }

    g_sPastTouchStatus = g_sCurTouchStatus;
} */


void gestureEngineTask(void) {
	if(g_bSPI_TransferActive) return;
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

void gestureEngineCalibrateScreen(void) {
    uint32_t flag = 0;
    uint32_t calib_transforms[6];
    
    // USAMOS sizeof() PARA OBTENER LOS BYTES CORRECTOS (4 bytes)
    HAL_EEPROM_readBytes(EEPROM_GET_ADDRESS(isCalibrated), &flag, sizeof(flag));

    if(flag == EEPROM_CALIBRATED_MAGIC_NUMBER) {
        // Restore values (6 elementos * 4 bytes = 24 bytes)
        HAL_EEPROM_readBytes(EEPROM_GET_ADDRESS(touchTransform), calib_transforms, sizeof(calib_transforms));
        EVE_WriteAllCalibrate32(calib_transforms);
    } else {
        gfx_calibrate();

        uint8_t i = 0;
        for(; i < 6; i++) {
            calib_transforms[i] = EVE_ReadCalibrateReg32(i);
        }
        
        // Guardamos la matriz de transformación usando sizeof (24 bytes)
        if (!HAL_EEPROM_writeBytes(EEPROM_GET_ADDRESS(touchTransform), calib_transforms, sizeof(calib_transforms))) {
            TIVA_LOGE(TASK_NAME, "Error escribiendo calibracion en EEPROM!");
        }
        
        uint32_t flagValue = EEPROM_CALIBRATED_MAGIC_NUMBER;
        
        // Guardamos la bandera mágica usando sizeof (4 bytes)
        if (!HAL_EEPROM_writeBytes(EEPROM_GET_ADDRESS(isCalibrated), &flagValue, sizeof(flagValue))) {
             TIVA_LOGE(TASK_NAME, "Error escribiendo flag en EEPROM!");
        }
        
        // Verificación de lectura
        HAL_EEPROM_readBytes(EEPROM_GET_ADDRESS(isCalibrated), &flag, sizeof(flag));
        TIVA_LOGI(TASK_NAME, "Value: %x", flag);
    }
}




