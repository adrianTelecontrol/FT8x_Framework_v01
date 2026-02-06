/*
 * tiva_spi.c
 *
 *  Created on: 12/01/2026 
 *      Author: Adrian Pulido
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <ti/drivers/SPI.h>
#include <ti/drivers/GPIO.h>
#include <xdc/runtime/System.h>
#include "inc/hw_memmap.h"

#include <driverlib/gpio.h>

#include "Board.h"

#include "tiva_spi.h"

/* Global SPI3 handle */
static SPI_Handle eveSPIHandle = NULL;

//Inicializa el SPI3
void SPI3_Init(void)
{
    SPI_Params spiParams;

    spiParams.transferMode = SPI_MODE_BLOCKING;
    spiParams.transferTimeout = SPI_WAIT_FOREVER;
    spiParams.bitRate = 12000000;
    spiParams.mode = SPI_MASTER;
    spiParams.frameFormat = SPI_POL0_PHA0;
    spiParams.dataSize = 8; // 8-bit frames

    eveSPIHandle = SPI_open(Board_SPI1, &spiParams);

    if(eveSPIHandle == NULL)
    {
        System_printf("\nError Initializing SPI3 for Display");
    }
}

bool display_SPI_transfer(const uint8_t *pTxBuffer, uint8_t *pRxBuffer, uint32_t count)
{
    SPI_Transaction transaction;
    bool transferOk;

    transaction.count = count;
    transaction.txBuf = (void *)pTxBuffer;
    transaction.rxBuf = (void *)pRxBuffer;

    transferOk = SPI_transfer(eveSPIHandle, &transaction);

    if(!transferOk)
    {
        System_printf("SPI Transfer failed");
        return false;
    }

    return true;

}


void init_SPI_screen(void)
{
    GPIOPinTypeGPIOOutput(GPIO_PORTM_BASE, SPI_DISP_PD);    //PD_pin SPI
    GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, SPI_DISP_CS);    // CS SPI SSI3
    GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, SPI_DISP_TURN_OFF);    //Pin que apaga la pantalla

    EVE_TURN_ON_LOW();
    EVE_CS_LOW();

    SPI3_Init();
}
