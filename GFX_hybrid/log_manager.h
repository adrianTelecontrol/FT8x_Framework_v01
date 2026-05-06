#ifndef LOG_MANAGER_H_
#define LOG_MANAGER_H_

typedef enum {
	LM_STATE_IDLE = 0,
	LM_STATE_RUNNING,
	LM_STATE_ERROR
} LM_state_e;

// return the id of the log created, otherwise -1
int LM_startLog(const uint8_t drive, const char *name, char **header, uint8_t size);

bool LM_addLine2Log(const uint8_t logId, const char* line);

#endif // LOG_MANAGER_H_