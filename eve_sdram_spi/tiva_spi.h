/*
 * tiva_spi.h
 *
 *  Created on: 15 may. 2020
 *      Author: Zerch
 */

#ifndef TIVA_SPI_H_
#define TIVA_SPI_H_

#include <stdint.h>
#include <stdbool.h>

#include <inc/hw_memmap.h>
#include <driverlib/gpio.h>
#include <driverlib/ssi.h>

#define SPI3_PORT   GPIO_PORTQ_BASE
#define SPI3_CLK    GPIO_PIN_0
#define SPI3_SDO    GPIO_PIN_2
#define SPI3_SDI    GPIO_PIN_3

#define HIGH        0XFF    //Estado alto para los pines
#define LOW         0X00    //Estado bajo para los pines

#define SPI_DISP_CS         GPIO_PIN_1  //PQ1   Chip selector de la pantalla
#define SPI_DISP_INT        GPIO_PIN_3  //PP3   pin que recibe interrupciones de la pantalla
#define SPI_DISP_PD         GPIO_PIN_6  //PM6   pin para despertar a la pantalla
#define SPI_DISP_TURN_OFF   GPIO_PIN_7
#define SPI_COMM_LED        GPIO_PIN_0

#define EVE_CS_LOW()        GPIOPinWrite(GPIO_PORTQ_BASE, SPI_DISP_CS, LOW)
#define EVE_CS_HIGH()       GPIOPinWrite(GPIO_PORTQ_BASE, SPI_DISP_CS, HIGH)
#define EVE_PD_LOW()        GPIOPinWrite(GPIO_PORTM_BASE, SPI_DISP_PD, LOW)
#define EVE_PD_HIGH()       GPIOPinWrite(GPIO_PORTM_BASE, SPI_DISP_PD, HIGH)
#define EVE_TURN_ON_HIGH()  GPIOPinWrite(GPIO_PORTK_BASE, SPI_DISP_TURN_OFF, HIGH)
#define EVE_TURN_ON_LOW()   GPIOPinWrite(GPIO_PORTK_BASE, SPI_DISP_TURN_OFF, LOW)
#define EVE_TURN_LED_ON()   GPIOPinWrite(GPIO_PORTN_BASE, SPI_COMM_LED, HIGH)
#define EVE_TURN_LED_OFF()  GPIOPinWrite(GPIO_PORTN_BASE, SPI_COMM_LED, OFF)

#define SPI_TRX_SIZE    1024

extern uint8_t *pui8SrcBuff;
extern uint8_t *pui8DstBuff;

bool display_SPI_transfer(const uint8_t *pTxBuffer, uint8_t *pRxBuffer, uint32_t count/*, bool bIsBlocking*/);

void init_SPI_screen(void);

#endif /* TIVA_SPI_H_ */
