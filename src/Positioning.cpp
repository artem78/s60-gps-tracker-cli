/*
 ============================================================================
 Name		: Positioning.cpp
 Author	  : artem78
 Version	 : 1.0
 Copyright   : 
 Description :
 ============================================================================
 */

#include "Positioning.h"
#include <lbssatellite.h>


// CPositionRequestor

//const TInt KGPSModuleID = 270526858;
const TInt KPositionMaxUpdateAge = 0; // Disable reuse positions


CPositionRequestor::CPositionRequestor(MPositionListener *aListener,
		TTimeIntervalMicroSeconds aUpdateInterval,
		TTimeIntervalMicroSeconds aUpdateTimeOut) :
	CActive(EPriorityStandard), // Standard priority
	iState(EStopped),
	iListener(aListener)
	{
	// Update position data every aUpdateInterval milliseconds
	iUpdateOptions.SetUpdateInterval(aUpdateInterval);
	
	// Cancel position request if position is not obtained
	// after aUpdateTimeOut milliseconds
	iUpdateOptions.SetUpdateTimeOut(aUpdateTimeOut);
	
	// Positions which have time stamp below KPositionMaxUpdateAge seconds
	// can be reused
	iUpdateOptions.SetMaxUpdateAge(TTimeIntervalMicroSeconds(KPositionMaxUpdateAge));
	
	// Send incomplete position data if more info is not available.
	// In this case data will be have valid only timestamp.
	iUpdateOptions.SetAcceptPartialUpdates(ETrue);
	}

CPositionRequestor* CPositionRequestor::NewLC(MPositionListener *aPositionListener,
		const TDesC &aRequestorName,
		TTimeIntervalMicroSeconds aUpdateInterval,
		TTimeIntervalMicroSeconds aUpdateTimeOut)
	{
	CPositionRequestor* self = new (ELeave) CPositionRequestor(aPositionListener,
			aUpdateInterval, aUpdateTimeOut);
	CleanupStack::PushL(self);
	self->ConstructL(aRequestorName);
	return self;
	}

CPositionRequestor* CPositionRequestor::NewL(MPositionListener *aPositionListener,
		const TDesC &aRequestorName,
		TTimeIntervalMicroSeconds aUpdateInterval,
		TTimeIntervalMicroSeconds aUpdateTimeOut)
	{
	CPositionRequestor* self = CPositionRequestor::NewLC(aPositionListener,
			aRequestorName, aUpdateInterval, aUpdateTimeOut);
	CleanupStack::Pop(); // self;
	return self;
	}

void CPositionRequestor::ConstructL(const TDesC &aRequestorName)
	{
	LOG(_L8("Position requestor created"));
	
	User::LeaveIfError(iPosServer.Connect());
	TPositionModuleId moduleId;
	User::LeaveIfError(iPosServer.GetDefaultModuleId(moduleId));
		// ToDo: Will be better to set GPS module instead default 
	
	TPositionModuleInfo moduleInfo;
	User::LeaveIfError(iPosServer.GetModuleInfoById(moduleId, moduleInfo));
	TUint32 moduleInfoFamily = moduleInfo.ClassesSupported(EPositionInfoFamily);
	
	// Create position info object with specified type depending
	// on the position module capabilities
	if (moduleInfoFamily & EPositionSatelliteInfoClass)
		{
		LOG(_L8("Sattelite info supported"));
		iLastPosInfo = new (ELeave) TPositionSatelliteInfo;
		iPrevLastPosInfo = new (ELeave) TPositionSatelliteInfo;
		}
	else if (moduleInfoFamily & EPositionCourseInfoClass)
		{
		LOG(_L8("Course info supported"));
		iLastPosInfo = new (ELeave) TPositionCourseInfo;
		iPrevLastPosInfo = new (ELeave) TPositionCourseInfo;
		}
	else if (moduleInfoFamily & EPositionInfoClass)
		{
		LOG(_L8("Position info supported"));
		iLastPosInfo = new (ELeave) TPositionInfo;
		iPrevLastPosInfo = new (ELeave) TPositionInfo;
		}
	else
		{
		LOG(_L8("Positioning module do not support any suitable class!"));
		User::Leave(KErrNotSupported);
		}
	
	
	// Preparing for start position recieving
	User::LeaveIfError(iPositioner.Open(iPosServer, moduleId));
	User::LeaveIfError(iPositioner.SetUpdateOptions(iUpdateOptions));
	User::LeaveIfError(
		iPositioner.SetRequestor(CRequestor::ERequestorService,
			CRequestor::EFormatApplication, aRequestorName)
	);
	
	CActiveScheduler::Add(this); // Add to scheduler
	}

CPositionRequestor::~CPositionRequestor()
	{
	Cancel(); // Cancel any request, if outstanding
	iPositioner.Close();
	iPosServer.Close();
	delete iPrevLastPosInfo;
	delete iLastPosInfo;
	LOG(_L8("Position requestor destroyed"));
	}

void CPositionRequestor::DoCancel()
	{
	LOG(_L8("Position requestor cancelled"));
	iPositioner.CancelRequest(EPositionerNotifyPositionUpdate);
	//iPosServer.CancelRequest(EPositionerNotifyPositionUpdate);
			// It seems that there is no need to call CancelRequest()
			// for position server (returns KErrNotSupported)
	
	SetState(EStopped);
	}

void CPositionRequestor::Start()
	{
	LOG(_L8("Position requestor started"));
	Cancel(); // Cancel any request, just to be sure
	RequestPositionUpdate();
	SetState(EPositionNotRecieved);
	}

void CPositionRequestor::RequestPositionUpdate()
	{
	iPositioner.NotifyPositionUpdate(*iLastPosInfo, iStatus);
	SetActive(); // Tell scheduler a request is active
	}

void CPositionRequestor::RunL()
	{
	switch (iStatus.Int())
		{
		// The fix is valid
		case KErrNone:
			{
			LOG(_L8("Position recieved"));

#ifdef __WINS__
			// On emulator time dilution of precision is NaN.
			// So set it to any plausible value.
			if (iLastPosInfo->PositionClassType() & EPositionSatelliteInfoClass)
				{
				TPositionSatelliteInfo* satteliteInfo = static_cast<TPositionSatelliteInfo*>(iLastPosInfo);
				if (Math::IsNaN(satteliteInfo->TimeDoP()))
					satteliteInfo->SetTimeDoP(1.5);
				}
#endif
			
			SetState(EPositionRecieved);
			iListener->OnPositionUpdated();
			RequestPositionUpdate();
			*iPrevLastPosInfo = *iLastPosInfo;
			break;
			}
			
		// The fix has only partially valid information.
		// It is guaranteed to only have a valid timestamp
		case KPositionPartialUpdate:
			{
			LOG(_L8("Partial position recieved"));
			SetState(EPositionNotRecieved);
			iListener->OnPositionPartialUpdated();
			RequestPositionUpdate();
			break;
			}
			
		// Position not recieved after specified time
		case KErrTimedOut:
			{
			LOG(_L8("Positioning request timed out"));
			SetState(EPositionNotRecieved);
			RequestPositionUpdate();
			break;
			}
			
		// Positioning has been stopped
		case KErrCancel:
			{
			LOG(_L8("Positioning request cancelled"));
			//setState(EStopped); // Not needed - State already has changed in DoCancel
			break;
			}
			
		// Any other errors
		default:
			{
			LOG(_L8("Positioning request failed, status: %d"), iStatus.Int());
			SetState(EStopped);
			iListener->OnPositionError(iStatus.Int());
			break;
			}
		}
	}

/*TInt CPositionRequestor::RunError(TInt aError)
	{
	//return KErrNone;
	return aError;
	}*/

void CPositionRequestor::SetState(TPositionRequestorState aState)
	{
	TInt oldState = iState;
	iState = aState;
	
	if (oldState != aState)
		{
		if (aState == EPositionRecieved)
			iListener->OnPositionRestored();
		else if (oldState == EPositionRecieved ||
				oldState == EStopped && aState == EPositionNotRecieved)
			iListener->OnPositionLost();
		}
	}


// CDynamicPositionRequestor

const TUint KMaxSpeedCalculationPeriod	= KSecond * 60;
const TReal KDistanceBetweenPoints		= 30.0;
const TUint KPositionMinUpdateInterval	= KSecond * 1;
const TUint KPositionMaxUpdateInterval	= KSecond * /*30*/ 10;

CDynamicPositionRequestor::CDynamicPositionRequestor(MPositionListener *aListener) :
	CPositionRequestor(aListener, KPositionMinUpdateInterval, KPositionMinUpdateInterval + KSecond)
	{
	// No implementation required
	}

CDynamicPositionRequestor::~CDynamicPositionRequestor()
	{
	delete iPointsCache;
	}

CDynamicPositionRequestor* CDynamicPositionRequestor::NewLC(MPositionListener *aPositionListener,
		const TDesC &aRequestorName)
	{
	CDynamicPositionRequestor* self = new (ELeave) CDynamicPositionRequestor(aPositionListener);
	CleanupStack::PushL(self);
	self->ConstructL(aRequestorName);
	return self;
	}

CDynamicPositionRequestor* CDynamicPositionRequestor::NewL(MPositionListener *aPositionListener,
		const TDesC &aRequestorName)
	{
	CDynamicPositionRequestor* self = CDynamicPositionRequestor::NewLC(aPositionListener,
			aRequestorName);
	CleanupStack::Pop(); // self;
	return self;
	}

void CDynamicPositionRequestor::ConstructL(const TDesC &aRequestorName)
	{
	iPointsCache = new (ELeave) CPointsCache(KMaxSpeedCalculationPeriod);
	
	CPositionRequestor::ConstructL(aRequestorName); // Run initialization of parent class
	}

void CDynamicPositionRequestor::RunL()
	{
	switch (iStatus.Int())
		{
		// The fix is valid
		case KErrNone:
			{
			TPosition pos;
			iLastPosInfo->GetPosition(pos);

			LOG(_L8("Current position: lat=%f lon=%f alt=%f"),
					pos.Latitude(), pos.Longitude(), pos.Altitude());
			
			iPointsCache->AddPoint(pos);
			
			TReal32 speed;
			TTimeIntervalMicroSeconds updateInterval;
			if (iPointsCache->GetMaxSpeed(speed) != KErrNone)
				{
				LOG(_L8("No max speed!"));
				updateInterval = KPositionMinUpdateInterval;
				}
			else
				{
				LOG(_L8("Max speed=%f m/s"), speed);
				TReal time = KDistanceBetweenPoints / speed;
				if (Math::IsFinite(time)) // i.e. speed > 0
					{
					TInt err = Math::Round(time, time, 0); // Round to seconds
								// to prevent too often positioner options updated
					if (err != KErrNone)
						LOG(_L8("Time Round error"));
					User::LeaveIfError(err);

					updateInterval = TTimeIntervalMicroSeconds(TInt64(time * KSecond));
					// Use range restrictions
					updateInterval = Min(
							Max(updateInterval, KPositionMinUpdateInterval),
							KPositionMaxUpdateInterval);
					}
				else
					{
					LOG(_L8("Error calculating position update time - set it to max value"));
					updateInterval = KPositionMaxUpdateInterval;
					}
				}
			
			if (updateInterval != iUpdateOptions.UpdateInterval())
				{
				iUpdateOptions.SetUpdateInterval(updateInterval);
				// Update timeout must not be less than update interval
				iUpdateOptions.SetUpdateTimeOut(updateInterval.Int64() + KSecond);
				iPositioner.SetUpdateOptions(iUpdateOptions); // Update positioner settings
				LOG(_L8("Update interval changed to %d s"), updateInterval.Int64() / KSecond);
				}
			
			break;
			}
		}
	
	CPositionRequestor::RunL(); // Run method from base class
	}


// CPointsCache

CPointsCache::CPointsCache(TTimeIntervalMicroSeconds aPeriod) :
	iPeriod(aPeriod)
	{
	// Not needed to initialize iPoints
	}

CPointsCache::~CPointsCache()
	{
	iPoints.Close();
	}

void CPointsCache::Panic(TPanic aPanic)
	{
	_LIT(KPanicCategory, "CPointsCache");
	User::Panic(KPanicCategory, aPanic);
	}

void CPointsCache::AddPoint(const TPosition &aPos)
	{
	ClearOldPoints();
	
	LOG(_L8("Point added to cache"));
	iPoints.Append(aPos);
	}
	
TInt CPointsCache::GetMaxSpeed(TReal32 &aMaxSpeed) 
	{
	// ToDo: Optimize
	
	ClearOldPoints();
	
	TInt count = iPoints.Count();
	
	if (count < 2)
		{
		LOG(_L8("Can`t calculate max speed - not enough points in cache (%d)!"), count);
		return KErrGeneral; // Can`t calculate speed
		}
	
	aMaxSpeed = 0;
	TReal32 speed;
	for (TInt i = 1; i < count; i++)
		{
		TInt r = iPoints[i].Speed(iPoints[i - 1], speed);
		__ASSERT_DEBUG(r == KErrNone, Panic(ESpeedCalculationFailed));
		aMaxSpeed = Max(speed, aMaxSpeed);
		}
	
	LOG(_L8("Max speed: %.1f m/s (total cached points: %d)"), aMaxSpeed, count);
	return KErrNone;
	}

void CPointsCache::ClearOldPoints()
	{
	// ToDo: Optimize
	
	TInt count = iPoints.Count();
	
	if (!count)
		return;
	
	TTime time;
	time.UniversalTime();
	time -= iPeriod;
	
#ifdef __WINS__
	TUint deletedCount = 0;
#endif
	for (TInt i = count - 1; i >= 0; i--) // Bypass from the oldest items at the end of array
		{
		if (iPoints[i].Time() < time)
			{
			iPoints.Remove(i);
#ifdef __WINS__
			deletedCount++;
#endif
			}
		}
	
#ifdef __WINS__
	if (deletedCount)
		LOG(_L8("Deleted %d outdated points"), deletedCount);
#endif
	}

