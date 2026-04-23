#ifndef EVENT_ENGINE_H
#define EVENT_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	// Commands from GUI -> System
	EVT_CMD_CHANGE_THEME = 0,
	EVT_CMD_FULL_REPAINT,
	EVT_CMD_START_BOOT_SEQ,
	// Updates from System -> GUI
	EVT_SYS_COUNTER_CHANGED,
	EVT_SYS_COUNTER2_CHANGED,
	EVT_SYS_SLIDER_VALUE_CHANGED,
	EVT_SYS_CHANGE_CURRENT_FORM,
	EVT_SYS_NEXT_FORM,
	EVT_SYS_NEW_GRAPH_VALUE,
	EVT_SYS_PREV_FORM,
	EVT_SYS_NEW_SAWTOOTH_VALUE,
	EVT_SYS_BOOT_EEPROM_OK,
	EVT_SYS_BOOT_TOUCH_OK,
	EVT_SYS_BOOT_BATT_OK,
	EVT_SYS_BOOT_INST_OK,
	EVT_SYS_BOOT_TIMES_COUNT,
	EVT_SYS_BOOT_RTC_DATE,
	EVT_SYS_BOOT_RTC_TIME,
	EVT_SYS_BOOT_PROGRESS_VALUE_CHANGE,
	EVT_SYS_BOOT_FINISHED,
	EVT_SYS_SHOW_HOME_FORM,
	EVT_SYS_T1_VAL_CHANGED,
	EVT_SYS_T2_VAL_CHANGED,
	EVT_SYS_T3_VAL_CHANGED,
	EVT_SYS_VIN_VAL_CHANGED,
	EVT_SYS_VOUT_VAL_CHANGED,
	
	NUM_EVENTS,
} EventID_e;

typedef struct {
	EventID_e id;
	int32_t arg;
} SystemEvent_t;

typedef void (*EventHandler_fn)(uint32_t arg);

bool Event_Post(EventID_e id, int32_t arg);

bool Event_Receive(SystemEvent_t *pEvent);

bool Event_Subscribe(EventID_e id, EventHandler_fn handler);

bool Event_Dispatch(void);
bool Event_Init(void);

#endif	//EVENT_ENGINE_H