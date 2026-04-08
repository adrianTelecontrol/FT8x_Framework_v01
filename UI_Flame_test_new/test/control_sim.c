
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "driverlib/sysctl.h"

#include "event_engine.h"
#include "helpers.h"
#include "control_sim.h"

// Timing parameters
static uint32_t g_currExecTime = 0;

// Test parameters
uint32_t g_ui32SecCounter;
uint32_t g_ui32Counter2;

void controlSimulatorInit(void)
{
	// Initialize variables
	g_ui32SecCounter = 2;
	g_ui32Counter2 = 0;
	
}

void controlSimulatiorTask(void) {

    g_currExecTime = GetExecTimeMs();
	if(g_currExecTime % 300 == 0 ) // A second has elapsed
	{
		g_ui32SecCounter++;
	
		bool ret = Event_Post(EVT_SYS_COUNTER_CHANGED, g_ui32SecCounter);
		while(!ret)
		{
			SysCtlDelay(MS_2_CLK(100));
		}
	}	

	if(g_currExecTime % 1000 == 0 ) // A second has elapsed
	{
		g_ui32Counter2++;
	
		bool ret = Event_Post(EVT_SYS_COUNTER2_CHANGED, g_ui32Counter2);
		while(!ret)
		{
			SysCtlDelay(MS_2_CLK(100));
		}
	}	

}


