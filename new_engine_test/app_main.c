
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inc/hw_epi.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>

#include "driverlib/epi.h"
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
#include "bmp_wrapper.h"
#include "ft81x_spi_test.h"
#include "gfx.h"
#include "helpers.h"
// #include "image_loader.h"
#include "graphics_engine.h"
#include "hal_spi.h"
#include "image_wrapper.h"
#include "sdram_hal.h"
#include "sdspi_hal.h"
#include "tiva_log.h"

#include "draw_bitmap.h"

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

//*****************************************************************************
//
// Use the following to specify the GPIO pins used by the SDRAM EPI bus.
//
//*****************************************************************************
#define EPI_PORTA_PINS (GPIO_PIN_7 | GPIO_PIN_6)
#define EPI_PORTB_PINS (GPIO_PIN_3 | GPIO_PIN_2)
#define EPI_PORTC_PINS (GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4)
#define EPI_PORTG_PINS (GPIO_PIN_1 | GPIO_PIN_0)
#define EPI_PORTH_PINS (GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0)
#define EPI_PORTK_PINS (GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4)
#define EPI_PORTL_PINS                                                         \
  (GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0)
#define EPI_PORTM_PINS (GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0)
#define EPI_PORTN_PINS (GPIO_PIN_5 | GPIO_PIN_4)
#define EPI_PORTP_PINS (GPIO_PIN_3 | GPIO_PIN_2)
#define EPI_PORTQ_PINS (GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0)

//*****************************************************************************
//
// Use the following to specify the SDRAM location where the application will
// be downloaded from the SDCARD
//
//*****************************************************************************
#define SDRAM_APP_START_ADDRESS 0x60000000
#define SDRAM_APP_END_ADDRESS 0x61FFFFFF

//*****************************************************************************
//
// Enable SDRAM test to be executed before using it
//
//*****************************************************************************
// #define ENABLE_SDRAM_TEST 1
uint32_t g_ui32EveColors[] = {EVE_DARK_BLUE, EVE_GREEN, EVE_RED,
                              EVE_YELLOW,    EVE_PINK,  EVE_DEEP_PURPLE,
                              EVE_THEAL,     EVE_MAUVE};

const uint8_t g_ui8EveColorNum = sizeof(g_ui32EveColors) / sizeof(uint32_t);

static const char TASK_NAME[] = "main_task";

const char *g_BMP_FILENAME = "TEST5.BMP";

#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line) {}
#endif

//*****************************************************************************
//
// Configure the EPI and its pins.  This must be called before boot command is
// invoked.
//
//*****************************************************************************
int ConfigureEPI(void) {
  //
  // The EPI0 peripheral must be enabled for use.
  //
  SysCtlPeripheralEnable(SYSCTL_PERIPH_EPI0);

  //
  // For this example EPI0 is used with multiple pins on PortA, B, C, G, H,
  // K, L, M and N.  The actual port and pins used may be different on your
  // part, consult the data sheet for more information.
  // TODO: Update based upon the EPI pin assignment on your target part.
  //
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);

  //
  // This step configures the internal pin muxes to set the EPI pins for use
  // with EPI.  Please refer to the datasheet for more information about pin
  // muxing.  Note that EPI0S27:20 are not used for the EPI SDRAM
  // implementation.
  //
  GPIOPinConfigure(GPIO_PH0_EPI0S0);
  GPIOPinConfigure(GPIO_PH1_EPI0S1);
  GPIOPinConfigure(GPIO_PH2_EPI0S2);
  GPIOPinConfigure(GPIO_PH3_EPI0S3);
  GPIOPinConfigure(GPIO_PC7_EPI0S4);
  GPIOPinConfigure(GPIO_PC6_EPI0S5);
  GPIOPinConfigure(GPIO_PC5_EPI0S6);
  GPIOPinConfigure(GPIO_PC4_EPI0S7);
  GPIOPinConfigure(GPIO_PA6_EPI0S8);
  GPIOPinConfigure(GPIO_PA7_EPI0S9);
  GPIOPinConfigure(GPIO_PG1_EPI0S10);
  GPIOPinConfigure(GPIO_PG0_EPI0S11);
  GPIOPinConfigure(GPIO_PM3_EPI0S12);
  GPIOPinConfigure(GPIO_PM2_EPI0S13);
  GPIOPinConfigure(GPIO_PM1_EPI0S14);
  GPIOPinConfigure(GPIO_PM0_EPI0S15);
  GPIOPinConfigure(GPIO_PL0_EPI0S16);
  GPIOPinConfigure(GPIO_PL1_EPI0S17);
  GPIOPinConfigure(GPIO_PL2_EPI0S18);
  GPIOPinConfigure(GPIO_PL3_EPI0S19);
  GPIOPinConfigure(GPIO_PQ0_EPI0S20);
  GPIOPinConfigure(GPIO_PQ1_EPI0S21);
  GPIOPinConfigure(GPIO_PQ2_EPI0S22);
  GPIOPinConfigure(GPIO_PQ3_EPI0S23);
  GPIOPinConfigure(GPIO_PK7_EPI0S24);
  GPIOPinConfigure(GPIO_PK6_EPI0S25);
  GPIOPinConfigure(GPIO_PL4_EPI0S26);
  GPIOPinConfigure(GPIO_PB2_EPI0S27);
  GPIOPinConfigure(GPIO_PB3_EPI0S28);
  GPIOPinConfigure(GPIO_PP2_EPI0S29);
  GPIOPinConfigure(GPIO_PP3_EPI0S30);
  GPIOPinConfigure(GPIO_PK5_EPI0S31);
  GPIOPinConfigure(GPIO_PK4_EPI0S32);
  GPIOPinConfigure(GPIO_PL5_EPI0S33);
  GPIOPinConfigure(GPIO_PN4_EPI0S34);
  GPIOPinConfigure(GPIO_PN5_EPI0S35);

  //
  // Configure the GPIO pins for EPI mode.  All the EPI pins require 8mA
  // drive strength in push-pull operation.  This step also gives control of
  // pins to the EPI module.
  //
  GPIOPinTypeEPI(GPIO_PORTA_BASE, EPI_PORTA_PINS);
  GPIOPinTypeEPI(GPIO_PORTB_BASE, EPI_PORTB_PINS);
  GPIOPinTypeEPI(GPIO_PORTC_BASE, EPI_PORTC_PINS);
  GPIOPinTypeEPI(GPIO_PORTG_BASE, EPI_PORTG_PINS);
  GPIOPinTypeEPI(GPIO_PORTH_BASE, EPI_PORTH_PINS);
  GPIOPinTypeEPI(GPIO_PORTK_BASE, EPI_PORTK_PINS);
  GPIOPinTypeEPI(GPIO_PORTL_BASE, EPI_PORTL_PINS);
  GPIOPinTypeEPI(GPIO_PORTM_BASE, EPI_PORTM_PINS);
  GPIOPinTypeEPI(GPIO_PORTN_BASE, EPI_PORTN_PINS);
  GPIOPinTypeEPI(GPIO_PORTP_BASE, EPI_PORTP_PINS);
  GPIOPinTypeEPI(GPIO_PORTQ_BASE, EPI_PORTQ_PINS);

  uint32_t ui32Strength = GPIO_STRENGTH_8MA;
  uint32_t ui32PinType = GPIO_PIN_TYPE_STD;

  GPIOPadConfigSet(GPIO_PORTA_BASE, EPI_PORTA_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTB_BASE, EPI_PORTB_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTC_BASE, EPI_PORTC_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTG_BASE, EPI_PORTG_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTH_BASE, EPI_PORTH_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTK_BASE, EPI_PORTK_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTL_BASE, EPI_PORTL_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTM_BASE, EPI_PORTM_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTN_BASE, EPI_PORTN_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTP_BASE, EPI_PORTP_PINS, ui32Strength, ui32PinType);
  GPIOPadConfigSet(GPIO_PORTQ_BASE, EPI_PORTQ_PINS, ui32Strength, ui32PinType);

  //
  // Set the EPI clock to half the system clock.
  //
  EPIDividerSet(EPI0_BASE, 4);

  //
  // Sets the usage mode of the EPI module.  For this example we will use
  // the SDRAM mode to talk to the external 64MB SDRAM daughter card.
  //
  EPIModeSet(EPI0_BASE, EPI_MODE_SDRAM);

  //
  // Configure the SDRAM mode.  We configure the SDRAM according to our core
  // clock frequency.  We will use the normal (or full power) operating
  // state which means we will not use the low power self-refresh state.
  // Set the SDRAM size to 64MB with a refresh interval of 468 clock ticks.
  //
  // DO NOT CHANGE UI32REFRESH VALUE!!!!!!!!!!!!!!!!!!!
  EPIConfigSDRAMSet(EPI0_BASE,
                    (EPI_SDRAM_CORE_FREQ_15_30 | EPI_SDRAM_FULL_POWER |
                     EPI_SDRAM_SIZE_512MBIT),
                    234);

  //
  // Set the address map.  The EPI0 is mapped from 0x60000000 to 0x01FFFFFF.
  // For this example, we will start from a base address of 0x60000000 with
  // a size of 256MB.  Although our SDRAM is only 64MB, there is no 64MB
  // aperture option so we pick the next larger size.
  //
  EPIAddressMapSet(EPI0_BASE, EPI_ADDR_RAM_SIZE_256MB | EPI_ADDR_RAM_BASE_6);

  //
  // Wait for the SDRAM wake-up to complete by polling the SDRAM
  // initialization sequence bit.  This bit is true when the SDRAM interface
  // is going through the initialization and false when the SDRAM interface
  // it is not in a wake-up period.
  //
  while (HWREG(EPI0_BASE + EPI_O_STAT) & EPI_STAT_INITSEQ) {
  }

  //
  // Write to the first 2 and last 2 address of the SDRAM card.  Since the
  // SDRAM card is word addressable, we will write words.
  //
  HWREGH(SDRAM_APP_START_ADDRESS) = 0xabcd;
  HWREGH(SDRAM_APP_START_ADDRESS + 0x2) = 0x1234;
  HWREGH(SDRAM_APP_END_ADDRESS - 0x3) = 0xdcba;
  HWREGH(SDRAM_APP_END_ADDRESS - 0x1) = 0x4321;

  if ((HWREGH(SDRAM_APP_START_ADDRESS) == 0xabcd) &&
      (HWREGH(SDRAM_APP_START_ADDRESS + 0x2) == 0x1234) &&
      (HWREGH(SDRAM_APP_END_ADDRESS - 0x3) == 0xdcba) &&
      (HWREGH(SDRAM_APP_END_ADDRESS - 0x1) == 0x4321)) {
    //
    // Read and write operations were successful.  Return with no errors.
    //
    return (1);
  } else {
    //
    // Read and write operations were failure.  Return with error.
    //
    return (0);
  }
}

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

void task01(void) { TIVA_LOGI(TASK_NAME, "Task01 executing!"); }

void task02(void) { TIVA_LOGI(TASK_NAME, "Task02 executing!"); }

int main(void) {

  // Enable all the ports
  PinoutSet(false, false);

  MAP_SysTickPeriodSet(g_ui32SysClock / 1000);
  MAP_SysTickEnable();
  MAP_SysTickIntEnable();

  // Configure the UART for system output
  ConfigureUART();

  MAP_IntMasterEnable();
  // Initial message
  UARTprintf("\n");
  TIVA_LOGI(
      TASK_NAME,
      "\t\t-------------------------------------------------------------");
  TIVA_LOGI(
      TASK_NAME,
      "\t\t---------------------STARTING APPLICATION--------------------");
  TIVA_LOGI(
      TASK_NAME,
      "\t\t-------------------------------------------------------------\n");
  TIVA_LOGI(TASK_NAME, "Starting application...");

  Gfx_initEngine(LCD_WIDTH, LCD_HEIGHT);

  // Test if the SDRAM is working
#ifdef ENABLE_SDRAM_TEST
  TIVA_LOGI(TASK_NAME, "Starting EPI SDRAM Verification");
  uint32_t *pCheck = (uint32_t *)0x60000000;
  *pCheck = 0xDEADBEEF;
  if (*pCheck == 0xDEADBEEF) {
    // Safe to use malloc now
    TIVA_LOGI(TASK_NAME, "SDRAM verification correct!");
  } else {
    TIVA_LOGE(TASK_NAME,
              "Failed initial SDRAM verification! Going into halt state.");
    while (1) {
    }
  }
  bool ret =
      SDRAM_Test((uint32_t)SDRAM_APP_START_ADDRESS, (uint32_t)0x100, 0.5);
  if (!ret) {
    TIVA_LOGE(TASK_NAME,
              "Failed extensive SDRAM verification! Going into halt state.");
    while (1) {
    }
  }
#endif // ENABLE_SDRAM_TEST

  TIVA_LOGI(TASK_NAME, "Setting up Screen SPI...");
  HAL_SPI_Init();
  TIVA_LOGI(TASK_NAME, "SPI set up successfully!");
  TIVA_LOGI(TASK_NAME, "Initial FT81x state...");
#ifdef DEBUG_LV_2
  EVE_Util_DebugReport();
#endif
  SysCtlDelay(MS_2_CLK(500));
  TIVA_LOGI(TASK_NAME, "Awaking screen...");
  API_WakeUpScreen();
  TIVA_LOGI(TASK_NAME, "Screen is awake!");
  SysCtlDelay(MS_2_CLK(1000));
#ifdef FT81x_SPI_QUICK_TEST
  TIVA_LOGI(TASK_NAME, "Running Quick FT81x SPI verification... ");
  QuickSanityCheck();
#endif // FT81x_SPI_QUICK_TEST
#ifdef FT81x_SPI_FULL_TEST
  TIVA_LOGI(TASK_NAME, "Running Full Ft81x SPI verification... ");
  RunAllSPITests();
#endif

#ifdef DEBUG_LV_2
  EVE_Util_DebugReport();
#endif
  TIVA_LOGI(TASK_NAME, "Clearing the screen to 0x%x color", EVE_PINK);
  gfx_start(EVE_PINK);
  gfx_end();
  EVE_MemWrite8(REG_PWM_DUTY, 128);
#ifdef DEBUG_LV_2
  TIVA_LOGI(TASK_NAME, "FT81x State before loading the image\n");
  EVE_Util_DebugReport();
#endif
  /*
    BitmapHandler_t sBitmapHandler;

    bool ret = SDSPI_MountFilesystem();
    if (ret) {
      SDSPI_pwd(0, NULL);
      SDSPI_ls(0, NULL);
      // int iFetchFileStatus = SDSPI_FetchFile(g_BMP_FILENAME,
    g_ui32BitmapSDRAM,
      // BMP_TEST_1_SIZE);
      int iFetchFileStatus =
          SDSPI_FetchBitmap(g_BMP_FILENAME, &sBitmapHandler, 800*480*2);
    }
  */
  initializeSquaresPhysics(); // Initialize squares

  while (1) {
    task01();

    // #ifdef MEASURE_PERF_ENABLE // Start sending time counter
    // #endif
	StartCycleCounter();
    drawSquares();

	uint32_t ui32ProcCycles = DWTGetCycleCounter();
	Gfx_setProcessDuration(ui32ProcCycles / ( g_ui32SysClock / 1E6 ) / 1000);
	DWTResetCycleCounter();

    Gfx_render();

    task02();
  }
}
