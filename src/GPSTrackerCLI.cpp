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

#include "PositionListener.h"
#include "PositionRequestor.h"
#include "KeyboardActive.h"

//  Constants

_LIT(KTextConsoleTitle, "Console");
_LIT(KTextFailed, " failed, leave code = %d");
//_LIT(KTextPressAnyKey, " [press any key]\n");
_LIT(KTextPressAnyKeyToQuit, " [press any key to quit]\n");
//_LIT(KTextBye, "Bye!\n");
_LIT(KTextControl, "\n\nControl keys:\n   up arrow\t- pause/resume tracking\n   down arrow\t- exit\n");

//  Global Variables

LOCAL_D CConsoleBase* console; // write all messages to this
// TODO: Rename to gConsole

//  Classes

CListener::CListener(CConsoleBase* aConsole)
	:iConsole(aConsole)	{
	
	iDisconnectedTime.HomeTime();
}

void CListener::SetPositionRequestor(CPositionRequestor* aPosRequestor)
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
	}

void CListener::OnPositionUpdatedL(const TPositionInfo& aPosInfo)
	{
	//iConsole->Write(_L("Position recieved\n"));
	
	TPosition pos;
	aPosInfo.GetPosition(pos);
	TBuf<100> timeBuff;
	pos.Time().FormatL(timeBuff, KTimeFormat);
	
	iConsole->ClearScreen();
	iConsole->Printf(_L("Latitude:\t%f\nLongitude:\t%f\nALtitude:\t%f\n"
			"Horizontal accuracy:\t%fm\nVertical accuracy:\t%fm\n"
			"Time:\t%S UTC"),  // ToDo: Move to const
			pos.Latitude(), pos.Longitude(), pos.Altitude(),
			pos.HorizontalAccuracy(), pos.VerticalAccuracy(), &timeBuff);
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
	}

//  Local Functions

LOCAL_C void MainL()
	{
	CListener* listener = new (ELeave) CListener(console);
	CleanupStack::PushL(listener);
	
	CKeyboardActive* keyboardActive = CKeyboardActive::NewL(console, listener);
	CleanupStack::PushL(keyboardActive);
	keyboardActive->StartL();	
	
	CPositionRequestor* posRequestor = CPositionRequestor::NewL(listener);
	CleanupStack::PushL(posRequestor);
	listener->SetPositionRequestor(posRequestor); // Do it before AO start
	posRequestor->StartL();
	
	CActiveScheduler::Start();
	
	//console->Write(_L("Press any key to stop\n"));
	//console->Getch();
	
	CleanupStack::PopAndDestroy(3, listener);
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

