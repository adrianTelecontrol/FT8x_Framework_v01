/*
 * tiva_spi.c
 *
 *  Created on: 6 jul. 2020
 *      Author: Zerch
 */
#include <stdint.h>
#include <stdbool.h>

#include "tiva_spi.h"
#include "tiva_config.h"

#include <driverlib/ssi.h>
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <inc/hw_memmap.h>

//Inicializa el SPI0
void SPI0_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);     //Habilita el SSI para configurar el SPI3
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI0)){}

    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinConfigure(GPIO_PA4_SSI0XDAT0);
    GPIOPinConfigure(GPIO_PA5_SSI0XDAT1);
    GPIOPinTypeSSI(PORT_SSI0, SPI0_CLK | SPI0_SDI | SPI0_SDO );
    SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), MODE_SPI0, TYPE_SPI0, SPEED_SPI0, BITS_SPI0);    //Esclavo spi
    SSIEnable(SSI0_BASE);           //Habilita el spi
}

//Inicializa el SPI1
void SPI1_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);     //Habilita el SSI para configurar el SPI3
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI1)){}

    GPIOPinConfigure(GPIO_PB5_SSI1CLK);
    GPIOPinConfigure(GPIO_PE4_SSI1XDAT0);
    GPIOPinConfigure(GPIO_PE5_SSI1XDAT1);
    GPIOPinTypeSSI(PORT_SSI1_CLK, SPI1_CLK);
    GPIOPinTypeSSI(PORT_SSI1_SD, SPI1_SDI | SPI1_SDO );
    SSIConfigSetExpClk(SSI1_BASE, SysCtlClockGet(), MODE_SPI1, TYPE_SPI1, SPEED_SPI1, BITS_SPI1);    //Esclavo spi
    SSIEnable(SSI1_BASE);           //Habilita el spi
}

//Inicializa el SPI2
void SPI2_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);     //Habilita el SSI para configurar el SPI3
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI2)){}

    GPIOPinConfigure(GPIO_PD3_SSI2CLK);
    GPIOPinConfigure(GPIO_PD1_SSI2XDAT0);
    GPIOPinConfigure(GPIO_PD0_SSI2XDAT1);
    GPIOPinTypeSSI(PORT_SSI2, SPI2_CLK | SPI2_SDI | SPI2_SDO );
    SSIConfigSetExpClk(SSI2_BASE, SysCtlClockGet(), MODE_SPI2, TYPE_SPI2, SPEED_SPI2, BITS_SPI2);    //Esclavo spi
    SSIEnable(SSI2_BASE);           //Habilita el spi
}

//Inicializa el SPI3
void SPI3_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);     //Habilita el SSI para configurar el SPI3
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI3));



    GPIOPinConfigure(GPIO_PQ0_SSI3CLK);
    GPIOPinConfigure(GPIO_PQ2_SSI3XDAT0);
    GPIOPinConfigure(GPIO_PQ3_SSI3XDAT1);
    GPIOPinTypeSSI(PORT_SSI3, SPI3_CLK | SPI3_SDI | SPI3_SDO );
    //SSIConfigSetExpClk(SSI3_BASE, SysCtlClockGet(), MODE_SPI3, SSI_MODE_MASTER, SPEED_SPI3,8);    //Esclavo spi
    SSIConfigSetExpClk(SSI3_BASE, SysCtlClockGet(), MODE_D, SSI_MODE_MASTER, 400000,8);
    SSIEnable(SSI3_BASE);           //Habilita el spi

    /*
    //Interrupciones por pin spi de la pantalla
    GPIOIntRegister(PUERTO_P, IntPortPP3DisplaySPI);
    GPIOIntTypeSet(PUERTO_P, CS_TIVA_1 , GPIO_LOW_LEVEL);//GPIO_FALLING_EDGE
    //Activa la resistencia
    GPIOPadConfigSet(PUERTO_P, CS_TIVA_1,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
     */
}

void initSPIScreen(void)
{
    //------------------------
    //-------- SPI3 Pantalla
    //------------------------
    //Configuraci�n para la pantalla

    GPIOPinTypeGPIOOutput(PUERTO_M, SPI_PD_PANTALLA);    //PD_pin SPI
    GPIOPinTypeGPIOOutput(PUERTO_Q, SPI3_CS_PANTALLA);    // CS SPI SSI3
    GPIOPinTypeGPIOOutput(PUERTO_K, GPIO_PIN_7);    //Pin que apaga la pantalla

    GPIOPinWrite(PUERTO_M, SPI_PD_PANTALLA, LOW);
    GPIOPinWrite(PUERTO_Q, SPI3_CS_PANTALLA, LOW);
    GPIOPinWrite(PUERTO_K, GPIO_PIN_7, LOW);

    SPI3_Init();
}
