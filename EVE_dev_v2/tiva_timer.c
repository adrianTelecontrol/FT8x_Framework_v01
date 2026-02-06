/*
 * timer.c
 *
 *  Created on: 3 feb. 2021
 *      Author: zerch
 */

#include "tiva_timer.h"
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/timer.h"
#include <ti/sysbios/knl/Task.h>


//Genera un retardo de lo que esté configurado
// el timer A con la sentencia CFG_TIMERA_INT
// en el archivo h
void delayMs(uint32_t tDy)
{
    Task_sleep(tDy);
//    TimerEnable(TIMER0_BASE, TIMER_A);        //Habilita el timer
//    ticksDelay = 0;             //Pone en cero la variable de conteo
//    flagTimerDelay = 0;         //La bandera de tiempo se pone en cero
//    timeDelay = tDy;            //se iguala la variable hasta la cual se va a contar
//    while(!flagTimerDelay){}    //Si llega a la cuenta
//    TimerDisable(TIMER0_BASE, TIMER_A);       //Desactiva el timer y sale del retardo
}
//Para que funcione la sentencia anterior debe estar así la función del timerA
//funcion del timer A
//void timeFucA(void)
//{
//    ticksDelay++;
//    if(ticksDelay >= timeDelay)
//    {
//        flagTimerDelay = 1;
//        ticksDelay = 0;
//    }
//}

//delay que no es exacto
void delay4us(uint16_t tc)
{
    short ADS_esp1 = 0;
    short ADS_esp2 = 0;
    for(ADS_esp1 = 0; ADS_esp1 < tc; ADS_esp1++)
        for(ADS_esp2 = 0; ADS_esp2 < 13; ADS_esp2++);
}
