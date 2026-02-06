/*
 * pantallaFT8xx.h
 *
 *  Created on: 22 jul. 2020
 *      Author: Zerch
 */

#ifndef CONFIG_SCREEN_FT8XX_H_
#define CONFIG_SCREEN_FT8XX_H_

#include "stdint.h"
//---------------------------------
//----Configuración para la API----
//---------------------------------
// WQVGA display parameters

//#define LCD_800_480
//#define LCD_480_272
//#define LCD_320_240
#define LCD_800_480_4

#ifdef LCD_800_480
#define LCD_WIDTH           800     // Active width of LCD display
#define LCD_HEIGHT          480     // Active height of LCD display
#define LCD_X               1     // Random value added to porch?
#define LCD_Y               1     // Random value added to porch?
#define LCD_SYSTEM_CLK      48     //Value in MHz
#define LCD_SCREEN_CLK      12     //Value in MHz
#define LCD_HFRONT_PORCH    LCD_X+4     //Value en clk or pixels
#define LCD_HBACK_PORCH     2     //Value en clk or pixels
//#define LCD_HPULSE_WIDTH    4     //Value en clk or pixels
#define LCD_VFRONT_PORCH    LCD_Y+4     //Value en clk or pixels
#define LCD_VBACK_PORCH     2     //Value en clk or pixels
//#define LCD_VPULSE_WIDTH    4     //Value en clk or pixels

#define LCD_H_CYCLE     LCD_WIDTH+LCD_HFRONT_PORCH+LCD_HBACK_PORCH+LCD_X     // Total number of clocks per line
#define LCD_H_SYNC_0    LCD_HFRONT_PORCH-LCD_X   // Start of horizontal sync pulse
#define LCD_H_SYNC_1    LCD_H_SYNC_0*2      // End of horizontal sync pulse
#define LCD_H_OFFSET    LCD_H_SYNC_1+LCD_HBACK_PORCH      // Start of active line
#define LCD_V_CYCLE     LCD_HEIGHT+LCD_VFRONT_PORCH+LCD_VBACK_PORCH+LCD_Y      // Total number of lines per screen
#define LCD_V_SYNC_0    LCD_VFRONT_PORCH-LCD_Y       // Start of vertical sync pulse
#define LCD_V_SYNC_1    LCD_V_SYNC_0*2        // End of vertical sync pulse
#define LCD_V_OFFSET    LCD_V_SYNC_1+LCD_VBACK_PORCH      // Start of active screen
#define LCD_P_CLK       LCD_SYSTEM_CLK/LCD_SCREEN_CLK       // Pixel Clock
#define LCD_SWIZZLE     0       // Define RGB output pins
#define LCD_P_CLK_POL   1       // Define active edge of PCLK
#endif

#ifdef LCD_480_272
#define LCD_WIDTH       480     // Active width of LCD display
#define LCD_HEIGHT      272     // Active height of LCD display
#define LCD_X               4     // Random value added to porch?
#define LCD_Y               4     // Random value added to porch?
#define LCD_SYSTEM_CLK      48     //Value in MHz
#define LCD_SCREEN_CLK      8     //Value in MHz
#define LCD_HFRONT_PORCH    LCD_X+4     //Value en clk or pixels
#define LCD_HBACK_PORCH     43     //Value en clk or pixels
//#define LCD_HPULSE_WIDTH    4     //Value en clk or pixels
#define LCD_VFRONT_PORCH    LCD_Y+4     //Value en clk or pixels
#define LCD_VBACK_PORCH     12     //Value en clk or pixels
//#define LCD_VPULSE_WIDTH    4     //Value en clk or pixels

#define LCD_H_CYCLE     LCD_WIDTH+LCD_HFRONT_PORCH+LCD_HBACK_PORCH+LCD_X     // Total number of clocks per line
#define LCD_H_SYNC_0    LCD_HFRONT_PORCH-LCD_X   // Start of horizontal sync pulse
#define LCD_H_SYNC_1    LCD_H_SYNC_0*2      // End of horizontal sync pulse
#define LCD_H_OFFSET    LCD_H_SYNC_1+LCD_HBACK_PORCH      // Start of active line
#define LCD_V_CYCLE     LCD_HEIGHT+LCD_VFRONT_PORCH+LCD_VBACK_PORCH+LCD_Y      // Total number of lines per screen
#define LCD_V_SYNC_0    LCD_VFRONT_PORCH-LCD_Y       // Start of vertical sync pulse
#define LCD_V_SYNC_1    LCD_V_SYNC_0*2        // End of vertical sync pulse
#define LCD_V_OFFSET    LCD_V_SYNC_1+LCD_VBACK_PORCH      // Start of active screen
#define LCD_P_CLK       LCD_SYSTEM_CLK/LCD_SCREEN_CLK       // Pixel Clock
#define LCD_SWIZZLE     0       // Define RGB output pins
#define LCD_P_CLK_POL   1       // Define active edge of PCLK
#endif

#ifdef LCD_800_480_4
#define LCD_WIDTH       800     // Active width of LCD display
#define LCD_HEIGHT      480     // Active height of LCD display


#define LCD_SYSTEM_CLK      48     //Value in MHz
#define LCD_SCREEN_CLK      12     //Value in MHz

#define LCD_H_OFFSET    75      // Start of active line
#define LCD_H_SYNC_0    16       // Start of horizontal sync pulse
#define LCD_H_SYNC_1    LCD_H_SYNC_0*2      // End of horizontal sync pulse
#define LCD_H_CYCLE     LCD_WIDTH+LCD_H_OFFSET+LCD_H_SYNC_0+LCD_H_SYNC_1     // Total number of clocks per line

#define LCD_V_OFFSET    15      // Start of active screen
#define LCD_V_SYNC_0    3       // Start of vertical sync pulse
#define LCD_V_SYNC_1    LCD_V_SYNC_0*2       // End of vertical sync pulse
#define LCD_V_CYCLE     LCD_HEIGHT+LCD_V_OFFSET+LCD_V_SYNC_0+LCD_V_SYNC_1     // Total number of lines per screen

#define LCD_P_CLK       LCD_SYSTEM_CLK/LCD_SCREEN_CLK       // Pixel Clock
#define LCD_SWIZZLE     0       // Define RGB output pins
#define LCD_P_CLK_POL   1       // Define active edge of PCLK
#endif

//Pin asociado a las interrupciones
// de la pantalla
#define INT_TOUCH_SCREEN        GPIO_PIN_6
//bits de interrupción
#define INT_BIT_SWAP            (0x0001)
#define INT_BIT_TOUCH           (0x0002)
#define INT_BIT_TAG             (0x0004)
#define INT_BIT_SOUND           (0x0008)
#define INT_BIT_PLAYBACK        (0x0010)
#define INT_BIT_CMDEMPTY        (0x0020)
#define INT_BIT_CMDFLAG         (0x0040)
#define INT_BIT_CONVCOMPLETE    (0x0080)

#define FT81x_ACTIVE    0x00

typedef struct FlagInterrupt
{
    uint8_t fSwap;
    uint8_t fTouch;
    uint8_t fTag;
    uint8_t fSound;
    uint8_t fPlayback;
    uint8_t fCMDEmpty;
    uint8_t fCMDFlag;
    uint8_t fConvComplete;
}FlagInterrupt;

FlagInterrupt flagIntScreen;

//Funciones de la API
void APP_Init(void);
//void PD_SEC(void);


#endif /* PANTALLAFT8XX_H_ */
