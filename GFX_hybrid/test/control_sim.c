
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

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
	srand(GetExecTimeMs());
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

	if(g_currExecTime % 1 == 0)
	{
		//uint32_t val = rand() % 70;
		uint32_t val = (uint32_t)( 25.0f * sin((double)g_currExecTime / 700.0) + 50.0 );
		
		bool ret = Event_Post(EVT_SYS_NEW_GRAPH_VALUE, val);
		while(!ret)
		{
			SysCtlDelay(MS_2_CLK(100));
		}
	}

	if(g_currExecTime % 1 == 0)
    {
        // 1. Define the period (how many ticks for one full wave cycle)
        // 4400 roughly matches the frequency of your previous sine wave
        const uint32_t PERIOD = 4400 / 2; 
        
        // 2. Define the Peak-to-Peak amplitude (75 max - 25 min = 50)
        const uint32_t AMPLITUDE = 50;
        
        // 3. Define the DC Offset (the minimum value the wave hits)
        const uint32_t OFFSET = 25;

        // Calculate the sawtooth using pure integer math.
        // We multiply first, then divide, to prevent integer truncation to 0.
        uint32_t val = (((g_currExecTime % PERIOD) * AMPLITUDE) / PERIOD) + OFFSET;
        
        bool ret = Event_Post(EVT_SYS_NEW_SAWTOOTH_VALUE, val);
        while(!ret)
        {
            SysCtlDelay(MS_2_CLK(100)); // Assuming MS_2_CLK is your macro for delay
        }
    }

}


