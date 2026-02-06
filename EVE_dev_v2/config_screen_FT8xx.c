/*
 * pantallaFT8xx.c
 *
 *  Created on: 22 jul. 2020
 *      Author: Zerch
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driverlib/gpio.h"
#include "inc/hw_memmap.h"
#include "tiva_config.h"

#include "EVE.h"
#include "FT8xx.h"

#include "config_screen_FT8xx.h"
#include "tiva_spi.h"
#include "tiva_timer.h"

// Inicializa EVE
void APP_Init(void) {
  uint32_t ramDisplayList = RAM_DL; // Set beginning of display list memory
  uint8_t FT81x_GPIO; // Used for GPIO register

  EVE_CmdWrite(FT81x_ACTIVE, 0x00); // Sends 00 00 00 to wake FT8xx
  delayMs(500);

  // ---------------------- Optional clock configuration ---------------------

  // Un-comment this line if using external crystal. Leave commented if using
  // internal oscillator. Note that most FTDI/BRT modules based on FT800/FT801
  // use an external crystal
  EVE_CmdWrite(0x44, 0x00); // 0x44 = CLKEXT
  EVE_CmdWrite(0x62, 0x00); // 48[MHz]

  // You can also send the host command to set the PLL here if you want to
  // change it from the default of 48MHz (FT80x) or 60MHz (FT81x) The command
  // would be called here before the Active command wakes up the deviceP

  // --------------- Check that FT8xx ready and SPI comms OK -----------------

  while (EVE_MemRead8(REG_ID) !=
         0x7C) // Read REG_ID register (0x302000) until reads 0x7C
  {
  }
  // Ensure CPUreset register reads 0 and so FT8xx is ready
  while (EVE_MemRead8(REG_CPURESET) != 0x00) {
  }

  // ------------------------- Display settings ------------------------------

  EVE_MemWrite16(REG_HSIZE, LCD_WIDTH);
  EVE_MemWrite16(REG_HCYCLE, LCD_H_CYCLE);
  EVE_MemWrite16(REG_HOFFSET, LCD_H_OFFSET);
  EVE_MemWrite16(REG_HSYNC0, LCD_H_SYNC_0);
  EVE_MemWrite16(REG_HSYNC1, LCD_H_SYNC_1);
  EVE_MemWrite16(REG_VSIZE, LCD_HEIGHT);
  EVE_MemWrite16(REG_VCYCLE, LCD_V_CYCLE);
  EVE_MemWrite16(REG_VOFFSET, LCD_V_OFFSET);
  EVE_MemWrite16(REG_VSYNC0, LCD_V_SYNC_0);
  EVE_MemWrite16(REG_VSYNC1, LCD_V_SYNC_1);
  EVE_MemWrite8(REG_SWIZZLE, LCD_SWIZZLE);
  EVE_MemWrite8(REG_PCLK_POL, LCD_P_CLK_POL);
  //---------- INTERRUPCIONES ------
  //    EVE_MemWrite8(REG_INT_EN, 1);
  //    EVE_MemWrite8(REG_INT_MASK, 0xFF);

  FT81x_GPIO = EVE_MemRead8(
      REG_GPIO); // Read the  GPIO register for a read/modify/write operation
  FT81x_GPIO = FT81x_GPIO |
               0x80; // set bit 7 of  GPIO register (DISP) - others are inputs
  EVE_MemWrite16(REG_GPIO,
                 FT81x_GPIO); // Enable the DISP signal to the LCD panel
                              //    EVE_MemWrite8(REG_GPIO, 0x7);
                              //    EVE_MemWrite16(REG_GPIO, 0x80);

  //----------- TOUCH -----------
  EVE_MemWrite8(REG_TOUCH_MODE,
                0x3); // 0x8381);   //Pantalla resistiva 0x0000 capacitiva
  EVE_MemWrite16(REG_TOUCH_RZTHRESH,
                 1200); // 0xFFFF muy sensible // Eliminate any false touches
  EVE_MemWrite8(REG_TOUCH_OVERSAMPLE,
                0xA); // de 0 a 15, entre m�s grande m�s muestreo pero m�s
                      // consumo energ�tico

  //  SSIConfigSetExpClk(SSI3_BASE, SysCtlClockGet(), MODE_C, SSI_MODE_MASTER,
  //  SPEED_SPI3,8);
  EVE_MemWrite32(REG_TOUCH_TRANSFORM_F, 0x00000000);
  EVE_MemWrite32(REG_TOUCH_TRANSFORM_E, 0x00010000);
  EVE_MemWrite32(REG_TOUCH_TRANSFORM_D, 0x00000000);
  EVE_MemWrite32(REG_TOUCH_TRANSFORM_C, 0x00000000);
  EVE_MemWrite32(REG_TOUCH_TRANSFORM_B, 0x00000000);
  EVE_MemWrite32(REG_TOUCH_TRANSFORM_A, 0x00010000);

  // Can move these 2 lines to after the first display list to make the start-up
  // appear cleaner to the user
  EVE_MemWrite8(REG_PCLK,
                LCD_P_CLK); // Now start clocking data to the LCD panel
  EVE_MemWrite8(REG_PWM_DUTY, 127);

  // ---------------------- Touch and Audio settings -------------------------

  EVE_MemWrite8(REG_VOL_PB, ZERO);    // turn recorded audio volume down
  EVE_MemWrite8(REG_VOL_SOUND, ZERO); // turn synthesizer volume down
  EVE_MemWrite16(REG_SOUND, 0x6000);  // set synthesizer to mute

  // ----------------- First Display List to clear screen --------------------

  ramDisplayList = RAM_DL; // start of Display List
  EVE_MemWrite32(
      ramDisplayList,
      0x02000000); // Clear Color RGB sets the colour to clear screen to black

  ramDisplayList += 4; // point to next location
  EVE_MemWrite32(
      ramDisplayList,
      (0x26000000 | 0x00000007)); // Clear 00100110 -------- -------- -----CST
                                  // (C/S/T define which parameters to clear)

  ramDisplayList += 4; // point to next location
  EVE_MemWrite32(ramDisplayList,
                 0x00000000); // DISPLAY command 00000000 00000000 00000000
                              // 00000000 (end of display list)

  EVE_MemWrite32(
      REG_DLSWAP,
      DLSWAP_FRAME); // Swap display list to make the edited one active
}
