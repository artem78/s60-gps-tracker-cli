/*
 * Logger.h
 *
 *  Created on: 01.09.2019
 *      Author: user
 */

#ifndef LOGGER_H_
#define LOGGER_H_

// INCLUDES
#include <e32std.h>
#include <e32base.h>
#include <f32file.h>


class Logger;

// ToDo: Add disabling logging when compile (in release build)
// ToDo: Write to log additinal info: __FILE__, __LINE__, module name, etc...
// ToDo: Add levels (debug, error, info, warning, etc...)

#define LOG_CONFIGURE(aFile) Logger::Configure(aFile)
//#define LOG_CONFIGURE(aFileName)
//#define LOG(aMsg, ...) Logger::WriteFormat(aMsg, ## __VA_ARGS__);
#define LOG(aMsg, anArgs...) Logger::WriteFormat(_L8(__PRETTY_FUNCTION__), aMsg, ##anArgs);
// https://ru.stackoverflow.com/questions/1026888/%d0%a1%d1%83%d1%89%d0%b5%d1%81%d1%82%d0%b2%d1%83%d0%b5%d1%82-%d0%bb%d0%b8-%d0%b2-c-%d0%bc%d0%b0%d0%ba%d1%80%d0%be%d1%81-%d1%81-%d0%b8%d0%bc%d0%b5%d0%bd%d0%b5%d0%bc-%d0%ba%d0%bb%d0%b0%d1%81%d1%81%d0%b0?noredirect=1#comment1747251_1026888

class Logger
// ToDo: Make class private (use macroses only)
	{
public:
	static void Configure(const RFile &aFile);
	//static void Configure(const TDesC aFileName);
	static void Write(const TDesC8 &aModule, const TDesC8 &aDes);
	static void WriteFormat(const TDesC8 &aModule, /*const TDesC8 &aFmt*/ TRefByValue<const TDesC8> aFmt, ...);
	//static void WriteEmptyLine();
	
private:
	Logger(); // Hide constructor for static class
	static TBool iIsConfigured;
	static RFile iFile;
	};


#endif /* LOGGER_H_ */
