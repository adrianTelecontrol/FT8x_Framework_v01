
#include <stdint.h>
#include <stdbool.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include "config_screen_FT8xx.h"
#include "FT8xx.h"
#include "EVE.h"
#include "EVE_colors.h"
#include "gfx.h"
#include "tiva_config.h"
#include "tiva_spi.h"

#include "forms/home_form.h"
#include "forms/showcase_form.h"

#include "display_task.h"

#define DISPLAY_TASK_STACK_SIZE      1024*10

static Task_Struct g_displayTaskStruct;
static Char g_displayTaskStack[DISPLAY_TASK_STACK_SIZE];
Task_Params displayTaskParams;

void calibrateSCR(void)
{
    API_LIB_BeginCoProList(); // Begin new screen
    API_CMD_DLSTART();
    API_CLEAR_COLOR_RGB(0, 0, 0); // Clear screen
    API_CLEAR(1, 1, 1);
    API_CMD_TEXT(80, 30, 27, OPT_CENTER, "Presiona los puntos...");
    API_CMD_CALIBRATE(0xAAAAAAAA);
    API_DISPLAY();             // Tell EVE that this is end of list
    API_CMD_SWAP();            // Swap buffers in EVE to make this list active
    API_LIB_EndCoProList();    // Finish the co-processor list burst write
    API_LIB_AwaitCoProEmpty(); // Wait until co-processor has consumed all commands
}

void wakeUpScreen(void)
{
    EVE_PD_LOW();
    EVE_TURN_ON_LOW();
    GPIOPinWrite(GPIO_PORTN_BASE, LED2_TIVA, HIGH);
    Task_sleep(100);
    GPIOPinWrite(GPIO_PORTN_BASE, LED2_TIVA, LOW);
    Task_sleep(100);
    GPIOPinWrite(GPIO_PORTN_BASE, LED2_TIVA, HIGH);
    Task_sleep(100);
    GPIOPinWrite(GPIO_PORTN_BASE, LED2_TIVA, LOW);
    Task_sleep(250); //350
    EVE_TURN_ON_HIGH();
    Task_sleep(250); //350
    EVE_PD_HIGH();
    Task_sleep(100); //250
    GPIOPinWrite(GPIO_PORTN_BASE, LED2_TIVA, HIGH);
    APP_Init();
}

void displayTask(UArg arg0, UArg arg1)
{
    System_printf("Starting Display Task");
    System_flush();

    initHomeForm();
    initShowcaseForm();
    
    GPIOPinWrite(PUERTO_F, LED3_TIVA, LOW);
    Task_sleep(1000);
    GPIOPinWrite(PUERTO_F, LED3_TIVA, HIGH);

    // Init Ft81x EVE engine
    wakeUpScreen();
    EVE_MemWrite8(REG_PWM_DUTY, 128);

    gfx_start(EVE_BLACK);
    API_CMD_ROTATE(0);
    gfx_end();

    calibrateSCR();


    //renderHomeForm();
    renderShowcaseForm();

    while (1) 
    {
        // State machine

        // renderHomeForm();
        renderShowcaseForm();
        // renderShowcaseForm();
        Task_sleep(1000 / 25); // 30 fps
        // System_printf("Exec displayTask");
        // System_flush();
    }
}

void displayTaskStart(void)
{
    System_printf("Initializing Display Task");
    System_flush();


    Task_Params_init(&displayTaskParams);
    displayTaskParams.stackSize = DISPLAY_TASK_STACK_SIZE;
    displayTaskParams.stack = &g_displayTaskStack;
    displayTaskParams.instance->name = "displayTsk";
    Task_construct(&g_displayTaskStruct, (Task_FuncPtr)displayTask, &displayTaskParams, NULL);
}

