//
// NOTE:
// 1. Author: Xieliang in Allyes
// 2. It is thread-safe.
//
// interfaces:
//
// #0
// bool LOG_SYS_INIT(const string& log_config_file);
//
// #1
// LOG_DEBUG(format_string,...)
//
// #2
// LOG_INFO(format_string,...)
//
// #3
// LOG_WARNING(format_string,...)
//
// #4
// LOG_ERROR(format_string,...)
//
// #5
// void LOG_SET_LEVEL(ENUM_LOG_LEVEL level);


#ifndef _LOG_H_
#define _LOG_H_

#include <time.h>
#include <stdio.h>
#include <string>


enum ENUM_LOG_LEVEL {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_MAX   // DEBUG <= level < MAX !
};

// used only inside this file !!!
#define LOG_IMPL(level, format_string, ...)									\
{																			\
    std::string format("%s");                                               \
    format.append(format_string);                                           \
                                                                            \
    const size_t max_log_text_len = 4096;                                   \
    char str_log[max_log_text_len + 1];			    					    \
                                                                            \
    const int n = snprintf(str_log, sizeof(str_log), format.c_str(), "", ##__VA_ARGS__);\
    																		\
	if(n >= 0) {															\
		if( (unsigned int)(n) < sizeof(str_log) ) {							\
			LOG_OUT(str_log, level);										\
		}																	\
		else {																\
			std::string msg(n+1, '\0'); 									\
			snprintf(&msg[0], n+1, format.c_str(), "", ##__VA_ARGS__);      \
			LOG_OUT(msg.c_str(), level);									\
		} 																	\
	}																		\
}

// interface #0, call this function before you use this LOG SYSTEM !!!
bool LOG_SYS_INIT(const std::string& log_config_file);

// interface #1
#define LOG_DEBUG(format_string,...)										\
{																		    \
	LOG_IMPL(LOG_LEVEL_DEBUG, format_string, ##__VA_ARGS__);				\
}

// interface #2
#define LOG_INFO(format_string,...)										    \
{																		    \
	LOG_IMPL(LOG_LEVEL_INFO, format_string, ##__VA_ARGS__);				    \
}

// interface #3
#define LOG_WARNING(format_string,...)										\
{																		    \
	LOG_IMPL(LOG_LEVEL_WARNING, format_string, ##__VA_ARGS__);				\
}

// interface #4
#define LOG_ERROR(format_string,...)										\
{																		    \
	LOG_IMPL(LOG_LEVEL_ERROR, format_string, ##__VA_ARGS__);				\
}

// interface #5
void LOG_SET_LEVEL(ENUM_LOG_LEVEL level);

void LOG_OUT(const std::string& log, ENUM_LOG_LEVEL level);
const char* get_log_level_txt(ENUM_LOG_LEVEL);

#endif /* _LOG_H_ */
