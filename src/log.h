#ifndef _LOG_H_
#define _LOG_H_

#include <string>
#include <time.h>
#include <iostream>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/interprocess_recursive_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include "log_config.h"


// log to the stand error
#define LOG_TO_STDERR(format_string,...)                             	\
{                                                                     	\
    time_t now;                                                         \
    char dbgtime[26] ;                                                  \
    time(&now);                                                         \
    ctime_r(&now, dbgtime);                                             \
    dbgtime[24] = '\0';                                                 \
    fprintf(stderr,"[%s] [LOG SYS] " #format_string " \n", dbgtime,##__VA_ARGS__); \
}

enum ENUM_LOG_TYPE {
	TO_STDERR = 0,
	TO_FILE,
	TO_ROLLING_FILE,
	TO_MAX,
};

enum ENUM_LOG_LEVEL {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_MAX   // DEBUG <= level < MAX !
};

bool LOG_SYS_INIT(const string& log_config_file);
void LOG_OUT(const string& log, ENUM_LOG_LEVEL level);

class Logger;
class RollingFileLogger;

class LogSys {
public:
	// static methods:
    static bool initialize(const string& config_file);
    static boost::shared_ptr<LogSys> getInstance();

    virtual ~LogSys();

    void log(const string& msg, ENUM_LOG_LEVEL level);
    void setLevel(ENUM_LOG_LEVEL level);

private:
    // static:
    static boost::shared_ptr<LogSys> s_pLogSys;

    // constructor
    LogSys(const string& config_file) throw (runtime_error);

    // non-static:
    LogConfig m_LogConfig;
    boost::shared_ptr<Logger> m_pLogger;
};


class Logger {
public:
	// static:
    static boost::shared_ptr<Logger> createLoggerInterface(ENUM_LOG_TYPE type) throw (runtime_error);

    // constructor
    Logger();
    Logger(ENUM_LOG_LEVEL level, unsigned long flush_num);

    virtual ~Logger();

    virtual bool config(const LogConfig& conf);
    virtual bool open() = 0;
    virtual bool close() = 0;

    bool log(const std::string& msg, ENUM_LOG_LEVEL level);
    ENUM_LOG_LEVEL getLevel() const;
    void setLevel(ENUM_LOG_LEVEL level);
    
protected:
    virtual bool logImpl(const std::string& msg) = 0;

    ENUM_LOG_LEVEL m_Level;
	unsigned long m_MaxFlushNum;
	unsigned long m_NotFlushedNum;
};


//
// class FileLogger
//
class FileLogger: public Logger {
public:

	friend class RollingFileLogger;

    FileLogger();
    FileLogger(const string& path,
    		const string& base_name,
    		const string& suffix,
    		ENUM_LOG_LEVEL level,
    		unsigned long flush_num,
    		bool thread_safe );

    virtual ~FileLogger();

    virtual bool config(const LogConfig& conf);
    virtual bool open();
    virtual bool close();

protected:
    virtual bool logImpl(const std::string& msg);
    std::string getFullFileName() const;

private:
    FileLogger(const FileLogger& rhs);
    const FileLogger& operator=(const FileLogger& rhs);
    bool writeLog(const std::string& msg);

    std::string m_FilePath;
    std::string m_FileBaseName;
    std::string m_FileSuffix;

    std::fstream m_File;

    const bool m_IsThreadSafe;
	boost::interprocess::interprocess_recursive_mutex m_Mutex;  	// the mutex
};


//
// class StdErrLogger
//
class StdErrLogger: public Logger {
public:
	StdErrLogger();
	virtual ~StdErrLogger();

	virtual bool config(const LogConfig& conf);
    virtual bool open();
    virtual bool close();

protected:
    virtual bool logImpl(const std::string& msg);

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

    virtual bool config(const LogConfig& conf);
    virtual bool open();
    virtual bool close();

protected:
    virtual bool logImpl(const std::string& msg);
    void rotateFile();

private:
    RollingFileLogger(const RollingFileLogger& rhs);
    const RollingFileLogger& operator=(const RollingFileLogger& rhs);

    void getCurrentDate(struct tm& date);
    string getFileNameByDate(const struct tm& date);

    std::string m_FilePath;
    std::string m_FileBaseName;
    std::string m_FileSuffix;

    boost::shared_ptr<FileLogger> m_pFileLogger;	// Rolling file logger uses a "file logger" to write log
	boost::interprocess::interprocess_recursive_mutex m_Mutex; 			// the mutex
	struct tm m_LastCreatedTime;
};

#endif /* _LOG_H_ */
