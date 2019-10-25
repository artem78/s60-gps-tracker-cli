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
#include "TrackWriter.h"


#ifdef _DEBUG
#define LOGGING_ENABLED 1
#else
#define LOGGING_ENABLED 0
#endif


//  Constants

_LIT(KProgramName, "GPS Tracker CLI");
_LIT(KProgramVersion, "1.2.0");
_LIT(KProgramDataDir, "c:\\data\\GPSTracker\\"); // ToDo: Use drive wnere program is installed
_LIT(KTracksDir, "c:\\data\\GPSTracker\\tracks\\"); // TODO: Make this path relative
_LIT(KLogsDir, "c:\\data\\GPSTracker\\logs\\"); // TODO: Make this path relative
_LIT(KTimeFormatForFileName, "%F%Y%M%D_%H%T%S");
const TChar KSpace = TChar(0x20);


//  Global Variables

LOCAL_D CConsoleBase* gConsole; // write all messages to this

//  Classes

CGPSTrackerCLI::CGPSTrackerCLI(CConsoleBase* aConsole) :
	iConsole(aConsole)
	{
	}

CGPSTrackerCLI::~CGPSTrackerCLI()
	{
	delete iPosRequestor;
	delete iKeyboardActive;
	delete iTrackWriter;
	
	// Remove gpx file without points
	TFileName gpxFileName;
	TInt res = iTrackFile.FullName(gpxFileName);
	iTrackFile.Close();
	if (res == KErrNone)
		{
		if (iTotalPointsCount == 0)
			iFs.Delete(gpxFileName);
		}
	
#if LOGGING_ENABLED
	iLogFile.Close();
#endif
	iFs.Close();
	}

CGPSTrackerCLI* CGPSTrackerCLI::NewLC(CConsoleBase* aConsole)
	{
	CGPSTrackerCLI* self = new (ELeave) CGPSTrackerCLI(aConsole);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}

CGPSTrackerCLI* CGPSTrackerCLI::NewL(CConsoleBase* aConsole)
	{
	CGPSTrackerCLI* self = CGPSTrackerCLI::NewLC(aConsole);
	CleanupStack::Pop(); // self;
	return self;
	}

void CGPSTrackerCLI::ConstructL()
	{
	User::LeaveIfError(iFs.Connect());
	//User::LeaveIfError(iFs.SetSessionPath(KProgramDataDir));
#if LOGGING_ENABLED
	InitializeLoggingL();
#endif
	InitializeTrackL();
	
	iKeyboardActive = CKeyboardActive::NewL(iConsole, this);
	iKeyboardActive->StartL();
	
	iPosRequestor = CDynamicPositionRequestor::NewL(this);
	iPosRequestor->StartL();
	}

TInt CGPSTrackerCLI::MakeDir(const TDesC &aDir)
	{
	TInt res = iFs.MkDirAll(aDir);
	if (res == KErrNone || res == KErrAlreadyExists)
		return KErrNone;
	return res;
	}

TInt CGPSTrackerCLI::CurrentDateTime(TDes &aDes, const TDesC &aFormat)
	{
	TTime now;
	now.HomeTime();
	TRAPD(res, now.FormatL(aDes, aFormat));
	return res;
	}

#if LOGGING_ENABLED
void CGPSTrackerCLI::InitializeLoggingL()
	{
	// Create dir
	User::LeaveIfError(MakeDir(KLogsDir));
	
	// Configure logging file
	TFileName logFileName;
	logFileName.Append(KLogsDir);
#ifndef __WINS__
	TBuf<20> timeBuff;
	CurrentDateTime(timeBuff, KTimeFormatForFileName);
	
	_LIT(KLogFilePrefix, "log_");
	logFileName.Append(KLogFilePrefix);
	logFileName.Append(timeBuff);
#else
	_LIT(KLogFilename, "log");
	logFileName.Append(KLogFilename);
#endif
	_LIT(KLogFileExtension, ".txt");
	logFileName.Append(KLogFileExtension);
	User::LeaveIfError(iLogFile.Replace(iFs, logFileName, EFileWrite));
	LOG_CONFIGURE(iLogFile);
	LOG(_L8("Logging started"));
	}
#endif

void CGPSTrackerCLI::InitializeTrackL()
	{
	// Create dir
	User::LeaveIfError(MakeDir(KTracksDir));
	
	TBuf<20> timeBuff;
	CurrentDateTime(timeBuff, KTimeFormatForFileName);
	
	// Create gpx file for writing track
	TFileName gpxFileName;
	gpxFileName.Append(KTracksDir);
	_LIT(KTrackFilePrefix, "track_");
	gpxFileName.Append(KTrackFilePrefix);
	gpxFileName.Append(timeBuff);
	_LIT(KTrackFileExtension, ".gpx");
	gpxFileName.Append(KTrackFileExtension);
	User::LeaveIfError(iTrackFile.Create(iFs, gpxFileName, EFileWrite));
	
	TBuf<100> programFullName;
	programFullName.Append(KProgramName);
	programFullName.Append(KSpace);
	programFullName.Append(KProgramVersion);
	iTrackWriter = CGPXTrackWriter::NewL(iTrackFile, ETrue, programFullName);
	}

void CGPSTrackerCLI::Run()
	{
	CActiveScheduler::Start();
	}

void CGPSTrackerCLI::ShowDataL()
	{
	_LIT(KTextNoPosition, "[ Waiting for GPS signal... ]");
	_LIT(KTextTrackingPaused, "[ Track recording paused ]");
	_LIT(KTextControl1, "\n\nControl keys:\n   up arrow\t- ");
	_LIT(KTextControl2, " track recording\n   down arrow\t- exit\n");
	_LIT(KTextPause, "pause");
	_LIT(KTextResume, "resume");
	_LIT(KUTC, "UTC");

	_LIT(KTextNoValue,			"---");
	_LIT(KTextLat,				"Latitude: ");
	_LIT(KTextLon,				"Longitude: ");
	_LIT(KTextAlt,				"Altitude: ");
	_LIT(KTextHAcc,				"Horizontal accuracy: ");
	_LIT(KTextVAcc,				"Vertical accuracy: ");
	//_LIT(KTextTime,			"Time: ");
	_LIT(KTextPhoneTime,		"Phone time: ");
	_LIT(KTextSpeed,			"Speed: ");
	_LIT(KTextCourse,			"Course: ");
	_LIT(KTextHeading,			"Heading: ");
	_LIT(KTextSatellites,		"Satellites: ");
	_LIT(KTextSatelliteTime,	"GPS time: ");
	_LIT(KTextGDOP,				"GDOP: ");
	_LIT(KTextPointsCount,		"Saved points: ");
	_LIT(KTextTotalDistance,	"Odometer: ");
	_LIT(KTextPosUpdateInterval,"Position refresh rate: ");

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
	
	
	const TInt KLabelMaxWidth = 23;
	TRealFormat shortRealFmt = TRealFormat(10, 1);
	TRealFormat longRealFmt = TRealFormat(10, 5);
	
	
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
	
	// Course info
	if (posInfo->PositionClassType() & EPositionCourseInfoClass)
		{
		TPositionCourseInfo* courseInfo = static_cast<TPositionCourseInfo*>(posInfo);
		TCourse course;
		courseInfo->GetCourse(course);
		
		// Speed
		buff.AppendJustify(KTextSpeed, KLabelMaxWidth, ERight, KSpace);
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
		buff.Append(KLineBreak);
		
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
	if (iTotalDistance < 1000.0) // For <1km show distance in tens of meters
		{
		buff.AppendNum((TInt) /*(*/ iTotalDistance /*+ 5)*/ / 10 * 10);
		buff.Append(KSpace);
		buff.Append(KMetresUnit);
		}
	else // Show distance in kilometers
		{
		buff.AppendNum(iTotalDistance / 1000.0, shortRealFmt);
		buff.Append(KSpace);
		buff.Append(KKilometersUnit);
		}
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

void CGPSTrackerCLI::OnConnectedL()
	{
	LOG(_L8("Connected"));
	iIsAfterConnectionRestored = ETrue;
	}

void CGPSTrackerCLI::OnDisconnectedL()
	{
	LOG(_L8("Disconnected"));
	ShowDataL();
	
	iTrackWriter->StartNewSegment();
	}

void CGPSTrackerCLI::OnPositionUpdatedL()
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
	LOG(_L8("iIsAfterConnectionRestored=%d"), iIsAfterConnectionRestored);
	if (!iIsAfterConnectionRestored) // Skip distance increase when connection lost
		{
		pos.Distance(prevPos, distance);
		iTotalDistance += distance;
		}
	LOG(_L8("iTotalDistance=%f"), iTotalDistance);
	
	// Write position to file
	iTrackWriter->AddPoint(posInfo);
	
	// Write position to the screen	
	ShowDataL();
	
	iIsAfterConnectionRestored = EFalse;
	}

void CGPSTrackerCLI::OnPositionPartialUpdated()
	{
	TRAP_IGNORE(ShowDataL());
	}

void CGPSTrackerCLI::OnErrorL(TInt aErrCode)
	{
	_LIT(KTextError, "Error: %d");
	
	iConsole->ClearScreen();
	iConsole->Printf(KTextError, aErrCode);  // ToDo: Move to const
	}

void CGPSTrackerCLI::OnKeyPressed(TKeyCode aKeyCode)
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

void CGPSTrackerCLI::OnPauseTracking()
	{
	TRAP_IGNORE(ShowDataL());
	
	//iTrackWriter->StartNewSegment();
	}

void CGPSTrackerCLI::OnResumeTracking()
	{
	TRAP_IGNORE(ShowDataL());
	}

//  Local Functions

LOCAL_C void MainL()
	{
	CGPSTrackerCLI* tracker = CGPSTrackerCLI::NewL(gConsole);
	//CleanupStack::PushL(tracker);
	tracker->Run();
	//CleanupStack::PopAndDestroy(tracker);
	delete tracker;
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
	_LIT(KTextFailed, " failed, leave code = %d");
	_LIT(KTextPressAnyKeyToQuit, " [press any key to quit]\n");
	
	// Create cleanup stack
	__UHEAP_MARK;
	CTrapCleanup* cleanup = CTrapCleanup::New();

	// Create output console
	TRAPD(createError, gConsole = Console::NewL(KProgramName, TSize(
			KConsFullScreen, KConsFullScreen)));
	if (createError)
		{
		delete cleanup;
		return createError;
		}
	gConsole->SetCursorHeight(0); // Hide cursor
	
	// Run application code inside TRAP harness, wait keypress when terminated
	TRAPD(mainError, DoStartL());
	if (mainError)
		{
		gConsole->Printf(KTextFailed, mainError);
		gConsole->Printf(KTextPressAnyKeyToQuit);
		gConsole->Getch();
		}

	delete gConsole;
	delete cleanup;
	__UHEAP_MARKEND;
	return KErrNone;
	}

