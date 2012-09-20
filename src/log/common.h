#ifndef COMMON_H_
#define COMMON_H_

#include <string>
#include <assert.h>
#include <iostream>
#include "allyes-log.h"


enum ENUM_LOG_TYPE {
    TO_STDERR = 0,
    TO_FILE,
    TO_ROLLING_FILE,
    TO_MAX,
};


// config iterms
#define TEXT_LOG_DESTINATION        "log_dest"
#define TEXT_LOG_LEVEL              "log_level"
#define TEXT_LOG_FILE_PATH          "file_path"
#define TEXT_LOG_FILE_BASE_NAME     "file_base_name"
#define TEXT_LOG_FILE_SUFFIX        "file_suffix"
#define TEXT_LOG_FLUSH_NUM          "num_logs_to_flush"


// default values
const   ENUM_LOG_TYPE   LOG_DEFAULT_LOG_DEST = TO_STDERR;
const   ENUM_LOG_LEVEL  LOG_DEFAULT_LOGLEVEL = LOG_LEVEL_INFO;
#define LOG_DEFAULT_FILE_PATH       "/tmp/log"
#define LOG_DEFAULT_FILE_BASENAME   "log"
#define LOG_DEFAULT_FILE_SUFFIX     ""      // no suffix by default
#define LOG_DEFAULT_FLUSH_NUM       (1)


// log to the stand error
#define LOG_TO_STDERR(format_string, ...)                               \
{                                                                       \
    time_t now;                                                         \
    char dbgtime[26] ;                                                  \
    time(&now);                                                         \
    ctime_r(&now, dbgtime);                                             \
    dbgtime[24] = '\0';                                                 \
                                                                        \
    std::string format("[%s] [LOG SYS] ");                              \
    format.append(format_string);                                       \
    fprintf(stderr, format.c_str(), dbgtime, ##__VA_ARGS__);            \
    std::cerr << std::endl;                                             \
}


#define Assert(expr, msg)\
{\
    if (!(expr)) {\
        LOG_TO_STDERR(msg);\
        assert(false);\
    }\
}

#endif
