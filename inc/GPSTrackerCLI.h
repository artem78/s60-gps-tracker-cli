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

_LIT(KTimeFormat, "%H:%T:%S.%C");

// Classes

class CListener: public CBase, public MPositionListener, public MKeyboardListener
	{
private:
	CConsoleBase* iConsole;
	TTime iDisconnectedTime;
	CPositionRequestor* iPosRequestor;
	CTrackWriterBase* iTrackWriter;
public:
	CListener(CConsoleBase* aConsole, CTrackWriterBase* aTrackWriter);
	void SetPositionRequestor(CPositionRequestor* aPosRequestor);
		// Requestor currently used for start/stop tracking in keyboard control.
		// Setter used because there is cyclical dependency with requestor class.
	
	// Events
	void OnPositionUpdatedL(const TPositionInfo &aPosInfo);
	void OnConnectedL();
	void OnDisconnectedL();
	void OnErrorL(TInt aErrCode);
	void OnKeyPressed(TKeyCode aKeyCode);
	void OnPauseTracking();
	};

//  Function Prototypes

GLDEF_C TInt E32Main();

#endif  // __GPSTRACKERCLI_H__

