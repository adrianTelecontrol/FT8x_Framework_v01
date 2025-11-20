/*
 * tiva_timer.h
 *
 *  Created on: 2 jun. 2020
 *      Author: Zerch
 */

#ifndef TIVA_TIMER_H_
#define TIVA_TIMER_H_

#include <stdint.h>
//#include "inc/hw_memmap.h"
//#include "driverlib/timer.h"

//-------------------------------
//---------- Timer 120[MHz]  -------------
//-------------------------------
//Configuración del Timer pues está configurado a 120[MHz]
#define t_5_us           600
#define t_10_us         1200
#define t_25_us         3000
#define t_50_us         6000
#define t_100_us       12000
#define t_500_us       60000    //Se eligió esta  configuración  Timer0
#define t_1_ms        120000
#define t_5_ms        600000
#define t_10_ms      1200000
#define t_20_ms      2400000
#define t_50_ms      6000000
#define t_100_ms    12000000
#define t_250_ms    30000000
#define t_500_ms    60000000
#define t_1_s      120000000


//-------------------------------------------------
//---- CONFIGURACIÓN ESPECÍFICA DEL PROYECTO ------
//-------------------------------------------------
//Timer0
#define CFG_TIMER0_INT  t_1_ms          //Tiempo para el timerA
#define MS_200          200
#define MS_250          250
#define UN_SEGUNDO      1000              //número de ticks para que de 1 segundo
#define MEDIO_SEGUNDO   UN_SEGUNDO/2
#define DOS_SEGUNDOS    2*UN_SEGUNDO
#define INV_T_MUESTREO  UN_SEGUNDO      //Inverso del Tiempo de muestreo del timer= 1/tm
#define T_SERIAL_DATOS  UN_SEGUNDO
#define INACTIVITY      (60*15)             //Segundos * minutos de inactividad
//Timer1
#define CFG_TIMER1_INT  t_1_ms          //Configuración del timerB PWM

//-------------------------------
//-------- VARIABLES  -----------
//-------------------------------
//variables para el delay
volatile uint8_t ticksDelay, timeDelay, flagTimerDelay;
uint16_t ticksA;
uint16_t ticksB;
uint8_t flagEnableInactivity;
uint32_t timeInativity;
uint8_t flagInativity;
uint32_t timerScreenSaver;


//Variables del timer B
uint16_t ticksB, ticksGraph, timeGraph, ticksDataHome;
uint8_t seconds, minutes, hour;
uint8_t flagShowGraph, flagShowDataHome;

void delayMs(uint32_t);
void delay4us(uint16_t);
#endif /* TIVA_TIMER_H_ */
