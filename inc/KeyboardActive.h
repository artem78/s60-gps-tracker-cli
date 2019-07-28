/*
 ============================================================================
 Name		: KeyboardActive.h
 Author	  : artem78
 Version	 : 1.0
 Copyright   : 
 Description : CKeyboardActive declaration
 ============================================================================
 */

#ifndef KEYBOARDACTIVE_H
#define KEYBOARDACTIVE_H

#include <e32base.h>	// For CActive, link against: euser.lib
//#include <e32std.h>		// For RTimer, link against: euser.lib
#include <e32cons.h>
#include "KeyboardListener.h" 

class CKeyboardActive : public CActive
	{
public:
	// Cancel and destroy
	~CKeyboardActive();

	// Two-phased constructor.
	static CKeyboardActive* NewL(CConsoleBase* aConsole, MKeyboardListener* aListener);

	// Two-phased constructor.
	static CKeyboardActive* NewLC(CConsoleBase* aConsole, MKeyboardListener* aListener);

public:
	// New functions
	// Function for making the initial request
	void StartL();

private:
	// C++ constructor
	CKeyboardActive(CConsoleBase* aConsole, MKeyboardListener* aListener);

	// Second-phase constructor
	void ConstructL();

private:
	// From CActive
	// Handle completion
	void RunL(); // ToDo: Is L really needed?

	// How to cancel me
	void DoCancel();

	// Override to handle leaves from RunL(). Default implementation causes
	// the active scheduler to panic.
	//TInt RunError(TInt aError);

//private:
	/*enum TKeyboardActiveState
		{
		EUninitialized, // Uninitialized
		EInitialized, // Initalized
		EError
		// Error condition
		};*/

private:
	//TInt iState; // State of the active object
	//RTimer iTimer; // Provides async timing service
	
	CConsoleBase* iConsole;
	MKeyboardListener* iListener;

	};

#endif // KEYBOARDACTIVE_H
