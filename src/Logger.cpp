/*
 * Logger.cpp
 *
 *  Created on: 01.09.2019
 *      Author: user
 */

#include "Logger.h"
//#include <e32debug.h>// RDebug

_LIT8(KLineBreak, "\r\n");
_LIT8(KTab, "\t");


// Set initial values for static properties
TBool Logger::iIsConfigured = EFalse;
RFile Logger::iFile = RFile();


Logger::Logger()
	{
	// No implemenation needed
	}

void Logger::Configure(const RFile &aFile)
	{
	iFile = aFile;
	iIsConfigured = ETrue;
	}

void Logger::Write(const TDesC8 &aDes)
	{
	if (!iIsConfigured)
		return;
	
	//iLogFile.Write(aDes.)
	TBuf<20> timeBuff;
	_LIT/*8*/(KTimeFormat, "%H:%T:%S.%*C3");
	TTime time;
	time.HomeTime();
	time.FormatL(timeBuff, KTimeFormat);
	//buff.Append('\t');
	//buff.Append(aDes);
	
	//iFileText.Write(buff);
	
	TBuf8<20> timeBuff8;
	timeBuff8.Append(timeBuff);
	iFile.Write(timeBuff8);
			
	iFile.Write(_L8("\t"));
	iFile.Write(aDes);
	iFile.Write(KLineBreak);
	
	/////////
	/*TBuf8<256> buff8;
	buff8.Zero();
	buff8.Append(timeBuff8);
	buff8.Append(KTab);
	buff8.Append(aDes);
	buff8.Append(KLineBreak);
	TBuf16<256> buff;
	buff.Copy(buff8);
	RDebug::Print(buff);*/
	//////////
	}

void Logger::WriteFormat(/*const TDesC8 &aFmt*/ TRefByValue<const TDesC8> aFmt, ...)
	{
	// ToDo: Add format for date and time
	if (!iIsConfigured)
		return;

	VA_LIST list;
	VA_START(list, aFmt);
	
	TBuf8<256> buff;
	buff.Zero();
	buff.FormatList(aFmt, list);
	VA_END(list);
	
	Write(buff);
	
	}

//void Logger::WriteEmptyLine()
//	{
	//iFileText.Write(KNullDesC);
//	iFile.Write(KLineBreak);
	
	//////////
	/*TBuf16<10> buff;
	buff.Copy(KLineBreak);
	RDebug::Print(buff);*/
	///////////
//	}
