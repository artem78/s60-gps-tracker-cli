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

// Constants

_LIT(KTimeFormat, "%H:%T:%S");
//_LIT(KTimeFormatWithMS, "%H:%T:%S.%C");

// Classes

class CListener: public CBase, public MPositionListener, public MKeyboardListener
	{
private:
	CConsoleBase* iConsole;
	TTime iDisconnectedTime;
	/*CPositionRequestor**/ CDynamicPositionRequestor* iPosRequestor;
	/*CTrackWriterBase**/ CGPXTrackWriter* iTrackWriter;
	TUint iTotalPointsCount;
	TReal iTotalDistance;
	TReal32 iSpeed;
	
	void ShowDataL();
	
public:
	CListener(CConsoleBase* aConsole, /*CTrackWriterBase**/ CGPXTrackWriter* aTrackWriter);
	void SetPositionRequestor(/*CPositionRequestor**/ CDynamicPositionRequestor* aPosRequestor);
		// Requestor currently used for start/stop tracking in keyboard control.
		// Setter used because there is cyclical dependency with requestor class.
	
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

