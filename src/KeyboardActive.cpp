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
	CActiveScheduler::Add(this); // Add to scheduler
	}

CKeyboardActive::~CKeyboardActive()
	{
	Cancel(); // Cancel any request, if outstanding
	}

void CKeyboardActive::DoCancel()
	{
	iConsole->ReadCancel();
	}

void CKeyboardActive::Start()
	{
	Cancel(); // Cancel any request, just to be sure
	iConsole->Read(iStatus);
	SetActive(); // Tell scheduler a request is active
	}

void CKeyboardActive::RunL()
	{
	iListener->OnKeyPressed(iConsole->KeyCode());
	
	iConsole->Read(iStatus);
	SetActive(); // Tell scheduler a request is active
	}
