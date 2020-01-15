/*
 ============================================================================
 Name		: KeyboardActive.cpp
 Author	  : artem78
 Version	 : 1.0
 Copyright   : 
 Description : CKeyCatcher implementation
 ============================================================================
 */

#include "KeyboardActive.h"

CKeyCatcher::CKeyCatcher(CConsoleBase* aConsole, MKeyCatcherObserver* aObserver) :
	CActive(EPriorityUserInput),
	iConsole(aConsole),
	iObserver(aObserver)
	{
	}

CKeyCatcher* CKeyCatcher::NewLC(CConsoleBase* aConsole, MKeyCatcherObserver* aObserver)
	{
	CKeyCatcher* self = new (ELeave) CKeyCatcher(aConsole, aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}

CKeyCatcher* CKeyCatcher::NewL(CConsoleBase* aConsole, MKeyCatcherObserver* aObserver)
	{
	CKeyCatcher* self = CKeyCatcher::NewLC(aConsole, aObserver);
	CleanupStack::Pop(); // self;
	return self;
	}

void CKeyCatcher::ConstructL()
	{
	CActiveScheduler::Add(this); // Add to scheduler
	}

CKeyCatcher::~CKeyCatcher()
	{
	Cancel(); // Cancel any request, if outstanding
	}

void CKeyCatcher::DoCancel()
	{
	iConsole->ReadCancel();
	}

void CKeyCatcher::Start()
	{
	Cancel(); // Cancel any request, just to be sure
	iConsole->Read(iStatus);
	SetActive(); // Tell scheduler a request is active
	}

void CKeyCatcher::RunL()
	{
	iObserver->OnKeyPressed(iConsole->KeyCode());
	
	iConsole->Read(iStatus);
	SetActive(); // Tell scheduler a request is active
	}
