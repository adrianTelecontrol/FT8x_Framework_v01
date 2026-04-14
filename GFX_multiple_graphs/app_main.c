
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inc/hw_epi.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_ssi.h>
#include <inc/hw_types.h>

#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "drivers/pinout.h"

#include "utils/uartstdio.h"

// #include "ft81x_spi_test.c"
#include "EVE.h"
#include "EVE_colors.h"
#include "FT8xx_params.h"
#include "bitmap_parser.h"
#include "ft81x_spi_test.h"
#include "gfx.h"
#include "helpers.h"
// #include "image_loader.h"
#include "font_engine.h"
#include "forms/home_form.h"
#include "forms_manager.h"
#include "gesture_engine.h"
#include "graphics_engine.h"
#include "hal_spi.h"
#include "image_wrapper.h"
#include "sdram_hal.h"
#include "sdspi_hal.h"
#include "tiva_log.h"
#include "event_engine.h"
#include "video_engine.h"
#include "test/control_sim.h"

#include "draw_bitmap.h"
#include "gfx_theme.h"

// Define clock freq
const uint32_t g_ui32SysClock = 120E6;

//*****************************************************************************
//
// Set up the debug level of verbosity
// LV_1 : Just basic log of the current state of the program: Initializers, etc
// LV_2 : Most detailed execution, including states before and after steps
//
//*****************************************************************************
#define DEBUG_LV_1
// #define DEBUG_LV_2

//*****************************************************************************
//
// Enable FT81x SPI communication test level
//
//*****************************************************************************
#define FT81x_SPI_QUICK_TEST
// #define FT81x_SPI_FULL_TEST
// #define ENABLE_SDRAM_TEST

static const char TASK_NAME[] = "main_task";

#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line) {}
#endif


int _system_pre_init(void) {
  MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
                          SYSCTL_CFG_VCO_240),
                         g_ui32SysClock);
  ConfigureEPI();
  return 1;
}

void ConfigureUART(void) {
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

  MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
  MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
  MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

  UARTStdioConfig(0, 115200, g_ui32SysClock);
}

int main(void) {

  // Enable all the ports
  PinoutSet(false, false);

  MAP_SysTickPeriodSet(g_ui32SysClock / 1000);
  MAP_SysTickEnable();
  MAP_SysTickIntEnable();

  // Configure the UART for system output
  ConfigureUART();

  //MAP_IntMasterEnable();
  TIVA_LOGI(TASK_NAME, "Starting application...");

  Gfx_initEngine(LCD_WIDTH, LCD_HEIGHT);

  if (!SDSPI_MountFilesystem()) {
    SysCtlDelay(MS_2_CLK(100));
  }
  TIVA_LOGI(TASK_NAME, "Setting up Screen SPI...");
  HAL_SPI_Init();
  TIVA_LOGI(TASK_NAME, "SPI set up successfully!");
  TIVA_LOGI(TASK_NAME, "Initial FT81x state...");
  SysCtlDelay(MS_2_CLK(500));
  TIVA_LOGI(TASK_NAME, "Awaking screen...");
  API_WakeUpScreen();
  TIVA_LOGI(TASK_NAME, "Screen is awake!");
  SysCtlDelay(MS_2_CLK(1000));
  TIVA_LOGI(TASK_NAME, "Running Quick FT81x SPI verification... ");
  QuickSanityCheck();

  TIVA_LOGI(TASK_NAME, "Clearing the screen to 0x%x color", EVE_PINK);
  EVE_MemWrite8(REG_PWM_DUTY, 128);
  EVE_MemWrite8(REG_CSPREAD, 0);
  gfx_start(0xFF52EE);
  gfx_end();

  //EVE_PlayIntroVideo();
  //gfx_calibrate();

  StartCycleCounter(); // The DWT will be our clock source

  Theme_Init();
  Theme_SetMode(0);

  formManagerInit();

  // Test Unit
  controlSimulatorInit();

  // Send initial full frame
  formManagerComposite(g_pDrawingBuffer);

  Gfx_render();

  gestureEngineInit();

  Event_Post(EVT_CMD_FULL_REPAINT, 0);
  
  while (1) {
	Gfx_RenderTask();
    //formManagerComposite(g_pDrawingBuffer);
  	//Gfx_render();

    gestureEngineTask();

	controlSimulatiorTask();

	Event_Dispatch();
  } 

}

