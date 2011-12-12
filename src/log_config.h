/*
 * log_config.h
 *
 *  Created on: 2011-10-25
 *      Author: xieliang
 */

#ifndef LOG_CONFIG_H_
#define LOG_CONFIG_H_

#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <queue>
#include <string>
#include <stdexcept>

using namespace std;


class LogConfig {
public:
	LogConfig();
	virtual ~LogConfig();

	bool parseConfig(const std::string& filename);

	bool getInt(const std::string& intName, long int& _return) const;
	bool getUnsigned(const std::string& intName, unsigned long int& _return) const;
	bool getUnsignedLongLong(const std::string& intName, unsigned long long& _return) const;
	bool getFloat(const std::string& floatName, float & _return) const;
	bool getString(const std::string& stringName, std::string& _return) const;

private:
	bool parseStore(std::queue<std::string>& raw_config);
	bool readConfFile(const std::string& filename, std::queue<std::string>& _return);

	static std::string trimString(const std::string& str);

	std::map<std::string, std::string> m_values;
};

#endif /* LOG_CONFIG_H_ */
