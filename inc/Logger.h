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
#define LOG(aMsg, anArgs...) Logger::WriteFormat(aMsg, ##anArgs);

class Logger
// ToDo: Make class private (use macroses only)
	{
public:
	static void Configure(const RFile &aFile);
	//static void Configure(const TDesC aFileName);
	static void Write(const TDesC8 &aDes);
	static void WriteFormat(/*const TDesC8 &aFmt*/ TRefByValue<const TDesC8> aFmt, ...);
	//static void WriteEmptyLine();
	
private:
	Logger(); // Hide constructor for static class
	static TBool iIsConfigured;
	static RFile iFile;
	};


#endif /* LOGGER_H_ */
