
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "drivers/pinout.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"

#include "utils/uartstdio.h"

#include "helpers.h"
#include "gfx.h"
#include "tiva_spi.h"
#include "EVE.h"
#include "EVE_colors.h"

//*****************************************************************************
//
// The control table used by the uDMA controller.  This table must be aligned
// to a 1024 byte boundary.
//
//*****************************************************************************
#if defined(ewarm)
#pragma data_alignment=1024
uint8_t pui8ControlTable[1024];
#elif defined(ccs)
#pragma DATA_ALIGN(pui8ControlTable, 1024)
uint8_t pui8ControlTable[1024];
#else
uint8_t pui8ControlTable[1024] __attribute__ ((aligned(1024)));
#endif

uint32_t g_ui32SysClock;

uint32_t g_ui32EveColors[] = {EVE_DARK_BLUE, EVE_GREEN, EVE_RED, EVE_YELLOW, EVE_PINK, EVE_DEEP_PURPLE, EVE_THEAL, EVE_MAUVE};

const uint8_t g_ui8EveColorNum = sizeof(g_ui32EveColors) / sizeof(uint32_t); 

#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif


void
ConfigureUART(void)
{
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTStdioConfig(0, 115200, g_ui32SysClock);
}


int
main(void)
{
    // g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
    //                                          SYSCTL_OSC_MAIN |
    //                                          SYSCTL_USE_PLL |
    //                                          SYSCTL_CFG_VCO_240), 120000000);
    g_ui32SysClock = 120000000;

    // PinoutSet(false, false);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
    MAP_IntMasterEnable();

    ConfigureUART();

    UARTprintf("Starting application... \n");

    UARTprintf("Setting up Screen SPI... \n");
    init_SPI_screen();
    UARTprintf("SPI set up successfully! \n");
    SysCtlDelay(MS_2_CLK(500));
    UARTprintf("Awaking screen... \n");
    API_WakeUpScreen();
    UARTprintf("Screen is awake! \n");

    UARTprintf("Setting bg color... \n");
    gfx_start(EVE_GREY);
    UARTprintf("Bg color was set! \n");
    API_CMD_ROTATE(0);
    UARTprintf("Ending cmd list... \n");
    gfx_end();
    EVE_MemWrite8(REG_PWM_DUTY, 128);

    while(1)
    {
        uint32_t ui32BgColor = g_ui32EveColors[rand() % (g_ui8EveColorNum)];
        gfx_start(ui32BgColor);
        API_CMD_ROTATE(0);
        gfx_end();

        UARTprintf("Running...\n");
        SysCtlDelay(MS_2_CLK(1000));

    }
}
