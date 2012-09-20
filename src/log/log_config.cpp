/*
 * log_config.cpp
 *
 *  Created on: 2011-10-25
 *      Author: xieliang
 */

#include <stdlib.h>
#include "log_config.h"
#include "common.h"


using namespace std;


LogConfig::LogConfig() {
}

LogConfig::~LogConfig() {
}

bool LogConfig::getInt(const string& intName, long int& _return) const {
	string str;
	if (getString(intName, str)) {
		_return = strtol(str.c_str(), NULL, 0);
		return true;
	}
	else {
		return false;
	}
}

bool LogConfig::getFloat(const std::string& floatName, float & _return) const {
	string str;
	if (getString(floatName, str)) {
		_return = strtof(str.c_str(), NULL);
		return true;
	}
	else {
		return false;
	}
}

bool LogConfig::getUnsigned(const string& intName,	unsigned long int& _return) const {
	string str;
	if (getString(intName, str)) {
		_return = strtoul(str.c_str(), NULL, 0);
		return true;
	}
	else {
		return false;
	}
}

bool LogConfig::getUnsignedLongLong(const string& llName, unsigned long long& _return) const {
	string str;
	if (getString(llName, str)) {
		_return = strtoull(str.c_str(), NULL, 10);
		return true;
	}
	else {
		return false;
	}
}

bool LogConfig::getString(const string& stringName, string& _return) const {
	map<std::string, std::string>::const_iterator iter = values_.find(
			stringName);

	if (iter != values_.end()) {
		_return = iter->second;
		return true;
	}

	return false;
}

// reads and parses the config data
bool LogConfig::parseConfig(const string& filename) {
	queue<string> config_strings;

	if ( !readConfFile(filename, config_strings) ) {
		LOG_TO_STDERR("Failed to open log config file: <%s>", filename.c_str());
		return false;
	}

	return parseStore(config_strings);
}

bool LogConfig::parseStore(queue<string>& raw_config) {

	string line;

	while (!raw_config.empty()) {
		line = raw_config.front();
		raw_config.pop();

		// remove leading and trailing whitespace
		line = trimString(line);

		// remove comment
		size_t comment = line.find_first_of('#');
		if (comment != string::npos) {
			line.erase(comment);
		}

		line = trimString(line);

		int length = line.size();
		if (length <= 0) {
			continue;
		}

		string::size_type eq = line.find('=');

		if (eq == string::npos) {
			LOG_TO_STDERR("Bad log config - line <%s> is missing an =", line.c_str());
			return false;
		}

		string arg = line.substr(0, eq);
		string val = line.substr(eq + 1, string::npos);

		// remove leading and trailing whitespace
		arg = trimString(arg);
		val = trimString(val);

		if (arg.empty() || val.empty()) {
			LOG_TO_STDERR("Bad log config - line <%s> is invalid!", line.c_str());
			return false;
		}

		if (values_.find(arg) != values_.end()) {
			LOG_TO_STDERR("Bad log config - duplicate key: %s", arg.c_str());
			return false;
		}

		values_[arg] = val;
	}

	return true;
}

// trims leading and trailing whitespace from a string
string LogConfig::trimString(const string& str) {
	string whitespace = " \t";
	size_t start = str.find_first_not_of(whitespace);
	size_t end = str.find_last_not_of(whitespace);

	if (start != string::npos) {
		return str.substr(start, end - start + 1);
	}
	else {
		return "";
	}
}

// reads every line from the file and pushes then onto _return
// returns false on error
bool LogConfig::readConfFile(const string& filename, queue<string>& _return) {
	string line;
	ifstream config_file;

	config_file.open(filename.c_str());

	if (!config_file.good()) {
		return false;
	}

	while (getline(config_file, line)) {
		_return.push(line);
	}

	config_file.close();
	return true;
}

