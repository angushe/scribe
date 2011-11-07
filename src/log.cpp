#include "log.h"
#include <pthread.h>

#define LOG_DEFAULT_CONFIG_FILE		"/usr/local/scribe/log_config.conf"
#define LOG_DEFAULT_FILE_PATH		"/tmp/log"
#define LOG_DEFAULT_FILE_BASENAME	"log"
#define LOG_DEFAULT_FILE_SUFFIX		""		// no suffix by default
#define LOG_DEFAULT_LOGLEVEL		0
#define LOG_DEFAULT_FLUSH_NUM		1

#define TEXT_LOG_DESTINATION		"log_desti"
#define TEXT_LOG_LEVEL				"log_level"
#define TEXT_LOG_FILE_PATH			"file_path"
#define TEXT_LOG_FILE_BASE_NAME		"file_base_name"
#define TEXT_LOG_FILE_SUFFIX		"file_suffix"
#define TEXT_LOG_FLUSH_NUM			"num_logs_to_flush"

#define LOG_TEST_TEXT				"This is a test message from Dionysus Xie in Shanghai.\n"


bool LOG_SYS_INIT(const string& config_file) {
	string f(config_file);

	if( f.empty() )
		f = LOG_DEFAULT_CONFIG_FILE;

	return LogSys::initialize( f );
}

void LOG_OUT(const string& log, const unsigned long level) {
	if (LogSys::getInstance() != NULL)
		LogSys::getInstance()->log(log, level);
}

void* thread_func(void *pLoops) {

	const unsigned long loops = (unsigned long)pLoops;

	LOG_TO_STDERR("thread begin, LOOPS = %lu", loops);

	for(unsigned long i = 0; i < loops; i++) {
		LOG_OUT(LOG_TEST_TEXT, ~0);	// this msg has the highest level
	}

	LOG_TO_STDERR("thread returned!");

	return NULL;
}

void LOG_SYS_TEST(const unsigned thread_num, const unsigned long logs_per_thread) {

	static pthread_t pThreads[10];

	const unsigned max_thread_num = sizeof(pThreads) / sizeof(pThreads[0]);
	unsigned _num = thread_num;

	if( _num > max_thread_num )
		_num = max_thread_num;

	const unsigned long total_num = logs_per_thread * _num;

	std::string text(LOG_TEST_TEXT);
	const unsigned long total_bytes = text.size() * total_num;

	LOG_TO_STDERR("[LOG TEST] test begin, thread number: %u, logs per thread: %lu", _num, logs_per_thread);

	struct timeval start, stop, diff;

	// get the starting time
	gettimeofday(&start, NULL);

	LOG_TO_STDERR("[LOG TEST] Start time, seconds: %ld, micro seconds: %ld", start.tv_sec, start.tv_usec);

	// create the threads and start them
	for(unsigned i = 0; i < _num; i++) {
	    pthread_create(&pThreads[i], NULL, thread_func, (void*)logs_per_thread);
	}

	// wait until all the threads return
	for(unsigned i = 0; i < _num; i++) {
		pthread_join(pThreads[i], NULL);
	}

	// get the ending time
	gettimeofday(&stop, NULL);

	// calculate the time used
	diff.tv_sec = stop.tv_sec - start.tv_sec;
	diff.tv_usec = stop.tv_usec - start.tv_usec;
	if(diff.tv_usec < 0) {
		diff.tv_sec--;
		diff.tv_usec += 1000000;
	}

	const double sec_used = diff.tv_sec + diff.tv_usec * 1.0 / 1000000;
	long logs_per_sec = total_num / sec_used;

	LOG_TO_STDERR("[LOG TEST] End   time, seconds: %ld, microseconds: %ld", stop.tv_sec,  stop.tv_usec);
	LOG_TO_STDERR("[LOG TEST] Used  time, seconds: %ld, microseconds: %ld", diff.tv_sec,  diff.tv_usec);
	LOG_TO_STDERR("[LOG TEST] Logs  per second: %ld", logs_per_sec);
	LOG_TO_STDERR("[LOG TEST] KB    per second: %ld", long(total_bytes / sec_used / 1024) );
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
				LOG_TO_STDERR("[LOG] Failed to creating the log system!");
				return false;
			}
		}
	}
	catch( std::exception& ex ) {
		LOG_TO_STDERR("[LOG] Exception: %s", ex.what());
		return false;
	}

	return true;
}

boost::shared_ptr<LogSys> LogSys::getInstance() {
	return s_pLogSys;
}

LogSys::LogSys(const string& config_file) throw (runtime_error) {

	LOG_TO_STDERR("[LOG] Opening file <%s> to get log config...", config_file.c_str());

	if( ! m_LogConfig.parseConfig(config_file) ) {
		runtime_error ex("[LOG] Failed to read the log config file!");
		throw ex;
	}

	unsigned long desti = (unsigned long)TO_STDERR;
	m_LogConfig.getUnsigned(TEXT_LOG_DESTINATION, desti);

	m_pLogger = Logger::createLoggerInterface( ENUM_LOG_TYPE(desti) );
	if( NULL == m_pLogger ) {
		runtime_error ex("[LOG] Failed to create the logger interface!");
		throw ex;
	}

	m_pLogger->config(m_LogConfig);

	if( ! m_pLogger->open() ) {
		runtime_error ex("[LOG] Failed to open log");
		throw ex;
	}

	LOG_TO_STDERR("[LOG] Log system initialized OK!");
}

LogSys::~LogSys() {
	if( m_pLogger )
		m_pLogger->close();
}

void LogSys::log(const string& msg, const unsigned long level) {
	if( m_pLogger )
		m_pLogger->log(msg, level);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// calss Logger
//

boost::shared_ptr<Logger> Logger::createLoggerInterface(const ENUM_LOG_TYPE type) throw (runtime_error) {

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
		runtime_error ex("[LOG] Wrong log type!");
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

Logger::Logger(const unsigned long level, const unsigned long flush_num):
	m_Level(level),
	m_MaxFlushNum(flush_num),
	m_NotFlushedNum(0) {
}

// destructor
Logger::~Logger() {
}

bool Logger::config(const LogConfig& conf) {
	conf.getUnsigned(TEXT_LOG_LEVEL, m_Level);
	conf.getUnsigned(TEXT_LOG_FLUSH_NUM, m_MaxFlushNum);

	return true;
}

bool Logger::log(const std::string& msg, const unsigned long level) {

	if (level >= this->m_Level)
		return logImpl(msg);
	else
		return false;
}

unsigned long Logger::getLevel() const {
    return m_Level;
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
		const unsigned long level,
		const unsigned long flush_num,
		const bool thread_safe):
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
				LOG_TO_STDERR("[LOG] Created log directory <%s>", m_FilePath.c_str());
			}
			else {
				LOG_TO_STDERR("[LOG] Failed to created log directory <%s>", m_FilePath.c_str());
				return false;
			}
		}
	}
	catch (const std::exception& e) {
		LOG_TO_STDERR("[LOG] Exception: %s", e.what());
		return false;
	}

	close();

	// open file for write in append mode
	ios_base::openmode mode = fstream::out | fstream::app;
	m_File.open( getFullFileName().c_str(), mode );

	if( !m_File.good() ) {
		LOG_TO_STDERR("[LOG] Failed to open log file <%s>", getFullFileName().c_str());
		return false;
	}
	else {
		LOG_TO_STDERR("[LOG] Opened log file <%s>", getFullFileName().c_str());
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
		LOG_TO_STDERR("[LOG] Exception: %s", ex.what());
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
		LOG_TO_STDERR("[LOG] Creating FileLogger failed! In RollingFileLogger::open()");
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
		LOG_TO_STDERR("[LOG] Exception: %s", ex.what());
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

