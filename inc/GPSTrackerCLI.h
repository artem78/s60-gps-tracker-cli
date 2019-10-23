/*
 ============================================================================
 Name		: GPSTrackerCLI.h
 Author	  : artem78
 Copyright   : 
 Description : Exe header file
 ============================================================================
 */

#ifndef __GPSTRACKERCLI_H__
#define __GPSTRACKERCLI_H__

//  Include Files

#include <e32base.h>
#include <e32cons.h>

#include "PositionListener.h"
#include "KeyboardListener.h"
#include "PositionRequestor.h"
#include "TrackWriter.h"
#include "KeyboardActive.h"

// Constants

_LIT(KTimeFormat, "%H:%T:%S");
//_LIT(KTimeFormatWithMS, "%H:%T:%S.%C");

// Classes

class CGPSTrackerCLI: public CBase, public MPositionListener, public MKeyboardListener
	{
private:
	RFs iFs;
	CConsoleBase* iConsole;
	CDynamicPositionRequestor* iPosRequestor;
	CGPXTrackWriter* iTrackWriter;
	CKeyboardActive* iKeyboardActive;
	TUint iTotalPointsCount;
	TReal iTotalDistance;
	TReal32 iSpeed;
	TBool iIsAfterConnectionRestored;
	
	CGPSTrackerCLI(CConsoleBase* aConsole);
	void ConstructL();
	
	/**
	 * @return Symbian OS error code or KErrNone
	 */
	TInt MakeDir(const TDesC &aDir);
	
	/**
	 * @return Symbian OS error code or KErrNone
	 */
	TInt CurrentDateTime(TDes &aDes, const TDesC &aFormat);
	
#if LOGGING_ENABLED
	RFile iLogFile;
	void InitializeLoggingL();
#endif
	RFile iTrackFile;
	void InitializeTrackL();
	void ShowDataL();
	
public:
	~CGPSTrackerCLI();
	static CGPSTrackerCLI* NewL(CConsoleBase* aConsole);
	static CGPSTrackerCLI* NewLC(CConsoleBase* aConsole);
	
	void Run();
	
	// Events
	void OnPositionUpdatedL();
	void OnPositionPartialUpdated();
	void OnConnectedL();
	void OnDisconnectedL();
	void OnErrorL(TInt aErrCode);
	void OnKeyPressed(TKeyCode aKeyCode);
	void OnPauseTracking();
	void OnResumeTracking();
	};

//  Function Prototypes

GLDEF_C TInt E32Main();

#endif  // __GPSTRACKERCLI_H__

