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

#include <ti/drivers/SPI.h>
#include <ti/drivers/GPIO.h>

#define HIGH        0XFF    //Estado alto para los pines
#define LOW         0X00    //Estado bajo para los pines

#define SPI_DISP_CS         GPIO_PIN_1  //PQ1   Chip selector de la pantalla
#define SPI_DISP_INT        GPIO_PIN_3  //PP3   pin que recibe interrupciones de la pantalla
#define SPI_DISP_PD         GPIO_PIN_6  //PM6   pin para despertar a la pantalla
#define SPI_DISP_TURN_OFF   GPIO_PIN_7

#define EVE_CS_LOW()        GPIOPinWrite(GPIO_PORTQ_BASE, SPI_DISP_CS, LOW)
#define EVE_CS_HIGH()       GPIOPinWrite(GPIO_PORTQ_BASE, SPI_DISP_CS, HIGH)
#define EVE_PD_LOW()        GPIOPinWrite(GPIO_PORTM_BASE, SPI_DISP_PD, LOW)
#define EVE_PD_HIGH()       GPIOPinWrite(GPIO_PORTM_BASE, SPI_DISP_PD, HIGH)
#define EVE_TURN_ON_HIGH()  GPIOPinWrite(GPIO_PORTK_BASE, SPI_DISP_TURN_OFF, HIGH)
#define EVE_TURN_ON_LOW()   GPIOPinWrite(GPIO_PORTK_BASE, SPI_DISP_TURN_OFF, LOW)

bool display_SPI_transfer(const uint8_t *pTxBuffer, uint8_t *pRxBuffer, uint32_t count);

void init_SPI_screen(void);

#endif /* TIVA_SPI_H_ */
