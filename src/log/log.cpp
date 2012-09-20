#include <string>
#include <iostream>
#include <map>
#include "allyes-log.h"
#include "LogSys.h"


using namespace std;


static const char* s_LogLevelNames[LOG_LEVEL_MAX] = {
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
};

const char* get_log_level_txt(ENUM_LOG_LEVEL level) {
    if (level < LOG_LEVEL_MAX) {
        return s_LogLevelNames[level];
    }
    else {
        return "UNKNOWN";
    }
}

bool LOG_SYS_INIT(const string& config_file) {
	return LogSys::getInstance().initialize(config_file);
}

void LOG_OUT(const string& log, ENUM_LOG_LEVEL level) {
    LogSys::getInstance().log(log, level);
}

void LOG_SET_LEVEL(ENUM_LOG_LEVEL level) {
    LogSys::getInstance().setLevel(level);
}

