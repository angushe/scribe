/*
 * LogSys.h
 *
 *  Created on: Aug 15, 2012
 *      Author: xieliang
 */

#ifndef LOGSYS_H_
#define LOGSYS_H_

#include "log_config.h"
#include "Logger.h"


class LogSys {
public:
    // static methods:
    static LogSys& getInstance();

    virtual ~LogSys();

    bool initialize(const std::string& config_file);

    void log(const std::string& msg, ENUM_LOG_LEVEL level);

    void setLevel(ENUM_LOG_LEVEL level);

private:
    LogSys();

private:
    boost::shared_ptr<Logger> logger_;
};

#endif /* LOGSYS_H_ */
