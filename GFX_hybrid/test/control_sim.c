
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "driverlib/sysctl.h"

#include "rtc_module.h"
#include "event_engine.h"
#include "helpers.h"
#include "control_sim.h"

// Timing parameters
static uint32_t g_currExecTime = 0;

// Test parameters
uint32_t g_ui32SecCounter;
uint32_t g_ui32Counter2;

bool g_bBootSequenceStart = false;
uint32_t g_ui32SeqStart = 0;
bool g_bBootFinished = false;
uint32_t g_ui32BootFinishedStart = 0;
bool g_bOnHomeForm = false;
uint32_t g_ui32HomeStarted = 0;

uint32_t ui32GraphCount = 0;

float val, t2val, t3val, vinVal, voutVal;

char statusStr[12] = "NOMINAL";
char dateStr[11] = "27/04/2026";
char timeStr[9] = "03:34:34";

static void onBootSequenceStart(uint32_t arg) {
	g_bBootSequenceStart = true;
	g_ui32SeqStart = GetExecTimeMs();
}

static void onBootFinished(uint32_t arg) {
	g_bBootFinished = true;
	g_bBootSequenceStart = false;
	g_ui32BootFinishedStart = GetExecTimeMs();
}

static void onHomeRendered(uint32_t arg) {
	g_ui32HomeStarted = GetExecTimeMs();
	g_bOnHomeForm = true;

	RTC_getFormattedDate(dateStr, sizeof(dateStr));
	Event_Post(EVT_SYS_DATE_CHANGED, (EventParam_t){.str = dateStr});
}

void controlSimulatorInit(void)
{
	// Initialize variables
	g_ui32SecCounter = 2;
	g_ui32Counter2 = 0;
	srand(GetExecTimeMs());
	Event_Subscribe(EVT_CMD_START_BOOT_SEQ, ( EventHandler_fn ) onBootSequenceStart);
	Event_Subscribe(EVT_SYS_BOOT_FINISHED, (EventHandler_fn) onBootFinished);
}

void controlSimulatiorTask(void) {

    g_currExecTime = GetExecTimeMs();

	// Update all clocks
	if(g_currExecTime % 1000 == 0) {
		RTC_getFormattedTime(timeStr, sizeof(timeStr));
		Event_Post(EVT_SYS_TIME_CHANGED, (EventParam_t){.str = timeStr});
	}

	if(g_currExecTime % 200 == 0)
	{
		ui32GraphCount++;
		//uint32_t val = rand() % 70;
		// float val =  25.0f * sin((double)g_currExecTime / (700.0)) + 50.0;
		float val =  25.0f * sin((double)ui32GraphCount / 100) + 50.0;
		
		Event_Post(EVT_SYS_NEW_GRAPH_VALUE, (EventParam_t){.f32 = val});
	}

	if(g_currExecTime % 200 == 0)
    {
        // 1. Define the period (how many ticks for one full wave cycle)
        // 4400 roughly matches the frequency of your previous sine wave
        // const uint32_t PERIOD = 4400.0f / 2.0f; 
        const uint32_t PERIOD = 200.0f; 
        
        // 2. Define the Peak-to-Peak amplitude (75 max - 25 min = 50)
        const float AMPLITUDE = 50.0f;
        
        // 3. Define the DC Offset (the minimum value the wave hits)
        const float OFFSET = 25.0f;

        // Calculate the sawtooth using pure integer math.
        // We multiply first, then divide, to prevent integer truncation to 0.
        // float val = (((g_currExecTime % PERIOD) * AMPLITUDE) / PERIOD) + OFFSET;
        float val = (((ui32GraphCount % PERIOD) * AMPLITUDE) / PERIOD) + OFFSET;
        
        bool ret = Event_Post(EVT_SYS_NEW_SAWTOOTH_VALUE, (EventParam_t){.f32 = val});
    }

	if(g_bBootSequenceStart) {
	//if(false) {
		uint32_t ui32TimeSinceBoot = g_currExecTime - g_ui32SeqStart;
		if(ui32TimeSinceBoot >= 1000 && ui32TimeSinceBoot <= 1200) {
			Event_Post(EVT_SYS_BOOT_EEPROM_OK, (EventParam_t){.ptr = NULL});
			Event_Post(EVT_SYS_BOOT_PROGRESS_VALUE_CHANGE, (EventParam_t){.ui32 = 22});
		}
		if(ui32TimeSinceBoot >= 2000 && ui32TimeSinceBoot <= 2200) {
			Event_Post(EVT_SYS_BOOT_TOUCH_OK, (EventParam_t){.ptr = NULL});
			Event_Post(EVT_SYS_BOOT_PROGRESS_VALUE_CHANGE, (EventParam_t){.ui32 = 34});
		} 
		if(ui32TimeSinceBoot >= 3000 && ui32TimeSinceBoot <= 3200) {
			Event_Post(EVT_SYS_BOOT_BATT_OK, (EventParam_t){.ptr = NULL});
			Event_Post(EVT_SYS_BOOT_PROGRESS_VALUE_CHANGE, (EventParam_t){.ui32 = 46});
		} 
		if(ui32TimeSinceBoot >= 4000 && ui32TimeSinceBoot <= 4200) {
			Event_Post(EVT_SYS_BOOT_INST_OK, (EventParam_t){.ptr = NULL});
			Event_Post(EVT_SYS_BOOT_PROGRESS_VALUE_CHANGE, (EventParam_t){.ui32 = 58});
		} 
		if(ui32TimeSinceBoot >= 5000 && ui32TimeSinceBoot <= 5200) {
			Event_Post(EVT_SYS_BOOT_TIMES_COUNT, (EventParam_t){.ptr = NULL});
			Event_Post(EVT_SYS_BOOT_PROGRESS_VALUE_CHANGE, (EventParam_t){.ui32 = 70});
		} 
		if(ui32TimeSinceBoot >= 6000 && ui32TimeSinceBoot <= 6200) {
			RTC_getFormattedDate(dateStr, sizeof(dateStr));
			Event_Post(EVT_SYS_BOOT_RTC_DATE, (EventParam_t){.str = dateStr});
			Event_Post(EVT_SYS_BOOT_PROGRESS_VALUE_CHANGE, (EventParam_t){.ui32 = 82});
		} 
		if(ui32TimeSinceBoot >= 7000 && ui32TimeSinceBoot <= 7200) {
			RTC_getFormattedTime(timeStr, sizeof(timeStr));
			Event_Post(EVT_SYS_BOOT_RTC_TIME, (EventParam_t){.str = timeStr});
			Event_Post(EVT_SYS_BOOT_PROGRESS_VALUE_CHANGE, (EventParam_t){.ui32 = 100});
		    onBootFinished(0);
		}

	}
	if(g_bBootFinished) {
		if((g_currExecTime - g_ui32BootFinishedStart) >= 3000) {
			g_bBootFinished = false;
			Event_Post(EVT_SYS_SHOW_HOME_FORM, (EventParam_t){.ptr = NULL});
			onHomeRendered(0);
		}
	}
	if(g_bOnHomeForm) {
	// if(false) {
		if(g_currExecTime % 500 == 0) {
			val = ((float)rand() / RAND_MAX) * 2.0 - 1.0;
			val = 25.0f + val;
			Event_Post(EVT_SYS_T1_VAL_CHANGED, (EventParam_t){.f32 = val});
		}
		if(g_currExecTime % 700 == 0) {
			t2val = ((float)rand() / RAND_MAX) * 4.0 - 2.0;
			t2val = 44.0f + t2val;
			Event_Post(EVT_SYS_T2_VAL_CHANGED, (EventParam_t){.f32 = t2val});
		}
		if(g_currExecTime % 1000 == 0) {
			t3val = ((float)rand() / RAND_MAX) * 3.0 - 1.0;
			t3val = 33.0f + t3val;
			Event_Post(EVT_SYS_T3_VAL_CHANGED, (EventParam_t){.f32 = t3val});
		}
		if(g_currExecTime % 900 == 0) {
			vinVal = ((float)rand() / RAND_MAX) * 10.0 - 5.0;
			vinVal = 12.0f + vinVal;
			Event_Post(EVT_SYS_VIN_VAL_CHANGED, (EventParam_t){.f32 = vinVal});
		}
		if(g_currExecTime % 300 == 0) {
			voutVal = ((float)rand() / RAND_MAX) * 2.0 - 1.0;
			voutVal = 5.0f + voutVal;
			Event_Post(EVT_SYS_VOUT_VAL_CHANGED, (EventParam_t){.f32 = voutVal});
		}
		if(g_currExecTime % 3000 == 0) {
			if(!strcmp(statusStr, "NOMINAL")) {
				strcpy(statusStr, "RUNNING");
			} else {
				strcpy(statusStr, "NOMINAL");
			}
			Event_Post(EVT_SYS_STATUS_CHANGED, (EventParam_t){.str = statusStr});
		}
	}

}


