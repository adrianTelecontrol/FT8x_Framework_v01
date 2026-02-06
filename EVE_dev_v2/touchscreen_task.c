
/* XDC header files */
#include <xdc/runtime/System.h>

/* BIOS headers files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>

/* FT/EVE engine */
#include "config_screen_FT8xx.h"
#include "FT8xx.h"
#include "EVE.h"
#include "EVE_colors.h"
#include "gfx.h"

#include "forms/home_form.h"
#include "forms/showcase_form.h"
#include "touchScreen_task.h"

#define TOUCHSCREEN_TASK_STACK_SIZE      1024*4

static Task_Struct g_touchScreenTaskStruct;
static Char g_touchScreenTaskStack[TOUCHSCREEN_TASK_STACK_SIZE];
Task_Params touchScreenTaskParams;

static TouchStatus g_sCurTouchStatus;
static TouchStatus g_sPastTouchStatus;
static gesture_type_e g_eGestureType;
static Clock_Struct g_clkGestureObj;

Void clkGestureCallbackFxn(UArg arg0)
{

}

Void touchScreenTask(UArg arg0, UArg arg1)
{
    UInt32 ui32CurrentTicks, ui32PressStartTicks, ui32TouchDuration = 0;

    while(1)
    {
        g_sCurTouchStatus = gfx_touchReadRegion();

        // Machine State
        if(g_sCurTouchStatus.state == true)
        {
            if(g_sPastTouchStatus.state == false)
            {
                ui32PressStartTicks = Clock_getTicks();
            }
            else 
            {
                ui32CurrentTicks = Clock_getTicks();
                ui32TouchDuration = ui32CurrentTicks - ui32PressStartTicks;
                /* The steps for a dragging gesture is first lock the widget up */
                // Lock
                if(ui32TouchDuration >= 30 && ui32TouchDuration < 120) // 50ms
                {
                    g_eGestureType = GESTURE_LOCK_OBJ;
                    // handleHomeFormTouch(g_sCurTouchStatus, g_eGestureType);
                    handleShowcaseFormTouch(g_sCurTouchStatus, g_eGestureType);
                }
                // Drag
                else if(ui32TouchDuration >= 120) // 200ms
                {
                    g_eGestureType = GESTURE_DRAG;
                    // handleHomeFormTouch(g_sCurTouchStatus, g_eGestureType);
                    handleShowcaseFormTouch(g_sCurTouchStatus, g_eGestureType);
                }

            }
        }
        else 
        {
            if(g_eGestureType == GESTURE_DRAG)
            {
                g_eGestureType = GESTURE_RELEASE;
                ui32PressStartTicks = ui32CurrentTicks; // REstart count
                // handleHomeFormTouch(g_sCurTouchStatus, g_eGestureType);
                handleShowcaseFormTouch(g_sCurTouchStatus, g_eGestureType);
            }
            else if(g_sPastTouchStatus.state)    // Previously was pressed
            {
                g_eGestureType = GESTURE_CLICK;
                // handleHomeFormTouch(g_sPastTouchStatus, g_eGestureType);
                handleShowcaseFormTouch(g_sPastTouchStatus, g_eGestureType);
                g_eGestureType = GESTURE_EMPTY;
            }
        }

        g_sPastTouchStatus = g_sCurTouchStatus;
        Task_sleep(1000 / 30); // 30 hz
    }
}

void touchScreenTaskStart(void)
{
    System_printf("Starting touchScreen task");
    System_flush();

    Task_Params_init(&touchScreenTaskParams);
    touchScreenTaskParams.stackSize = TOUCHSCREEN_TASK_STACK_SIZE;
    touchScreenTaskParams.stack = &g_touchScreenTaskStack;
    Task_construct(&g_touchScreenTaskStruct, (Task_FuncPtr)touchScreenTask, &touchScreenTaskParams, NULL);
}



