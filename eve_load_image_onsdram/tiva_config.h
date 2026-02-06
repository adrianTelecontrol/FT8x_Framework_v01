/*
 * tiva_config.h
 *
 *  Created on: 15 may. 2020
 *      Author: Zerch
 *
 *      Configuraciones generales para la tarjeta
 *      TIVA launchpad
 */

#ifndef TIVA_CONFIG_H_
#define TIVA_CONFIG_H_
#include "math.h"
#include "inc/hw_memmap.h"

//-------------------------------------------------
//----------- DEFINICIONES GENERALES --------------
//-------------------------------------------------
//Renombramiento de los puertos
#define PUERTO_A    GPIO_PORTA_AHB_BASE
#define PUERTO_B    GPIO_PORTB_AHB_BASE
#define PUERTO_C    GPIO_PORTC_AHB_BASE
#define PUERTO_D    GPIO_PORTD_AHB_BASE
#define PUERTO_E    GPIO_PORTE_AHB_BASE
#define PUERTO_F    GPIO_PORTF_AHB_BASE
#define PUERTO_G    GPIO_PORTG_AHB_BASE
#define PUERTO_H    GPIO_PORTH_AHB_BASE
#define PUERTO_J    GPIO_PORTJ_AHB_BASE
#define PUERTO_K    GPIO_PORTK_BASE
#define PUERTO_L    GPIO_PORTL_BASE
#define PUERTO_M    GPIO_PORTM_BASE
#define PUERTO_N    GPIO_PORTN_BASE
#define PUERTO_P    GPIO_PORTP_BASE
#define PUERTO_Q    GPIO_PORTQ_BASE
#define PUERTO_R    GPIO_PORTR_BASE
#define PUERTO_S    GPIO_PORTS_BASE
#define PUERTO_T    GPIO_PORTT_BASE

//Estádos lógicos
#define HIGH        0XFF    //Estado alto para los pines
#define LOW         0X00    //Estado bajo para los pines
//Estado negados para las salidas de uln2803 porque en la salida se invierte la lógica sobre el monster
#define HIGH_NOT    0x00    //Estado Alto
#define LOW_NOT     0xFF    //Estado Bajo

#define SUBIR       1       //Dirección
#define BAJAR       0       //
#define DERECHA     1
#define IZQUIERDA   0

#define PI         3.14159265358979323846

//-------------------------------
//-------- HARDWARE TIVA --------
//-------------------------------
//Leds sobre la TIVA
#define LED1_TIVA   GPIO_PIN_1  //PN1
#define LED2_TIVA   GPIO_PIN_0  //PN0
#define LED3_TIVA   GPIO_PIN_4  //PF4
#define LED4_TIVA   GPIO_PIN_0  //PF0
//Push Button sobre la TIVA
#define PUSH1_TIVA  GPIO_PIN_0  //PJ0
#define PUSH2_TIVA  GPIO_PIN_1  //PJ1



//-------------------------------------------------
//---- CONFIGURACIÓN ESPECÍFICA DEL PROYECTO ------
//-------------------------------------------------

//-------------------------------
//----------  ADC  --------------
//-------------------------------
#define SENS_FLUJO_1    GPIO_PIN_0  //AIN16 PK0
#define ADC_MAX_VOLT    3.3         //Máximo valor de voltaje en flujo
#define ADC_MAX_VALOR   4095        //Máximo valor de conversión del ADC
#define FSFR            200         //Constante que establece el sensor
#define R1              16.666      //Valor de la resistecia 1 en kohms de la etapa de amplificación
#define R2              32.8        //Valor de la resistecia 1 en kohms de la etapa de amplificación

//-------------------------------
//----------  I2C  --------------
//-------------------------------
//Dirección de los esclavos
#define ADDRES_SENS_PRESION_1     0x28

//-------------------------------
//----------- PWM ---------------
//-------------------------------


//-------------------------------
//----------  SPI  --------------
//-------------------------------
#define CS_TIVA_1           GPIO_PIN_3      //Puerto P pin 3 interrupción spi del maestro (chipselector)

//-------------------------------
//---------- UART ---------------
//-------------------------------




#endif /* TIVA_CONFIG_H_ */
