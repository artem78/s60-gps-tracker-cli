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

#include "PositionListener.h"
#include "PositionRequestor.h"
#include "KeyboardActive.h"
#include "TrackWriter.h"

//  Constants

_LIT(KTextConsoleTitle, "Console");
_LIT(KTextFailed, " failed, leave code = %d");
//_LIT(KTextPressAnyKey, " [press any key]\n");
_LIT(KTextPressAnyKeyToQuit, " [press any key to quit]\n");
//_LIT(KTextBye, "Bye!\n");
_LIT(KTextControl, "\n\nControl keys:\n   up arrow\t- pause/resume tracking\n   down arrow\t- exit\n");
//_LIT(KProgramDataDir, "e:\\documents\\GPS Tracker CLI\\");
_LIT(KProgramDataDir, "c:\\data\\GPSTracker\\"); // ToDo: Use drive wnere program is installed
_LIT(KTracksDir, "c:\\data\\GPSTracker\\tracks\\"); // TODO: Make this path relative
_LIT(KTimeFormatForFileName, "%F%Y%M%D_%H%T%S");

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
	//iConsole->Write(_L("Disconnected\n"));
	
	iConsole->ClearScreen();
	iConsole->Write(_L("[ No signal ]\n")); // ToDo: Move to const
	iConsole->Write(KTextControl);
	iDisconnectedTime.HomeTime();
	
	iTrackWriter->StartNewSegment();
	}

void CListener::OnPositionUpdatedL(const TPositionInfo& aPosInfo)
	{
	TPosition pos;
	aPosInfo.GetPosition(pos);
	
	// Increment counters and calculate speed
	iTotalPointsCount++;
	TReal32 distance = 0;
	TReal32 speed = 0;
	if (iTotalPointsCount > 1) // Skip first time when iLastKnownPosition is not set yet
		{
		pos.Distance(iLastKnownPosition, distance);
		iTotalDistance += distance;
		
		pos.Speed(iLastKnownPosition, speed);
		}
	
	// Save current position as last known
	iLastKnownPosition = pos;
	
	// Write position to file
	iTrackWriter->AddPoint(aPosInfo);
	
	// Write position to the screen
	//iConsole->Write(_L("Position recieved\n"));
	
	TBuf<100> timeBuff;
	pos.Time().FormatL(timeBuff, KTimeFormat);
	
	iConsole->ClearScreen();
	iConsole->Printf(_L("Latitude:\t%f\nLongitude:\t%f\nAltitude:\t%.1f m\n"
			"Horizontal accuracy:\t%.1f m\nVertical accuracy:\t%.1f m\n"
			"Time:\t%S UTC\nPoints: %d\nDistance: %.2f km\nUpdate interval: %.1f s\n"
			"Speed: %.1f k/h (%d m/s)"),  // ToDo: Move to const
			pos.Latitude(), pos.Longitude(), pos.Altitude(),
			pos.HorizontalAccuracy(), pos.VerticalAccuracy(), &timeBuff,
			iTotalPointsCount, iTotalDistance / 1000.0 /*m to km*/,
			TReal(iPosRequestor->UpdateInterval().Int64()) / KSecond,
			speed * 3.6 /* mps to kph */, TInt(speed));
	iConsole->Write(KTextControl);
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
				TRAP_IGNORE(
						iPosRequestor->StartL();
				);
			break;
			}
			
		default: // Just for hide compilation warnings
			break;
		}
	}

void CListener::OnPauseTracking()
	{
	iConsole->ClearScreen();
	iConsole->Printf(_L("[ Positioning paused ]")); // ToDo: Move to const
	iConsole->Write(KTextControl);
	
	//iTrackWriter->StartNewSegment();
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
	
	// Create gpx file for writing track
	TTime now;
	now.HomeTime();
	TBuf<20> timeBuff;
	now.FormatL(timeBuff, KTimeFormatForFileName);
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
	CGPXTrackWriter* trackWriter = CGPXTrackWriter::NewL(gpxFile);
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
	
	CleanupStack::PopAndDestroy(6, &fs);
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
	TRAPD(createError, console = Console::NewL(KTextConsoleTitle, TSize(
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

