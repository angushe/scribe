/*
 * LogSys.cpp
 *
 *  Created on: Aug 15, 2012
 *      Author: xieliang
 */

#include "LogSys.h"
#include "allyes-log.h"
#include "common.h"


using namespace std;
using namespace boost;


LogSys& LogSys::getInstance() {
    static LogSys the_one;

    return the_one;
}

LogSys::LogSys() {
}

LogSys::~LogSys() {
    if(logger_) {
        logger_.reset();
    }
}

bool LogSys::initialize(const string& config_file) {
    LogConfig config;

    if(config_file.empty()) {
        LOG_TO_STDERR("No log config file specified. The default setting will be used!");
    }
    else {
        LOG_TO_STDERR("Opening file <%s> to get log config...", config_file.c_str());

        if(!config.parseConfig(config_file)) {
            LOG_TO_STDERR("Errors happened when read the log config file!");
            return false;
        }
    }

    unsigned long dest = static_cast<unsigned long>(LOG_DEFAULT_LOG_DEST);
    config.getUnsigned(TEXT_LOG_DESTINATION, dest);

    logger_ = Logger::createLoggerInterface(ENUM_LOG_TYPE(dest));
    if (NULL == logger_) {
        LOG_TO_STDERR("Failed to create the logger interface!");
        return false;
    }

    if (!logger_->config(config)) {
        LOG_TO_STDERR("Failed to config the log system");
        return false;
    }

    if (!logger_->open()) {
        LOG_TO_STDERR("Failed to open the log system");
        return false;
    }

    LOG_TO_STDERR("Log system initialized OK!");
    return true;
}

void LogSys::log(const string& msg, ENUM_LOG_LEVEL level) {
    if(logger_) {
        logger_->log(msg, level);
    }
}

void LogSys::setLevel(ENUM_LOG_LEVEL level) {
    if(logger_) {
        logger_->setLevel(level);
    }
}
