/*
 ============================================================================
 Name		: GPSTrackerCLI.cpp
 Author	  : artem78
 Copyright   : 
 Description : Exe source file
 ============================================================================
 */

//  Include Files  

#include "GPSTrackerCLI.h"
#include <e32base.h>
#include <e32std.h>
#include <e32cons.h>			// Console
#include <f32file.h>
#include "Logger.h"
#include <lbspositioninfo.h>
#include "LBSSatelliteExtended.h"

#include "PositionListener.h"
#include "PositionRequestor.h"
#include "KeyboardActive.h"
#include "TrackWriter.h"


#ifdef _DEBUG
#define LOGGING_ENABLED 1
#else
#define LOGGING_ENABLED 0
#endif


//  Constants

_LIT(KProgramName, "GPS Tracker CLI");
_LIT(KTextFailed, " failed, leave code = %d");
//_LIT(KTextPressAnyKey, " [press any key]\n");
_LIT(KTextPressAnyKeyToQuit, " [press any key to quit]\n");
//_LIT(KTextBye, "Bye!\n");
_LIT(KTextControl1, "\n\nControl keys:\n   up arrow\t- ");
_LIT(KTextControl2, " track recording\n   down arrow\t- exit\n");
_LIT(KTextPause, "pause");
_LIT(KTextResume, "resume");
//_LIT(KProgramDataDir, "e:\\documents\\GPS Tracker CLI\\");
_LIT(KProgramDataDir, "c:\\data\\GPSTracker\\"); // ToDo: Use drive wnere program is installed
_LIT(KTracksDir, "c:\\data\\GPSTracker\\tracks\\"); // TODO: Make this path relative
_LIT(KLogsDir, "c:\\data\\GPSTracker\\logs\\"); // TODO: Make this path relative
_LIT(KTimeFormatForFileName, "%F%Y%M%D_%H%T%S");

// For screen output
_LIT(KTextNoValue, "---");
_LIT(KTextLat, "Latitude: ");
_LIT(KTextLon, "Longitude: ");
_LIT(KTextAlt, "Altitude: ");
_LIT(KTextHAcc, "Horizontal accuracy: ");
_LIT(KTextVAcc, "Vertical accuracy: ");
//_LIT(KTextTime, "Time: ");
_LIT(KTextPhoneTime, "Phone time: ");
_LIT(KTextSpeed, "Speed: ");
_LIT(KTextCourse, "Course: ");
_LIT(KTextHeading, "Heading: ");
_LIT(KTextSatellites, "Satellites: ");
_LIT(KTextSatelliteTime, "GPS time: ");
_LIT(KTextGDOP, "GDOP: ");
_LIT(KTextPointsCount, "Saved points: ");
_LIT(KTextTotalDistance, "Odometer: ");
_LIT(KTextPosUpdateInterval, "Position refresh rate: ");

const TChar KSpace = TChar(0x20);
const TChar KLineBreak = TChar(0x0A);
const TChar KForwardSlash = TChar(0x2F);

_LIT(KMetresUnit, "m");
_LIT(KKilometersUnit, "km");
#ifdef _DEBUG
_LIT(KMetresPerSecondsUnit, "m/s");
#endif
_LIT(KKilometersPerHourUnit, "km/h");
_LIT(KSecondsUnit, "s");
const TChar KDegree = TChar(0xB0);

_LIT(KTextNoPosition, "[ Waiting for GPS signal... ]");
_LIT(KTextTrackingPaused, "[ Track recording paused ]");


//  Global Variables

LOCAL_D CConsoleBase* console; // write all messages to this
// TODO: Rename to gConsole

//  Classes

CListener::CListener(CConsoleBase* aConsole, /*CTrackWriterBase**/ CGPXTrackWriter* aTrackWriter) :
	iConsole(aConsole),
	iTrackWriter(aTrackWriter)
	{
		
	iDisconnectedTime.HomeTime();
	}

void CListener::SetPositionRequestor(/*CPositionRequestor**/ CDynamicPositionRequestor* aPosRequestor)
	{
	iPosRequestor = aPosRequestor;
	}

void CListener::ShowDataL()
	{	
	const TInt KLabelMaxWidth = 23;
	TRealFormat shortRealFmt = TRealFormat(10, 1);
	TRealFormat longRealFmt = TRealFormat(10, 5);
	_LIT(KUTC, "UTC");
	
	TPositionInfo* posInfo = iPosRequestor->LastKnownPositionInfo(); 
	TPosition pos;
	posInfo->GetPosition(pos);
	
	RBuf buff;
	buff.CreateL(1024);
	buff.CleanupClosePushL();
	
	// Messages
	//buff.Append(KLineBreak);
	TSize consoleSize = iConsole->ScreenSize();
	if (!iPosRequestor->IsRunning())
		buff.AppendJustify(KTextTrackingPaused, consoleSize.iWidth, ECenter, KSpace);
	else if (!iPosRequestor->IsPositionRecieved())
		buff.AppendJustify(KTextNoPosition, consoleSize.iWidth, ECenter, KSpace);
	else
		buff.Append(KLineBreak);
	buff.Append(KLineBreak);
	
	
	// Latitude
	buff.AppendJustify(KTextLat, KLabelMaxWidth, ERight, KSpace);
	if (iPosRequestor->State() == CPositionRequestor::EPositionRecieved)
		{
		buff.AppendNum(pos.Latitude(), longRealFmt);
		buff.Append(KDegree);
		}
	else
		buff.Append(KTextNoValue);
	buff.Append(KLineBreak);
	
	// Longitude
	buff.AppendJustify(KTextLon, KLabelMaxWidth, ERight, KSpace);
	if (iPosRequestor->IsPositionRecieved())
		{
		buff.AppendNum(pos.Longitude(), longRealFmt);
		buff.Append(KDegree);
		}
	else
		buff.Append(KTextNoValue);
	buff.Append(KLineBreak);
	
	// Altitude
	buff.AppendJustify(KTextAlt, KLabelMaxWidth, ERight, KSpace);
	if (iPosRequestor->IsPositionRecieved())
		{
		buff.AppendNum(pos.Altitude(), shortRealFmt);
		buff.Append(KSpace);
		buff.Append(KMetresUnit);
		}
	else
		buff.Append(KTextNoValue);
	buff.Append(KLineBreak);
	
	// Horizontal accuracy
	buff.AppendJustify(KTextHAcc, KLabelMaxWidth, ERight, KSpace);
	if (iPosRequestor->IsPositionRecieved())
		{
		buff.AppendNum(pos.HorizontalAccuracy(), shortRealFmt);
		buff.Append(KSpace);
		buff.Append(KMetresUnit);
		}
	else
		buff.Append(KTextNoValue);
	buff.Append(KLineBreak);
	
	// Vertical accuracy
	buff.AppendJustify(KTextVAcc, KLabelMaxWidth, ERight, KSpace);
	if (iPosRequestor->IsPositionRecieved())
		{
		buff.AppendNum(pos.VerticalAccuracy(), shortRealFmt);
		buff.Append(KSpace);
		buff.Append(KMetresUnit);
		}
	else
		buff.Append(KTextNoValue);
	buff.Append(KLineBreak);
	
	// Speed
	buff.AppendJustify(KTextSpeed, KLabelMaxWidth, ERight, KSpace);
	if (iPosRequestor->IsPositionRecieved())
		{
		buff.AppendNum(iSpeed * 3.6, shortRealFmt);
		buff.Append(KSpace);
		buff.Append(KKilometersPerHourUnit);
#ifdef _DEBUG
		buff.Append(KSpace);
		buff.Append(TChar(0x28));
		buff.AppendNum(iSpeed, /*shortRealFmt*/ longRealFmt);
		buff.Append(KSpace);
		buff.Append(KMetresPerSecondsUnit);
		buff.Append(TChar(0x29));
#endif
		}
	else
		buff.Append(KTextNoValue);
	buff.Append(KLineBreak);
	
	// Course info
	if (posInfo->PositionClassType() & EPositionCourseInfoClass)
		{
		TPositionCourseInfo* courseInfo = static_cast<TPositionCourseInfo*>(posInfo);
		TCourse course;
		courseInfo->GetCourse(course);
		
		// Speed
		/*buff.AppendJustify(KTextSpeed, KLabelMaxWidth, ERight, KSpace);
		if (iPosRequestor->IsPositionRecieved() && !Math::IsNaN(course.Speed()))
			{
			buff.AppendNum(course.Speed() * 3.6, shortRealFmt);
			buff.Append(KSpace);
			buff.Append(KKilometersPerHourUnit);
#ifdef _DEBUG
			buff.Append(KSpace);
			buff.Append(TChar(0x28));
			buff.AppendNum(course.Speed(), longRealFmt);
			buff.Append(KSpace);
			buff.Append(KMetresPerSecondsUnit);
			buff.Append(TChar(0x29));
#endif
			}
		else
			buff.Append(KTextNoValue);
		buff.Append(KLineBreak);*/
		
		// Course
		/*// ToDo: What is the difference between course and heading?
		buff.AppendJustify(KTextCourse, KLabelMaxWidth, ERight, KSpace);
		if (iPosRequestor->IsPositionRecieved() && !Math::IsNaN(course.Course()))
			{
			buff.AppendNum(course.Course(), shortRealFmt);
			buff.Append(KDegree);
			}
		else
			buff.Append(KTextNoValue);
		buff.Append(KLineBreak);*/
		
		// Heading
		buff.AppendJustify(KTextHeading, KLabelMaxWidth, ERight, KSpace);
		if (iPosRequestor->IsPositionRecieved() && !Math::IsNaN(course.Heading()))
			{
			buff.AppendNum(course.Heading(), shortRealFmt);
			buff.Append(KDegree);
			}
		else
			buff.Append(KTextNoValue);
		buff.Append(KLineBreak);
		
		}
	
	// Sattelite info
	if (posInfo->PositionClassType() & EPositionSatelliteInfoClass)
		{
		TPositionSatelliteInfoExtended* satelliteInfo = static_cast<TPositionSatelliteInfoExtended*>(posInfo);
		
		// Satellites count
		buff.AppendJustify(KTextSatellites, KLabelMaxWidth, ERight, KSpace);
		if (iPosRequestor->IsRunning())
			{
			buff.AppendNum(satelliteInfo->NumSatellitesUsed());
			buff.Append(KSpace);
			buff.Append(KForwardSlash);
			buff.Append(KSpace);
			buff.AppendNum(satelliteInfo->NumSatellitesInView());
			}
		else
			buff.Append(KTextNoValue);
		buff.Append(KLineBreak);
		
		// GDOP
		buff.AppendJustify(KTextGDOP, KLabelMaxWidth, ERight, KSpace);
		if (iPosRequestor->IsPositionRecieved())
			buff.AppendNum(satelliteInfo->GeometricDoP(), shortRealFmt);
		else
			buff.Append(KTextNoValue);
		buff.Append(KLineBreak);
		
		// Satellite time
		buff.AppendJustify(KTextSatelliteTime, KLabelMaxWidth, ERight, KSpace);
		//if (iPosRequestor->IsPositionRecieved())
		if (iPosRequestor->IsRunning() && satelliteInfo->SatelliteTime().Int64() > 0)
			{
			TBuf<20> timeBuff;
			satelliteInfo->SatelliteTime().FormatL(timeBuff, KTimeFormat);
			buff.Append(timeBuff);
			buff.Append(KSpace);
			buff.Append(KUTC);
			}
		else
			buff.Append(KTextNoValue);
		buff.Append(KLineBreak);
		}
	
	// Time
	buff.AppendJustify(KTextPhoneTime, KLabelMaxWidth, ERight, KSpace);
	if (iPosRequestor->IsPositionRecieved())
		{
		TBuf<20> timeBuff;
		pos.Time().FormatL(timeBuff, KTimeFormat);
		buff.Append(timeBuff);
		buff.Append(KSpace);
		buff.Append(KUTC);
		}
	else
		buff.Append(KTextNoValue);
	buff.Append(KLineBreak);
	
	// Total points count
	buff.AppendJustify(KTextPointsCount, KLabelMaxWidth, ERight, KSpace);
	buff.AppendNum(iTotalPointsCount);
	buff.Append(KLineBreak);
	
	// Total distance
	buff.AppendJustify(KTextTotalDistance, KLabelMaxWidth, ERight, KSpace);
	buff.AppendNum(iTotalDistance / 1000.0, shortRealFmt);
	buff.Append(KSpace);
	buff.Append(KKilometersUnit);
	buff.Append(KLineBreak);
	
	// Update interval
	buff.AppendJustify(KTextPosUpdateInterval, KLabelMaxWidth, ERight, KSpace);
	if (iPosRequestor->IsRunning())
		{
		buff.AppendNum(iPosRequestor->UpdateInterval().Int64() / KSecond);
		buff.Append(KSpace);
		buff.Append(KSecondsUnit);
		}
	else
		buff.Append(KTextNoValue);
	buff.Append(KLineBreak);

	
	// Controls information
	buff.Append(KTextControl1);
	if (iPosRequestor->IsRunning())
		buff.Append(KTextPause);
	else
		buff.Append(KTextResume);
	buff.Append(KTextControl2);
	
	
	iConsole->ClearScreen();
	iConsole->Write(buff);
	
	CleanupStack::PopAndDestroy(&buff);
	}

void CListener::OnConnectedL()
	{
	/*TTime now;
	now.HomeTime();
	
	TTimeIntervalSeconds interval;
	now.SecondsFrom(iDisconnectedTime, interval);
	
	iConsole->Printf(_L("Connected (after %d seconds)\n"), interval.Int());*/
	//iConsole->Write(_L("Connected\n"));
	}

void CListener::OnDisconnectedL()
	{
	ShowDataL();

	iDisconnectedTime.HomeTime();
	
	iTrackWriter->StartNewSegment();
	}

void CListener::OnPositionUpdatedL()
	{
	TPositionInfo* posInfo = iPosRequestor->LastKnownPositionInfo();
	TPosition pos;
	posInfo->GetPosition(pos);
	
	TPositionInfo* prevPosInfo = iPosRequestor->PrevLastKnownPositionInfo();
	TPosition prevPos;
	prevPosInfo->GetPosition(prevPos);
	
	// Increment counters and calculate speed
	iTotalPointsCount++;
	TReal32 distance = 0;
	iSpeed = 0;
	if (iTotalPointsCount > 1) // Skip first time when iLastKnownPositionInfo is not set yet
		{
		pos.Distance(prevPos, distance);
		iTotalDistance += distance;
		
		pos.Speed(prevPos, iSpeed);
		}
	LOG(_L8("Current speed %.1f m/s"), iSpeed);
	
	// Write position to file
	iTrackWriter->AddPoint(posInfo);
	
	// Write position to the screen	
	ShowDataL();
	}

void CListener::OnPositionPartialUpdated()
	{
	ShowDataL();
	}

void CListener::OnErrorL(TInt aErrCode)
	{
	iConsole->ClearScreen();
	iConsole->Printf(_L("Error: %d"), aErrCode);  // ToDo: Move to const
	}

void CListener::OnKeyPressed(TKeyCode aKeyCode)
	{
	switch (aKeyCode)
		{
		case EKeyDownArrow:
			{
			LOG(_L8("Program exit"));
			CActiveScheduler::Stop(); // Exit
			break;
			}
		case EKeyUpArrow:
			// Stop/resume tracking position
			{
			if (iPosRequestor->IsRunning())
				{
				iPosRequestor->Cancel();
				OnPauseTracking();
				}
			else
				{
				TRAP_IGNORE(
						iPosRequestor->StartL();
				);
				OnResumeTracking();
				}
			break;
			}
			
		default: // Just for hide compilation warnings
			break;
		}
	}

void CListener::OnPauseTracking()
	{
	ShowDataL();
	
	//iTrackWriter->StartNewSegment();
	}

void CListener::OnResumeTracking()
	{
	ShowDataL();
	}

//  Local Functions

LOCAL_C void MainL()
	{
	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);
	// Create data directory if not exist
	TInt res = fs.MkDirAll(KProgramDataDir);
	if (res != KErrNone && res != KErrAlreadyExists)
		User::Leave(res);
	User::LeaveIfError(fs.SetSessionPath(KProgramDataDir));
	// Create other directories
	res = fs.MkDir(KTracksDir);
	if (res != KErrNone && res != KErrAlreadyExists)
		User::Leave(res);
	res = fs.MkDir(KLogsDir);
	if (res != KErrNone && res != KErrAlreadyExists)
		User::Leave(res);
	
	TTime now;
	now.HomeTime();
	TBuf<20> timeBuff;
	now.FormatL(timeBuff, KTimeFormatForFileName);
	
	// Configure logging
#if LOGGING_ENABLED
	TFileName logFileName;
	logFileName.Append(KLogsDir);
#ifndef __WINS__
	logFileName.Append(_L("log_"));
	logFileName.Append(timeBuff);
#else
	logFileName.Append(_L("log"));
#endif
	logFileName.Append(_L(".txt"));
	RFile logFile;
	User::LeaveIfError(logFile./*Create*/Replace(fs, logFileName, EFileWrite));
	CleanupClosePushL(logFile);
	LOG_CONFIGURE(logFile);
	LOG(_L8("Log started"));
#endif
	
	// Create gpx file for writing track
	TFileName gpxFileName;
	gpxFileName.Append(KTracksDir);
	gpxFileName.Append(_L("track_"));
	gpxFileName.Append(timeBuff);
	gpxFileName.Append(_L(".gpx"));
	RFile gpxFile;
	User::LeaveIfError(gpxFile.Create(fs, gpxFileName, EFileWrite));
	//User::LeaveIfError(gpxFile.Create(fs, gpxFileName, EFileWrite | /*EFileShareReadersOnly*/ EFileShareReadersOrWriters));
	CleanupClosePushL(gpxFile);
	
	//CGPXTrackWriter* trackWriter = new (ELeave) CGPXTrackWriter(gpxFile);
	CGPXTrackWriter* trackWriter = CGPXTrackWriter::NewL(gpxFile, ETrue);
	CleanupStack::PushL(trackWriter);
	
	CListener* listener = new (ELeave) CListener(console, trackWriter);
	CleanupStack::PushL(listener);
	
	CKeyboardActive* keyboardActive = CKeyboardActive::NewL(console, listener);
	CleanupStack::PushL(keyboardActive);
	keyboardActive->StartL();
	
	/*CPositionRequestor**/ CDynamicPositionRequestor* posRequestor = /*CPositionRequestor*/ CDynamicPositionRequestor::NewL(listener);
	CleanupStack::PushL(posRequestor);
	listener->SetPositionRequestor(posRequestor); // Do it before AO start
	posRequestor->StartL();	

	CActiveScheduler::Start();
	
	//console->Write(_L("Press any key to stop\n"));
	//console->Getch();
	
#if LOGGING_ENABLED
	CleanupStack::PopAndDestroy(7, &fs); // logFile added
#else
	CleanupStack::PopAndDestroy(6, &fs);
#endif
	}

LOCAL_C void DoStartL()
	{
	// Create active scheduler (to run active objects)
	CActiveScheduler* scheduler = new (ELeave) CActiveScheduler();
	CleanupStack::PushL(scheduler);
	CActiveScheduler::Install(scheduler);

	MainL();
	
	// Delete active scheduler
	CleanupStack::PopAndDestroy(scheduler);
	}

//  Global Functions

GLDEF_C TInt E32Main()
	{
	// Create cleanup stack
	__UHEAP_MARK;
	CTrapCleanup* cleanup = CTrapCleanup::New();

	// Create output console
	TRAPD(createError, console = Console::NewL(KProgramName, TSize(
			KConsFullScreen, KConsFullScreen)));
	if (createError)
		{
		delete cleanup;
		return createError;
		}
	console->SetCursorHeight(0); // Hide cursor
	
	// Run application code inside TRAP harness, wait keypress when terminated
	TRAPD(mainError, DoStartL());
	if (mainError)
		{
		console->Printf(KTextFailed, mainError);
		console->Printf(KTextPressAnyKeyToQuit);
		console->Getch();
		}

	delete console;
	delete cleanup;
	__UHEAP_MARKEND;
	return KErrNone;
	}

