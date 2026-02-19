
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
#include "gfx.h"
#include "graphics_engine.h"
#include "helpers.h"
#include "scatter_gather_test.h"
#include "tiva_log.h"
#include "tiva_spi.h"

const uint32_t g_ui32SysClock = 120E6;

#define DEBUG_LV_1

#define FT81x_SPI_QUICK_TEST

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

#define SDRAM_APP_START_ADDRESS 0x60000000
#define SDRAM_APP_END_ADDRESS 0x61FFFFFF

uint32_t g_ui32EveColors[] = {EVE_DARK_BLUE, EVE_GREEN, EVE_RED,
                              EVE_YELLOW,    EVE_PINK,  EVE_DEEP_PURPLE,
                              EVE_THEAL,     EVE_MAUVE};

const uint8_t g_ui8EveColorNum = sizeof(g_ui32EveColors) / sizeof(uint32_t);

static const char TASK_NAME[] = "main_task";

const char *g_BMP_FILENAME = "TEST4.BMP";

#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line) {}
#endif

int ConfigureEPI(void) {
  SysCtlPeripheralEnable(SYSCTL_PERIPH_EPI0);

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

  EPIDividerSet(EPI0_BASE, 4);

  EPIModeSet(EPI0_BASE, EPI_MODE_SDRAM);

  // DO NOT CHANGE UI32REFRESH VALUE!!!!!!!!!!!!!!!!!!!
  EPIConfigSDRAMSet(EPI0_BASE,
                    (EPI_SDRAM_CORE_FREQ_15_30 | EPI_SDRAM_FULL_POWER |
                     EPI_SDRAM_SIZE_512MBIT),
                    234);

  EPIAddressMapSet(EPI0_BASE, EPI_ADDR_RAM_SIZE_256MB | EPI_ADDR_RAM_BASE_6);

  while (HWREG(EPI0_BASE + EPI_O_STAT) & EPI_STAT_INITSEQ) {
  }

  HWREGH(SDRAM_APP_START_ADDRESS) = 0xabcd;
  HWREGH(SDRAM_APP_START_ADDRESS + 0x2) = 0x1234;
  HWREGH(SDRAM_APP_END_ADDRESS - 0x3) = 0xdcba;
  HWREGH(SDRAM_APP_END_ADDRESS - 0x1) = 0x4321;

  if ((HWREGH(SDRAM_APP_START_ADDRESS) == 0xabcd) &&
      (HWREGH(SDRAM_APP_START_ADDRESS + 0x2) == 0x1234) &&
      (HWREGH(SDRAM_APP_END_ADDRESS - 0x3) == 0xdcba) &&
      (HWREGH(SDRAM_APP_END_ADDRESS - 0x1) == 0x4321)) {
    return (1);
  } else {
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

void DisplayBitmap(void) {
  uint16_t ui16Width = 100;
  uint16_t ui16Height = 100;
  TIVA_LOGI(TASK_NAME, "Bitmap finished to load into RAM_G");

  API_LIB_BeginCoProList();
  API_CMD_DLSTART();
  API_CLEAR_COLOR_RGB(11, 19, 30);
  API_CLEAR(1, 1, 1);
  API_COLOR_RGB(255, 255, 255);

  API_BITMAP_HANDLE(0);
  API_BITMAP_SOURCE(RAM_G);
  uint16_t BytesPerPixel = 1; // RGB565 is 2 bytes
  // uint16_t ui16Stride = ((ui16Width * BytesPerPixel) + 3) & ~3;
  uint16_t ui16Stride = ui16Width * BytesPerPixel;

  // 1. Calculate Target Dimensions
  uint8_t u8Scale = 4;
  uint16_t u16DrawnWidth = ui16Width * u8Scale;
  uint16_t u16DrawnHeight = ui16Height * u8Scale;

  // 2. BITMAP_LAYOUT (Remains based on the SOURCE image size)
  API_BITMAP_LAYOUT(RGB332, ui16Stride, ui16Height);
  API_BITMAP_LAYOUT_H(ui16Stride >> 10, ui16Height >> 9);

  // 3. BITMAP_SIZE (Update this to the NEW scaled size on screen)
  //    We use NEAREST for clean integer scaling (pixel art look).
  //    Use BILINEAR if you want it smoothed/blurry.
  API_BITMAP_SIZE(NEAREST, BORDER, BORDER, u16DrawnWidth, u16DrawnHeight);
  API_BITMAP_SIZE_H(u16DrawnWidth >> 9, u16DrawnHeight >> 9);

  // 4. TRANSFORM MATRIX
  //    We calculate the sampling step.
  //    To make it 4x BIGGER, we step 0.25x per pixel.
  //    16.16 Fixed Point: 65536 / 4 = 16384
  int32_t s32ScaleFactor = 65536 * u8Scale;

  API_CMD_LOADIDENTITY();
  API_CMD_SCALE(s32ScaleFactor, s32ScaleFactor);
  API_CMD_SETMATRIX();

  // 5. Draw
  API_BEGIN(BITMAPS);
  API_VERTEX2II(0, 0, 0, 0);
  API_END();

#ifdef MEASURE_PERF_ENABLE
  API_COLOR_RGB(255, 0, 0);
  API_CMD_NUMBER(LCD_WIDTH - 100, LCD_HEIGHT - 50, 30, 0, g_ui32ExecDurMs);
  // API_CMD_NUMBER(LCD_WIDTH / 2, LCD_HEIGHT / 2, 29, 0, g_ui32ExecDurMs);
  API_CMD_SETBASE(10);
#endif
  API_DISPLAY();
  API_CMD_SWAP();
  API_LIB_EndCoProList();
  API_LIB_AwaitCoProEmpty();
}

int main(void) {

  // Enable all the ports
  PinoutSet(false, false);

  // Configure the UART for system output
  ConfigureUART();

  MAP_IntMasterEnable();
  TIVA_LOGI(TASK_NAME, "Starting application...");

  Gfx_initEngine(LCD_WIDTH, LCD_HEIGHT);
  init_SPI_screen();
  SysCtlDelay(MS_2_CLK(500));
  API_WakeUpScreen();
  SysCtlDelay(MS_2_CLK(1000));
  TIVA_LOGI(TASK_NAME, "Clearing the screen to 0x%x color", EVE_PINK);
  gfx_start(EVE_PINK);
  gfx_end();
  EVE_MemWrite8(REG_PWM_DUTY, 128);

  // Initialize test
  Init_Buffers();
  Setup_SG_Transfer();

  
  while (1) {
	EVE_CS_LOW();
    EVE_AddrForWr(RAM_G);
    
	SPI_Start_SG_Transfer();
	while (!g_bSPI_TransferDone);
	EVE_CS_HIGH();
	DisplayBitmap();

	SysCtlDelay(MS_2_CLK(100));
  }
}
