#ifndef GESTURE_ENGINE_H_
#define GESTURE_ENGINE_H_

typedef enum
{
    GESTURE_EMPTY,
    GESTURE_CLICK,
    GESTURE_DRAG,
    GESTURE_LOCK_OBJ,
    GESTURE_RELEASE,
} gesture_type_e;

void gestureEngineInit(void);

void gestureEngineTask(void);

gesture_type_e gestureEngineGetGesture(void);

TouchStatus gestureEngineGetGestureStatus(void);

#endif 	// GESTURE_ENGINE_H_


