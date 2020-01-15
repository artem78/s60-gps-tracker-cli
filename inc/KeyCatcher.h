/*
 ============================================================================
 Name		: KeyCatcher.h
 Author	  : artem78
 Version	 : 1.0
 Copyright   : 
 Description : Receiving keys pressed in console.
 ============================================================================
 */

#ifndef KEYCATCHER_H
#define KEYCATCHER_H

#include <e32base.h>	// For CActive, link against: euser.lib
#include <e32cons.h>
#include <e32keys.h>


// Observer for recieving events from CKeyCatcher class.
class MKeyCatcherObserver
	{
public:
	virtual void OnKeyPressed(TKeyCode aKeyCode) = 0;
	};


// Makes notifications on key presses in console. Observer class must
// implement OnKeyPressed method to recieve code of pressed key.
class CKeyCatcher : public CActive
	{
public:
	// Cancel and destroy
	~CKeyCatcher();

	// Two-phased constructor.
	static CKeyCatcher* NewL(CConsoleBase* aConsole, MKeyCatcherObserver* aObserver);

	// Two-phased constructor.
	static CKeyCatcher* NewLC(CConsoleBase* aConsole, MKeyCatcherObserver* aObserver);

public:
	// New functions
	// Function for making the initial request
	void Start();

private:
	// C++ constructor
	CKeyCatcher(CConsoleBase* aConsole, MKeyCatcherObserver* aObserver);

	// Second-phase constructor
	void ConstructL();

private:
	// From CActive
	// Handle completion
	void RunL();

	// How to cancel me
	void DoCancel();

	// Custom properties and methods
private:
	CConsoleBase* iConsole;
	MKeyCatcherObserver* iObserver;

	};


#endif // KEYCATCHER_H
