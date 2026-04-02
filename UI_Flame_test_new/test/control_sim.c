
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "driverlib/sysctl.h"

#include "event_engine.h"
#include "helpers.h"
#include "control_sim.h"

// Timing parameters
static uint32_t g_lastExecTime = 0;
static uint32_t g_currExecTime = 0;

// Test parameters
uint32_t g_ui32SecCounter;

void controlSimulatorInit(void)
{
	// Initialize variables
	g_ui32SecCounter = 0;
	
}

void controlSimulatiorTask(void) {

    g_currExecTime = GetExecTimeMs();
	if(g_currExecTime - g_lastExecTime >= 1000 ) // A second has elapsed
	{
		g_ui32SecCounter++;
	
	    g_lastExecTime = g_currExecTime;
		bool ret = Event_Post(EVT_SYS_COUNTER_CHANGED, g_ui32SecCounter);
		while(!ret)
		{
			SysCtlDelay(MS_2_CLK(100));
		}
	}	

	
}


