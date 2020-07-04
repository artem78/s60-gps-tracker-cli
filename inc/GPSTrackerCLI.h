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

#include <LoggingDefs.h>
#include "Logger.h"
#include "Positioning.h"
#include "TrackWriter.h"
#include "KeyCatcher.h"

// Constants

_LIT(KTimeFormat, "%H:%T:%S");
//_LIT(KTimeFormatWithMS, "%H:%T:%S.%C");

// Classes

class CGPSTrackerCLI: public CBase, public MPositionListener, public MKeyCatcherObserver
	{
private:
	RFs iFs;
	CConsoleBase* iConsole;
	CDynamicPositionRequestor* iPosRequestor;
	CGPXTrackWriter* iTrackWriter;
	CKeyCatcher* iKeyCatcher;
	TUint iTotalPointsCount;
	TReal iTotalDistance;
	TBool iIsAfterConnectionRestored;
	TInt iReturnCode; // Only used for passing code from Shutdown()
	
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
	CLogger* iLogger;
	void InitializeLoggingL();
#endif
	RFile iTrackFile;
	void InitializeTrackL();
	void ShowDataL();
	void /*Get*/ProgramDataDir(TDes &aDir);
	void Shutdown(TInt aReturnCode = KErrNone);
	
public:
	~CGPSTrackerCLI();
	static CGPSTrackerCLI* NewL(CConsoleBase* aConsole);
	static CGPSTrackerCLI* NewLC(CConsoleBase* aConsole);

	/**
	 * Runs tracker. This method returns only after program will be closed
	 * or any fatal error occurred. Note: active scheduler must be created
	 * before call this method.
	 * 
	 * @return Error code or KErrNone
	 */
	TInt Run();
	
	// Events
	void OnPositionUpdated();
	void OnPositionPartialUpdated();
	void OnPositionRestored();
	void OnPositionLost();
	void OnPositionError(TInt aErrCode);
	void OnKeyPressed(TKeyCode aKeyCode);
	void OnPauseTracking();
	void OnResumeTracking();
	};

//  Function Prototypes

GLDEF_C TInt E32Main();

#endif  // __GPSTRACKERCLI_H__

