/*
 ============================================================================
 Name		: KeyboardActive.cpp
 Author	  : artem78
 Version	 : 1.0
 Copyright   : 
 Description : CKeyboardActive implementation
 ============================================================================
 */

#include "KeyboardActive.h"

CKeyboardActive::CKeyboardActive(CConsoleBase* aConsole, MKeyboardListener* aListener) :
	//CActive(EPriorityStandard), // Standard priority
	CActive(EPriorityUserInput),
	iConsole(aConsole),
	iListener(aListener)
	{
	}

CKeyboardActive* CKeyboardActive::NewLC(CConsoleBase* aConsole, MKeyboardListener* aListener)
	{
	CKeyboardActive* self = new (ELeave) CKeyboardActive(aConsole, aListener);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}

CKeyboardActive* CKeyboardActive::NewL(CConsoleBase* aConsole, MKeyboardListener* aListener)
	{
	CKeyboardActive* self = CKeyboardActive::NewLC(aConsole, aListener);
	CleanupStack::Pop(); // self;
	return self;
	}

void CKeyboardActive::ConstructL()
	{
	//User::LeaveIfError(iTimer.CreateLocal()); // Initialize timer
	CActiveScheduler::Add(this); // Add to scheduler
	}

CKeyboardActive::~CKeyboardActive()
	{
	Cancel(); // Cancel any request, if outstanding
	//iTimer.Close(); // Destroy the RTimer object
	// Delete instance variables if any
	}

void CKeyboardActive::DoCancel()
	{
	//iTimer.Cancel();
	iConsole->ReadCancel();
	}

void CKeyboardActive::StartL()
	{
	Cancel(); // Cancel any request, just to be sure
	//iState = EUninitialized;
	//iTimer.After(iStatus, aDelay); // Set for later
	iConsole->Read(iStatus);
	SetActive(); // Tell scheduler a request is active
	}

void CKeyboardActive::RunL()
	{
	/*if (iState == EUninitialized)
		{
		// Do something the first time RunL() is called
		iState = EInitialized;
		}
	else if (iState != EError)
		{
		// Do something
		}*/
	//iTimer.After(iStatus, 1000000); // Set for 1 sec later
	
	iListener->OnKeyPressed(iConsole->KeyCode());
	
	iConsole->Read(iStatus);
	SetActive(); // Tell scheduler a request is active
	}

/*TInt CKeyboardActive::RunError(TInt aError)
	{
	return aError;
	}
*/
