#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_LOGGER_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_LOGGER_H


#include <fstream>
#include <string>

namespace HlasmPlugin {
namespace HlasmLanguageServer {

#define LOG_ERROR(x)    HlasmPlugin::HlasmLanguageServer::Logger::getInstance().log(x)
#define LOG_WARNING(x)	HlasmPlugin::HlasmLanguageServer::Logger::getInstance().log(x)
#define LOG_INFO(x)     HlasmPlugin::HlasmLanguageServer::Logger::getInstance().log(x)

class Logger
{
public:

	static Logger & getInstance()
	{
		static Logger instance;
		return instance;
	}

	void log(const std::string & data);
	void log(const char * data);

protected:
	std::string getCurrentTime();
	
private:
	Logger();
	~Logger();
	std::ofstream m_file;
	
};

} //namespace HlasmLanguageServer
} //namespace HlasmPlugin
#endif 

