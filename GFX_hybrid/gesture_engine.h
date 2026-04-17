#ifndef GESTURE_ENGINE_H_
#define GESTURE_ENGINE_H_

#define EEPROM_CALIBRATED_MAGIC_NUMBER	0xFB82

typedef enum
{
    GESTURE_EMPTY,
	GESTURE_PRESSED,
    GESTURE_RELEASE,
    GESTURE_DRAG,
    GESTURE_LOCK_OBJ,
} gesture_type_e;

void gestureEngineInit(void);

void gestureEngineTask(void);

gesture_type_e gestureEngineGetGesture(void);

void gestureEngineCalibrateScreen(void);

// TouchStatus gestureEngineGetGestureStatus(void);

#endif 	// GESTURE_ENGINE_H_


