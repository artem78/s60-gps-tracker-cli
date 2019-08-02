/*
 ============================================================================
 Name		: PositionRequestor.h
 Author	  : artem78
 Version	 : 1.0
 Copyright   : 
 Description : CPositionRequestor declaration
 ============================================================================
 */

//TODO: Return satellites info 

#ifndef POSITIONREQUESTOR_H
#define POSITIONREQUESTOR_H

#include <e32base.h>	// For CActive, link against: euser.lib
//#include <e32std.h>		// For RTimer, link against: euser.lib
#include <lbs.h>
#include "PositionListener.h"
//#pragma comment(lib, "c:\\Symbian\\9.2\\S60_3rd_FP1\\Epoc32\\release\\winscw\\udeb\\lbs.lib") // TODO: Change path to local


const TInt KSecond = 1000000;
const TInt KDefaultPositionUpdateInterval = /* 5 * */ KSecond;
const TInt KDefaultPositionUpdateTimeOut = /*15 * KSecond*/ KDefaultPositionUpdateInterval * 5;


class CPositionRequestor : public CActive
	{
public:
	// Cancel and destroy
	~CPositionRequestor();

	// Two-phased constructor.
	static CPositionRequestor* NewL(MPositionListener *aListener,
			TTimeIntervalMicroSeconds aUpdateInterval = TTimeIntervalMicroSeconds(KDefaultPositionUpdateInterval),
			TTimeIntervalMicroSeconds aUpdateTimeOut = TTimeIntervalMicroSeconds(KDefaultPositionUpdateTimeOut));

	// Two-phased constructor.
	static CPositionRequestor* NewLC(MPositionListener *aListener,
			TTimeIntervalMicroSeconds aUpdateInterval = TTimeIntervalMicroSeconds(KDefaultPositionUpdateInterval),
			TTimeIntervalMicroSeconds aUpdateTimeOut = TTimeIntervalMicroSeconds(KDefaultPositionUpdateTimeOut));

public:
	// New functions
	// Function for making the initial request
	void StartL();

//private:
protected: // ToDo: Make all private protected?
	// C++ constructor
	CPositionRequestor(MPositionListener *aListener,
			TTimeIntervalMicroSeconds aUpdateInterval,
			TTimeIntervalMicroSeconds aUpdateTimeOut);
	
//private:

	// Second-phase constructor
	void ConstructL();

//private:
protected:
	// From CActive
	// Handle completion
	void RunL(); // ToDo: Is L really needed?
	
private:
	// How to cancel me
	void DoCancel();

	// Override to handle leaves from RunL(). Default implementation causes
	// the active scheduler to panic.
	//TInt RunError(TInt aError);

private:
	enum TPositionRequestorState
		{
		EStopped,				// Positioning is disabled
		EPositionNotRecieved, /*Pending*/	// Position is not yet recieved
		EPositionRecieved		// Position recieved
		};


public:
	inline TInt State() const;
	/*inline*/ TBool IsRunning() const; // ToDo: Why "Undefined symbol" error when inline?
	
private:
	TInt /*(TODO: TPositionRequestorState ?)*/ iState; // State of the active object
	
	MPositionListener *iListener;
	
	RPositionServer iPosServer;
protected:
	RPositioner iPositioner;
	
//protected:
	TPositionInfo iLastPosInfo;
	
private:
	
	void SetState(TInt aState);
	
protected:
	TPositionUpdateOptions iUpdateOptions;

	};


#endif // POSITIONREQUESTOR_H
