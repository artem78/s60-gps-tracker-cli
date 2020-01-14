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
	void Start();

private:
	// C++ constructor
	CKeyboardActive(CConsoleBase* aConsole, MKeyboardListener* aListener);

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
	MKeyboardListener* iListener;

	};

#endif // KEYBOARDACTIVE_H
