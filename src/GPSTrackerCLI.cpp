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


#ifdef _DEBUG
#define LOGGING_ENABLED 1
#else
#define LOGGING_ENABLED 0
#endif


//  Constants

_LIT(KProgramName, "GPS Tracker CLI");
_LIT(KProgramVersion, "1.2.1");
_LIT(KTracksDir, "\\data\\GPSTracker\\tracks\\"); // TODO: Make this path relative
_LIT(KLogsDir, "\\data\\GPSTracker\\logs\\"); // TODO: Make this path relative
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
	delete iKeyCatcher;
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
	TFileName programDataDir;
	/*Get*/ProgramDataDir(programDataDir);
	User::LeaveIfError(MakeDir(programDataDir));
	User::LeaveIfError(iFs.SetSessionPath(programDataDir));
#if LOGGING_ENABLED
	InitializeLoggingL();
#endif
	InitializeTrackL();
	
	iKeyCatcher = CKeyCatcher::NewL(iConsole, this);
	iKeyCatcher->Start();
	
	iPosRequestor = CDynamicPositionRequestor::NewL(this, KProgramName);
	iPosRequestor->Start();
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

TInt CGPSTrackerCLI::Run()
	{
	CActiveScheduler::Start();
	return iReturnCode;
	}

void CGPSTrackerCLI::Shutdown(TInt aReturnCode)
	{
	LOG(_L8("Program exit with code %d"), aReturnCode);
	iReturnCode = aReturnCode;
	CActiveScheduler::Stop();
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
	//_LIT(KTextCourse,			"Course: ");
	_LIT(KTextHeading,			"Heading: ");
	_LIT(KTextSatellites,		"Satellites: ");
	_LIT(KTextSatelliteTime,	"GPS time: ");
	_LIT(KTextGDOP,				"GDOP: ");
	_LIT(KTextPointsCount,		"Saved points: ");
	_LIT(KTextTotalDistance,	"Odometer: ");
	_LIT(KTextPosUpdateInterval,"Position refresh rate: ");

	const TChar KLineBreak = TChar(0x0A);
	const TChar KForwardSlash = TChar(0x2F);

	_LIT(KMetersUnit, "m");
	_LIT(KKilometersUnit, "km");
	#ifdef _DEBUG
	_LIT(KMetersPerSecondsUnit, "m/s");
	#endif
	_LIT(KKilometersPerHourUnit, "km/h");
	_LIT(KSecondsUnit, "s");
	const TChar KDegree = TChar(0xB0);
	
	
	const TInt KLabelMaxWidth = 23;
	TRealFormat shortRealFmt = TRealFormat(10, 1);
	TRealFormat longRealFmt = TRealFormat(10, 5);
	
	const TReal KSpeedRatio  = 3.6; // m/s to km/h
	const TReal KMetersInKilometer = 1000.0;
	
	
	const TPositionInfo* posInfo = iPosRequestor->LastKnownPositionInfo(); 
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
	if (iPosRequestor->IsPositionRecieved())
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
		buff.Append(KMetersUnit);
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
		buff.Append(KMetersUnit);
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
		buff.Append(KMetersUnit);
		}
	else
		buff.Append(KTextNoValue);
	buff.Append(KLineBreak);
	
	// Course info
	if (posInfo->PositionClassType() & EPositionCourseInfoClass)
		{
		const TPositionCourseInfo* courseInfo = static_cast<const TPositionCourseInfo*>(posInfo);
		TCourse course;
		courseInfo->GetCourse(course);
		
		// Speed
		buff.AppendJustify(KTextSpeed, KLabelMaxWidth, ERight, KSpace);
		if (iPosRequestor->IsPositionRecieved() && !Math::IsNaN(course.Speed()))
			{
			buff.AppendNum(course.Speed() * KSpeedRatio, shortRealFmt);
			buff.Append(KSpace);
			buff.Append(KKilometersPerHourUnit);
#ifdef _DEBUG
			buff.Append(KSpace);
			buff.Append(TChar(0x28));
			buff.AppendNum(course.Speed(), longRealFmt);
			buff.Append(KSpace);
			buff.Append(KMetersPerSecondsUnit);
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
		const TPositionSatelliteInfoExtended* satelliteInfo = static_cast<const TPositionSatelliteInfoExtended*>(posInfo);
		
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
	if (iTotalDistance < KMetersInKilometer) // For <1km show distance in tens of meters
		{
		buff.AppendNum((TInt) /*(*/ iTotalDistance /*+ 5)*/ / 10 * 10);
		buff.Append(KSpace);
		buff.Append(KMetersUnit);
		}
	else // Show distance in kilometers
		{
		buff.AppendNum(iTotalDistance / KMetersInKilometer, shortRealFmt);
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

void CGPSTrackerCLI::/*Get*/ProgramDataDir(TDes &aDir)
	{
	_LIT(KProgramDataDirWithoutDrive, "\\data\\GPSTracker\\");
	
	aDir.Zero();
#ifdef __WINS__
	_LIT(KDriveC, "c:");
	aDir.Append(KDriveC);
#else
	// Get drive from current process (path to exe)
	RProcess proc;
	TFileName procPath = proc.FileName();
	TParse parser;
	parser.Set(procPath, NULL, NULL);
	aDir.Append(parser.Drive());
#endif
	aDir.Append(KProgramDataDirWithoutDrive);
	}

void CGPSTrackerCLI::OnPositionRestored()
	{
	LOG(_L8("Position recieved"));
	iIsAfterConnectionRestored = ETrue;
	}

void CGPSTrackerCLI::OnPositionLost()
	{
	LOG(_L8("Position lost"));
	TRAP_IGNORE(ShowDataL());
	
	TRAPD(r, iTrackWriter->StartNewSegmentL());
	if (r != KErrNone)
		{
		LOG(_L8("Error start new segment in gpx with code %d"), r);
		Shutdown(r);
		}
	}

void CGPSTrackerCLI::OnPositionUpdated()
	{
	const TPositionInfo* posInfo = iPosRequestor->LastKnownPositionInfo();
	TPosition pos;
	posInfo->GetPosition(pos);
	
	const TPositionInfo* prevPosInfo = iPosRequestor->PrevLastKnownPositionInfo();
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
	TRAPD(r, iTrackWriter->AddPointL(posInfo));
	if (r != KErrNone)
		{
		LOG(_L8("Error write position to the file with code %d"), r);
		Shutdown(r);
		}
	
	// Write position to the screen	
	TRAP_IGNORE(ShowDataL());
	
	iIsAfterConnectionRestored = EFalse;
	}

void CGPSTrackerCLI::OnPositionPartialUpdated()
	{
	TRAP_IGNORE(ShowDataL());
	}

void CGPSTrackerCLI::OnPositionError(TInt aErrCode)
	{
	Shutdown(aErrCode);
	}

void CGPSTrackerCLI::OnKeyPressed(TKeyCode aKeyCode)
	{
	switch (aKeyCode)
		{
		case EKeyDownArrow:
			{
			Shutdown();
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
				iPosRequestor->Start();
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
	
	//TRAP(r, iTrackWriter->StartNewSegmentL());
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
	TInt r = tracker->Run();
	//CleanupStack::PopAndDestroy(tracker);
	delete tracker;
	
	User::LeaveIfError(r);
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
	_LIT(KTextError, "Error: %d\n");
	_LIT(KTextPressAnyKeyToQuit, "\nPress any key to quit...\n");
	
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
		gConsole->ClearScreen();
		gConsole->Printf(KTextError, mainError);
		gConsole->Printf(KTextPressAnyKeyToQuit);
		gConsole->Getch();
		}

	delete gConsole;
	delete cleanup;
	__UHEAP_MARKEND;
	return /*KErrNone*/ mainError;  // ToDo: Find out how this value processed outside
									// by application framework?
	}

