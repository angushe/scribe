/*
 * Logger.h
 *
 *  Created on: Aug 15, 2012
 *      Author: xieliang
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "allyes-log.h"
#include "log_config.h"
#include "common.h"


class Logger {
public:

    enum ENUM_LOGGER_STATUS {
        CREATED = 0,    // the logger is created with the default or specified configurations
        OPENED,         // the logger is opend and ready for logging
        CLOSED,         // the logger is closed
    };


    // static:
    static boost::shared_ptr<Logger> createLoggerInterface(ENUM_LOG_TYPE type) throw (std::runtime_error);

    virtual ~Logger();

    bool config(const LogConfig& conf);
    bool open();
    void close();
    bool log(const std::string& msg, ENUM_LOG_LEVEL level);
    void setLevel(ENUM_LOG_LEVEL new_level);

    ENUM_LOG_LEVEL getLevel() const;
    unsigned long getMaxFlushNum() const;

protected:
    // constructors
    Logger();
    Logger(ENUM_LOG_LEVEL level, unsigned long flush_num);

    virtual bool configImpl(const LogConfig& conf) = 0;
    virtual bool openImpl() = 0;
    virtual void closeImpl() = 0;
    virtual bool logImpl(const std::string& msg, ENUM_LOG_LEVEL level) = 0;
    virtual void setLevelImpl(ENUM_LOG_LEVEL new_level) {}
    virtual void flush() = 0;

private:
    void setDefaultConf();

private:
    ENUM_LOG_LEVEL level_;
    unsigned long max_flush_num_;
    unsigned long not_flushed_num_; // the num of logs not to be flushed

    ENUM_LOGGER_STATUS status_;
    boost::mutex mutex_;
};


//
// class FileLogger
//
class FileLogger: public Logger {
public:
    FileLogger();
    FileLogger(const std::string& path,
            const std::string& base_name,
            const std::string& suffix,
            ENUM_LOG_LEVEL level,
            unsigned long flush_num);

    virtual ~FileLogger();

protected:
    virtual bool configImpl(const LogConfig& conf);
    virtual bool openImpl();
    virtual void closeImpl();
    virtual bool logImpl(const std::string& msg, ENUM_LOG_LEVEL level);
    virtual void flush();

private:
    // disabled methods
    FileLogger(const FileLogger& rhs);
    const FileLogger& operator=(const FileLogger& rhs);

private:
    std::string getFullFileName() const;
    void setDefaultConf();

private:
    std::string file_path_;
    std::string file_base_name_;
    std::string file_suffix_;
    std::fstream file_;
};


//
// class StdErrLogger
//
class StdErrLogger: public Logger {
public:
    StdErrLogger();
    virtual ~StdErrLogger();

protected:
    virtual bool configImpl(const LogConfig& conf);
    virtual bool openImpl();
    virtual void closeImpl();
    virtual bool logImpl(const std::string& msg, ENUM_LOG_LEVEL level);
    virtual void flush();

private:
    StdErrLogger(const StdErrLogger& rhs);
    const StdErrLogger& operator=(const StdErrLogger& rhs);
};


//
// class RollingFileLogger
//
class RollingFileLogger : public Logger {
public:
    RollingFileLogger();
    virtual ~RollingFileLogger();

protected:
    virtual bool configImpl(const LogConfig& conf);
    virtual bool openImpl();
    virtual void closeImpl();
    virtual bool logImpl(const std::string& msg, ENUM_LOG_LEVEL level);
    virtual void setLevelImpl(ENUM_LOG_LEVEL new_level);
    virtual void flush();

private:
    // disabled methods
    RollingFileLogger(const RollingFileLogger& rhs);
    const RollingFileLogger& operator=(const RollingFileLogger& rhs);

private:
    void rotateFile(const struct tm& old_date, const struct tm& new_date);
    void getCurrentDate(struct tm& date);
    void setDefaultConf();
    std::string getFileNameByDate(const struct tm& date);

private:
    std::string file_path_;
    std::string file_base_name_;
    std::string file_suffix_;

    // Rolling file logger uses a "file logger" to write log
    boost::shared_ptr<Logger> file_logger_;

    struct tm last_created_time_;
};

#endif /* LOGGER_H_ */
