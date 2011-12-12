#include <pthread.h>
#include "log.h"

#define LOG_DEFAULT_FILE_PATH		"/tmp/log"
#define LOG_DEFAULT_FILE_BASENAME	"log"
#define LOG_DEFAULT_FILE_SUFFIX		""		// no suffix by default
const   ENUM_LOG_LEVEL LOG_DEFAULT_LOGLEVEL = LOG_LEVEL_INFO;
#define LOG_DEFAULT_FLUSH_NUM		1

#define TEXT_LOG_DESTINATION		"log_dest"
#define TEXT_LOG_LEVEL				"log_level"
#define TEXT_LOG_FILE_PATH			"file_path"
#define TEXT_LOG_FILE_BASE_NAME		"file_base_name"
#define TEXT_LOG_FILE_SUFFIX		"file_suffix"
#define TEXT_LOG_FLUSH_NUM			"num_logs_to_flush"

using namespace boost::interprocess;

static const char* s_LogLevelNames[LOG_LEVEL_MAX] = {
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
};

bool LOG_SYS_INIT(const string& config_file) {
	return LogSys::initialize( config_file );
}

void LOG_OUT(const string& log, ENUM_LOG_LEVEL level) {
	if (LogSys::getInstance() != NULL)
		LogSys::getInstance()->log(log, level);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// calss LogSys
//

boost::shared_ptr<LogSys> LogSys::s_pLogSys;

bool LogSys::initialize(const string& config_file) {

	try{
		if (NULL == s_pLogSys) {
			s_pLogSys = boost::shared_ptr<LogSys>(new LogSys(config_file));

			if (NULL == s_pLogSys) {
				LOG_TO_STDERR("Failed to creating the log system!");
				return false;
			}
		}
	}
	catch( std::exception& ex ) {
		LOG_TO_STDERR("Exception: %s", ex.what());
		return false;
	}

	return true;
}

boost::shared_ptr<LogSys> LogSys::getInstance() {
	return s_pLogSys;
}

LogSys::LogSys(const string& config_file) throw (runtime_error) {

    if( config_file.empty() ) {
        LOG_TO_STDERR("No log cofig file specified, log to stderr!");
    }
    else {
        LOG_TO_STDERR("Opening file <%s> to get log config...", config_file.c_str());

        if( ! m_LogConfig.parseConfig(config_file) ) {
            runtime_error ex("Errors happened when read the log config file!");
            throw ex;
        }
    }

	unsigned long desti = (unsigned long)TO_STDERR;
	m_LogConfig.getUnsigned(TEXT_LOG_DESTINATION, desti);

	m_pLogger = Logger::createLoggerInterface( ENUM_LOG_TYPE(desti) );
	if( NULL == m_pLogger ) {
		runtime_error ex("Failed to create the logger interface!");
		throw ex;
	}

	m_pLogger->config(m_LogConfig);

	if( ! m_pLogger->open() ) {
		runtime_error ex("Failed to open log");
		throw ex;
	}

	LOG_TO_STDERR("Log system initialized OK!");
}

LogSys::~LogSys() {
	if( m_pLogger )
		m_pLogger->close();
}

void LogSys::log(const string& msg, ENUM_LOG_LEVEL level) {
	if( m_pLogger )
		m_pLogger->log(msg, level);
}

void LogSys::setLevel(ENUM_LOG_LEVEL level) {
    if( m_pLogger )
        m_pLogger->setLevel(level);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// calss Logger
//

boost::shared_ptr<Logger> Logger::createLoggerInterface(ENUM_LOG_TYPE type) throw (runtime_error) {

	switch(type) {
	case TO_STDERR:
		return boost::shared_ptr<Logger>( new StdErrLogger() );
		break;

	case TO_FILE:
		return boost::shared_ptr<Logger>( new FileLogger() );
		break;

	case TO_ROLLING_FILE:
		return boost::shared_ptr<Logger>( new RollingFileLogger() );
		break;

	default:
		runtime_error ex("Wrong log type!");
		throw ex;
		break;
	}

	return boost::shared_ptr<Logger>();
}

// constructor
Logger::Logger():
	m_Level(LOG_DEFAULT_LOGLEVEL),
	m_MaxFlushNum(LOG_DEFAULT_FLUSH_NUM),
	m_NotFlushedNum(0) {
}

Logger::Logger(ENUM_LOG_LEVEL level, unsigned long flush_num):
	m_Level(level),
	m_MaxFlushNum(flush_num),
	m_NotFlushedNum(0) {
}

// destructor
Logger::~Logger() {
}

bool Logger::config(const LogConfig& conf) {
    // get log level
    unsigned long int num = 0;
	if(conf.getUnsigned(TEXT_LOG_LEVEL, num))
	{
	    if(num < static_cast<unsigned long int>(LOG_LEVEL_MAX))
	        m_Level = static_cast<ENUM_LOG_LEVEL>(num);
	}
	LOG_TO_STDERR("Log level: %s", s_LogLevelNames[m_Level]);

	conf.getUnsigned(TEXT_LOG_FLUSH_NUM, m_MaxFlushNum);

	return true;
}

bool Logger::log(const std::string& msg, ENUM_LOG_LEVEL level) {

	if (level >= this->m_Level)
		return logImpl(msg);
	else
		return false;
}

ENUM_LOG_LEVEL Logger::getLevel() const {
    return m_Level;
}

void Logger::setLevel(ENUM_LOG_LEVEL level) {
    if(level >= LOG_LEVEL_MAX) {
        LOG_TO_STDERR("Invalid log level!");
    }
    else if(m_Level != level) {
        m_Level = level;
        LOG_TO_STDERR("Log level has been reset to: %s", s_LogLevelNames[m_Level]);
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// calss FileLogger
//

FileLogger::FileLogger():	// set the default value
	Logger(),
	m_FilePath(LOG_DEFAULT_FILE_PATH),
	m_FileBaseName(LOG_DEFAULT_FILE_BASENAME),
	m_FileSuffix(LOG_DEFAULT_FILE_SUFFIX),
	m_IsThreadSafe(true) {
}

FileLogger::FileLogger(const string& path,
		const string& base_name,
		const string& suffix,
		ENUM_LOG_LEVEL level,
		unsigned long flush_num,
		bool thread_safe):
	Logger(level, flush_num),
	m_FilePath(path),
	m_FileBaseName(base_name),
	m_FileSuffix(suffix),
	m_IsThreadSafe(thread_safe) {
}

FileLogger::~FileLogger() {
	close();
}

bool FileLogger::config(const LogConfig& conf) {
	Logger::config(conf);

	conf.getString(TEXT_LOG_FILE_PATH, 		m_FilePath);
	conf.getString(TEXT_LOG_FILE_BASE_NAME, m_FileBaseName);
	conf.getString(TEXT_LOG_FILE_SUFFIX, 	m_FileSuffix);

	return true;
}

bool FileLogger::open() {

	// create the directory first
	try {

		if( !boost::filesystem::exists(m_FilePath) ) {
			if( boost::filesystem::create_directories(m_FilePath) ) {
				LOG_TO_STDERR("Created log directory <%s>", m_FilePath.c_str());
			}
			else {
				LOG_TO_STDERR("Failed to created log directory <%s>", m_FilePath.c_str());
				return false;
			}
		}
	}
	catch (const std::exception& e) {
		LOG_TO_STDERR("Exception: %s", e.what());
		return false;
	}

	close();

	// open file for write in append mode
	ios_base::openmode mode = fstream::out | fstream::app;
	m_File.open( getFullFileName().c_str(), mode );

	if( !m_File.good() ) {
		LOG_TO_STDERR("Failed to open log file <%s>", getFullFileName().c_str());
		return false;
	}
	else {
		LOG_TO_STDERR("Opened log file <%s>", getFullFileName().c_str());
		return true;
	}
}

bool FileLogger::close() {
	if( m_File.is_open() ) {
		m_File.flush();
		m_File.close();
	}

	return true;
}

bool FileLogger::logImpl(const std::string& msg) {
	try{

		if( m_IsThreadSafe ) {
			// lock first, it will unlock automaticlly when this function return
			scoped_lock<interprocess_recursive_mutex> lock(m_Mutex);

			return writeLog(msg);
		}
		else {
			return writeLog(msg);
		}
	}
	catch (std::exception& ex) {
		LOG_TO_STDERR("Exception: %s", ex.what());
		return false;
	}

	return true;
}

std::string FileLogger::getFullFileName() const {

	string full_name;

	if( !m_FilePath.empty() ) {
		full_name += m_FilePath;

		if( m_FilePath[ m_FilePath.size() - 1 ] != '/' )
			full_name += "/";
	}

	full_name += m_FileBaseName;

	if( !m_FileSuffix.empty() ) {
		if( m_FileSuffix[0] == '.' )
			full_name += m_FileSuffix;
		else
			full_name = full_name + "." + m_FileSuffix;
	}

	return full_name;
}

bool FileLogger::writeLog(const std::string& msg) {
	if (!m_File.is_open())
		return false;

	m_File << msg;

	Logger::m_NotFlushedNum++;
	if (Logger::m_NotFlushedNum >= Logger::m_MaxFlushNum) {
		m_File.flush();
		Logger::m_NotFlushedNum = 0;
	}

	return !m_File.bad();
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// calss StdErrLogger
//

StdErrLogger::StdErrLogger():
	Logger() {
}

StdErrLogger::~StdErrLogger() {
	close();
}

bool StdErrLogger::config(const LogConfig& conf) {
	return Logger::config(conf);
}

bool StdErrLogger::open() {
	return true;
}

bool StdErrLogger::close() {
	return true;
}

bool StdErrLogger::logImpl(const std::string& msg) {
	fprintf(stderr, "%s", msg.c_str());
	return true;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// calss RollingFileLogger
//

RollingFileLogger::RollingFileLogger():
	Logger(),
	m_FilePath(LOG_DEFAULT_FILE_PATH),
	m_FileBaseName(LOG_DEFAULT_FILE_BASENAME),
	m_FileSuffix(LOG_DEFAULT_FILE_SUFFIX) {
}

RollingFileLogger::~RollingFileLogger() {
	close();
}

bool RollingFileLogger::config(const LogConfig& conf) {
	Logger::config(conf);

	conf.getString(TEXT_LOG_FILE_PATH, 		m_FilePath);
	conf.getString(TEXT_LOG_FILE_BASE_NAME, m_FileBaseName);
	conf.getString(TEXT_LOG_FILE_SUFFIX, 	m_FileSuffix);

	return true;
}

bool RollingFileLogger::open() {

	getCurrentDate( m_LastCreatedTime );
	string file_name = getFileNameByDate(m_LastCreatedTime);

	m_pFileLogger = boost::shared_ptr<FileLogger>( new FileLogger(m_FilePath, file_name, m_FileSuffix, getLevel(), Logger::m_MaxFlushNum, false) );
	if( NULL == m_pFileLogger ) {
		LOG_TO_STDERR("Creating FileLogger failed! In RollingFileLogger::open()");
		return false;
	}

	return m_pFileLogger->open();
}

bool RollingFileLogger::close() {
	if( m_pFileLogger )
		m_pFileLogger->close();

	return true;
}

bool RollingFileLogger::logImpl(const std::string& msg) {

	try{
		// lock first, it will unlock automaticlly when this function return
		scoped_lock<interprocess_recursive_mutex> lock(m_Mutex);

		// create a new file when a day passed
		struct tm date_now;
		getCurrentDate(date_now);
		if( m_LastCreatedTime.tm_mday != date_now.tm_mday ) {
			rotateFile();
		}

		if( m_pFileLogger )
			return m_pFileLogger->logImpl(msg);
		else
			return false;
	}
	catch (std::exception& ex) {
		LOG_TO_STDERR("Exception: %s", ex.what());
		return false;
	}

	return true;
}

void RollingFileLogger::rotateFile() {
	close();
	open();
}

void RollingFileLogger::getCurrentDate(struct tm& date) {
	time_t raw_time = time(NULL);
	localtime_r(&raw_time, &date);
}

string RollingFileLogger::getFileNameByDate(const struct tm& date) {

	ostringstream filename;
	filename << m_FileBaseName << '-' << date.tm_year + 1900 << '-'
			<< setw(2) << setfill('0') << date.tm_mon + 1 << '-'
			<< setw(2) << setfill('0') << date.tm_mday;
	return filename.str();
}

