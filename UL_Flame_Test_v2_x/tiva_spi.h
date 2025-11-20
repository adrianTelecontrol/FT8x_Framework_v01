/*
 * tiva_spi.h
 *
 *  Created on: 15 may. 2020
 *      Author: Zerch
 */

#ifndef TIVA_SPI_H_
#define TIVA_SPI_H_


//SPI3
#define SPI3_CLK    GPIO_PIN_0  //PQ0
#define SPI3_SDO    GPIO_PIN_2  //PQ2
#define SPI3_SDI    GPIO_PIN_3  //PQ3

//-------------------------------------------------
//----------- PANTALLAS 4DGL ----------------------
//-------------------------------------------------
//Compatibilidad con la pantalla uLCD-7DT de 4dgl
#define MODE_A              SSI_FRF_MOTO_MODE_3
#define MODE_B              SSI_FRF_MOTO_MODE_1
#define MODE_C              SSI_FRF_MOTO_MODE_2
#define MODE_D              SSI_FRF_MOTO_MODE_0


//------------------------------------------
//------------------ SPI0 ------------------
//------------------------------------------
#define PORT_SSI0       PUERTO_A
#define SPI0_CLK        GPIO_PIN_2  //PA2
#define SPI0_SDO        GPIO_PIN_4  //PA4
#define SPI0_SDI        GPIO_PIN_5  //PA5

//configuración del puerto
#define MODE_SPI0       MODE_D          //SPI0 en modo D
#define TYPE_SPI0       SSI_MODE_MASTER //Tipo de dispositivo
#define SPEED_SPI0      410000          //Velocidad de la transmisión
#define BITS_SPI0       8               //Tamaño de dato a enviar
//------------------------------------------
//------------------ SPI1 ------------------
//------------------------------------------
#define PORT_SSI1_CLK   PUERTO_B
#define PORT_SSI1_SD    PUERTO_E
#define SPI1_CLK        GPIO_PIN_5  //PB5
#define SPI1_SDO        GPIO_PIN_4  //PE4
#define SPI1_SDI        GPIO_PIN_5  //PE5

//configuración del puerto
#define MODE_SPI1       MODE_D          //SPI1 en modo D
#define TYPE_SPI1       SSI_MODE_MASTER //Tipo de dispositivo
#define SPEED_SPI1      410000          //Velocidad de la transmisión
#define BITS_SPI1       8               //Tamaño de dato a enviar
//------------------------------------------
//------------------ SPI2 ------------------
//------------------------------------------
#define PORT_SSI2       PUERTO_D
#define SPI2_CLK        GPIO_PIN_3  //PD3
#define SPI2_SDO        GPIO_PIN_1  //PD1
#define SPI2_SDI        GPIO_PIN_0  //PD0

//configuración del puerto
#define MODE_SPI2       MODE_D          //SPI2 en modo D
#define TYPE_SPI2       SSI_MODE_MASTER //Tipo de dispositivo
#define SPEED_SPI2      410000          //Velocidad de la transmisión
#define BITS_SPI2       8               //Tamaño de dato a enviar


//------------------------------------------
//------------------ SPI3 ------------------
//------------------------------------------
//Seleccionar el puerto deseado como SSI3
#define PQ_SSI3         //Puerto Q
//#define PF_SSI3         //Puerto F

#ifdef PQ_SSI3
    #define PORT_SSI3   PUERTO_Q
    #define SPI3_CLK    GPIO_PIN_0  //PQ0
    #define SPI3_SDO    GPIO_PIN_2  //PQ2
    #define SPI3_SDI    GPIO_PIN_3  //PQ3
#elif  defined(PF_SSI3)
    #define PORT_SSI3   PUERTO_F
    #define SPI3_CLK    GPIO_PIN_3  //PF3
    #define SPI3_SDO    GPIO_PIN_1  //PF1
    #define SPI3_SDI    GPIO_PIN_0  //PF0
#endif


#define SPI3_CS_PANTALLA    GPIO_PIN_1  //PQ1   Chip selector de la pantalla
#define SPI_INT_PANTALLA    GPIO_PIN_3  //PP3   pin que recibe interrupciones de la pantalla
#define SPI_PD_PANTALLA     GPIO_PIN_6  //PM6   pin para despertar a la pantalla

//-------------------------------------------------
//---- CONFIGURACIÓN ESPECÍFICA DEL PROYECTO ------
//-------------------------------------------------
#define MODE_SPI3           MODE_D          //Configuración del spi0 en modo A
#define SPEED_SPI3          100000//410000// 500000//500000// 21880         //Velocidad de la transmisión AVS
#define MAX_TAM_VAL_SPI     0x7FFF          //Valor máximo a enviar por SPI
#define DECIMALES_SPI       100             //Factor para obtener los decimales (10), centenas(100), milésimas(1000), etc.


uint32_t countIntentSPI;            //Conteo de intentos en SPI
uint8_t flagSPIWithoutResponse, flagFifoUnresponded;

void initSPIScreen(void);

#endif /* TIVA_SPI_H_ */
