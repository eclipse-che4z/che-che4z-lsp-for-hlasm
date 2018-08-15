
#include <iostream>
#include <cstdlib>
#include <ctime>

#include "logger.h"

using namespace std;
using namespace HlasmPlugin::HlasmLanguageServer;

const string logFileName = "hlasmplugin.log";

Logger::Logger()
{
   m_file.open(logFileName.c_str(), ios::out);
   
}

Logger::~Logger()
{
   m_file.close();
}


void Logger::log(const std::string & data)
{
   m_file << getCurrentTime() << "  " << data << endl;
}

void Logger::log(const char * data)
{
	m_file << getCurrentTime() << "  " << data << endl;
}

string Logger::getCurrentTime()
{
   string currTime;
   //Current date/time based on current time
   time_t now = time(0); 
   // Convert current time to string

   char cstr[50];
   ctime_s(cstr, sizeof cstr, &now);
   currTime.assign(cstr);

   // Last charactor of currentTime is "\n", so remove it
   string currentTime = currTime.substr(0, currTime.size()-1);
   return currentTime;
}

