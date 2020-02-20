/*
 ============================================================================
 Name		: Positioning.h
 Author	  : artem78
 Version	 : 1.0
 Copyright   : 
 Description : Receiving location info
 ============================================================================
 */


#ifndef POSITIONING_H
#define POSITIONING_H

#include <e32base.h>	// For CActive, link against: euser.lib
#include <lbs.h>
#include <e32math.h>
#include "Logger.h"


// Constants

const TInt KSecond = 1000000;
const TInt KDefaultPositionUpdateInterval = /* 5 * */ KSecond;
const TInt KDefaultPositionUpdateTimeOut = /*15 * KSecond*/ KDefaultPositionUpdateInterval * 5;


// Classes

class MPositionListener
	{
public:
	virtual void OnPositionUpdated() = 0;
	virtual void OnPositionPartialUpdated() = 0;
	virtual void OnPositionRestored() = 0;
	virtual void OnPositionLost() = 0;
	virtual void OnPositionError(TInt aErrCode) = 0;
	};


/*
 * Periodically get information about current location.
 */
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
	void Start();

protected:
	// C++ constructor
	CPositionRequestor(MPositionListener *aListener,
			TTimeIntervalMicroSeconds aUpdateInterval,
			TTimeIntervalMicroSeconds aUpdateTimeOut);

	// Second-phase constructor
	void ConstructL();

	// From CActive
	// Handle completion
	void RunL();
	
private:
	// How to cancel me
	void DoCancel();

	// Override to handle leaves from RunL(). Default implementation causes
	// the active scheduler to panic.
	//TInt RunError(TInt aError);

	// Custom properties and methods
public:
	inline TBool IsRunning() const
		{ return iState != EStopped; };
	
	inline TBool IsPositionRecieved() const
		{ return iState == EPositionRecieved; };
	
	inline const TPositionInfo* LastKnownPositionInfo() const
		{ return iLastPosInfo; };
	
	inline const TPositionInfo* PrevLastKnownPositionInfo() const
		{ return iPrevLastPosInfo; };
	
protected:
	RPositioner iPositioner;
	TPositionUpdateOptions iUpdateOptions;
	TPositionInfo* iLastPosInfo;
	TPositionInfo* iPrevLastPosInfo; // ToDo: Move from this class to GPSTrackerCLI
	
private:
	enum TPositionRequestorState
		{
		EStopped = 0,			// Positioning is disabled
		EPositionNotRecieved,	// Position is not yet recieved
		EPositionRecieved		// Position recieved
		};
	
	TPositionRequestorState iState; // State of the active object
	MPositionListener *iListener;
	RPositionServer iPosServer;
	
	void RequestPositionUpdate();
	void SetState(TPositionRequestorState aState);
	
	};


// Forward declaration
class CPointsCache;

/*
 * Automatically set position update interval depending on current speed
 * to preserve approximately equal distance between points 
 */
class CDynamicPositionRequestor : public CPositionRequestor
	{
	// Override parent class methods
public:
	~CDynamicPositionRequestor();
	
	static CDynamicPositionRequestor* NewL(MPositionListener *aListener);

	// Two-phased constructor.
	static CDynamicPositionRequestor* NewLC(MPositionListener *aListener);
	
	inline TTimeIntervalMicroSeconds UpdateInterval()
		{ return iUpdateOptions.UpdateInterval(); };
	
private:
	// C++ constructor
	CDynamicPositionRequestor(MPositionListener *aListener);
	
	// Second-phase constructor
	void ConstructL();
	
	// From CActive
	// Handle completion
	void RunL();

	// New methods and properties
private:	
	CPointsCache* iPointsCache;
	
	};

/*
 * Stores positions for the last period and return max speed
 */
class CPointsCache : public CBase
	{
public:
	CPointsCache(TTimeIntervalMicroSeconds aPeriod);
	~CPointsCache();
	
	void AddPoint(const TPosition &aPos);
	
	/* Get max speed from saved points.
	 * @return KErrNone if there`s no error and KErrGeneral
	 * when there are not enouth points to calculate speed (less then 2).
	 * @panic CPointsCache 1 If speed calculation has been failed.
	 */
	TInt GetMaxSpeed(TReal32 &aMaxSpeed);
	
private:
	enum TPanic
		{
		ESpeedCalculationFailed = 1,
		};
	
	TTimeIntervalMicroSeconds iPeriod;
	RArray<TPosition> iPoints; // ToDo: What about CCirBuf?
		// ToDo: Set granularity
	
	void Panic(TPanic aPanic);
	void ClearOldPoints();
	};


#endif // POSITIONING_H
