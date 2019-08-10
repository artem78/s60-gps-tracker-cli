/*
 ============================================================================
 Name		: PositionRequestor.cpp
 Author	  : artem78
 Version	 : 1.0
 Copyright   : 
 Description : CPositionRequestor implementation
 ============================================================================
 */

#include "PositionRequestor.h"


// CPositionRequestor

//const TInt KGPSModuleID = 270526858;
//const TInt KSecond = 1000000;
//const TInt KDefaultPositionUpdateInterval = /* 5 * */ KSecond;
//const TInt KDefaultPositionUpdateTimeOut = /*15 * KSecond*/ KPositionUpdateInterval * 5; // TODO: Set value in constructor
const TInt KPositionMaxUpdateAge = 0;

_LIT(KRequestorString, "MyRequestor"); // ToDo: Change

CPositionRequestor::CPositionRequestor(MPositionListener *aListener,
		TTimeIntervalMicroSeconds aUpdateInterval,
		TTimeIntervalMicroSeconds aUpdateTimeOut) :
	CActive(EPriorityStandard), // Standard priority
	iState(EStopped),
	iListener(aListener)//,
	//iUpdateInterval(aUpdateInterval),
	//iUpdateTimeOut(aUpdateTimeOut)
	{
		iUpdateOptions.SetUpdateInterval(aUpdateInterval);
		iUpdateOptions.SetUpdateTimeOut(aUpdateTimeOut);
		iUpdateOptions.SetMaxUpdateAge(TTimeIntervalMicroSeconds(KPositionMaxUpdateAge));
		iUpdateOptions.SetAcceptPartialUpdates(EFalse);
	}

CPositionRequestor* CPositionRequestor::NewLC(MPositionListener *aPositionListener,
		TTimeIntervalMicroSeconds aUpdateInterval,
		TTimeIntervalMicroSeconds aUpdateTimeOut)
	{
	CPositionRequestor* self = new (ELeave) CPositionRequestor(aPositionListener,
			aUpdateInterval, aUpdateTimeOut);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}

CPositionRequestor* CPositionRequestor::NewL(MPositionListener *aPositionListener,
		TTimeIntervalMicroSeconds aUpdateInterval,
		TTimeIntervalMicroSeconds aUpdateTimeOut)
	{
	CPositionRequestor* self = CPositionRequestor::NewLC(aPositionListener,
			aUpdateInterval, aUpdateTimeOut);
	CleanupStack::Pop(); // self;
	return self;
	}

void CPositionRequestor::ConstructL()
	{
	//User::LeaveIfError(iTimer.CreateLocal()); // Initialize timer
	
	// 1. Create a session with the location server
	User::LeaveIfError(iPosServer.Connect());
	//CleanupClosePushL(iPosServer);
	
	// 2. Create a subsession using the default positioning module
	//TPositionModuleId moduleId = TPositionModuleId();
	//moduleId.Uid(KGPSModuleID);
	User::LeaveIfError(iPositioner.Open(iPosServer/*, moduleId*/));
	//CleanupClosePushL(iPositioner);
	
	// 3. Set update options
	// Set the options
	User::LeaveIfError(iPositioner.SetUpdateOptions(iUpdateOptions));
	
	User::LeaveIfError(
		iPositioner.SetRequestor(CRequestor::ERequestorService,
			CRequestor::EFormatApplication, KRequestorString) // Todo: Why this needed?
	);
	
	CActiveScheduler::Add(this); // Add to scheduler
	}

CPositionRequestor::~CPositionRequestor()
	{
	Cancel(); // Cancel any request, if outstanding
	//iTimer.Close(); // Destroy the RTimer object
	// Delete instance variables if any
	iPositioner.Close();
	iPosServer.Close();
	}

void CPositionRequestor::DoCancel()
	{
	//iTimer.Cancel();
	//iPositioner.
	iPositioner.CancelRequest(/*EPositionerGetLastKnownPosition*/ EPositionerNotifyPositionUpdate);
	//iPosServer.CancelRequest(EPositionerGetLastKnownPosition);
	
	SetState(EStopped);
	}

void CPositionRequestor::StartL()
	{
	Cancel(); // Cancel any request, just to be sure
	//iState = EPositionNotRecieved;
	//iTimer.After(iStatus, aDelay); // Set for later
	iPositioner.NotifyPositionUpdate(iLastPosInfo, iStatus);
	SetActive(); // Tell scheduler a request is active
	SetState(EPositionNotRecieved);
	}

void CPositionRequestor::RunL()
	{
	/*if (iState == EUninitialized)
		{
		// Do something the first time RunL() is called
		iState = EInitialized;
		}
	else if (iState != EError)
		{
		// Do something
		}*/
	//iTimer.After(iStatus, 1000000); // Set for 1 sec later
	
	switch (iStatus.Int()) {
        // The fix is valid
        case KErrNone:
        // The fix has only partially valid information.
        // It is guaranteed to only have a valid timestamp
        //case KPositionPartialUpdate:
        // case KPositionQualityLoss: // TODO: Maybe uncomment
            {
            RDebug::Print(_L("Position recieved"));

            /*if (iState != EPositionRecieved) {
				iState = EPositionRecieved;
				TRAP_IGNORE(
					iListener->onConnectedL();
				);
            }*/
            SetState(EPositionRecieved);
            
            
            // Pre process the position information
            //PositionUpdatedL();
            TRAP_IGNORE(
            	iListener->OnPositionUpdatedL(iLastPosInfo);
            );
            
			iPositioner.NotifyPositionUpdate(iLastPosInfo, iStatus);
			SetActive();
            
            break;
            }
            
        // The data class used was not supported
        //case KErrArgument:
        /*    {
            break;
            }*/
            
        // The position data could not be delivered
        //case KPositionQualityLoss:
        /*    {
            break;
            }*/
            
        // Access is denied
        //case KErrAccessDenied:
            /*{
            break;
            }*/
            
        // Request timed out
        /*case KErrTimedOut:
            {
            break;
            }*/
            
        // The request was canceled
        /*case KErrCancel:
            {
            break;
            }*/
            
        /*// There is no last known position
        case KErrUnknown:
        // The fix has only partially valid information.
        // It is guaranteed to only have a valid timestamp
        case KPositionPartialUpdate:*/
            
        case KErrTimedOut:
            {
            RDebug::Print(_L("Positioning request is timed out"));
            
            /*if (iState != EPositionNotRecieved) {
				iState = EPositionNotRecieved;
				TRAP_IGNORE(
					iListener->onDisConnectedL();
				);
            }*/
            SetState(EPositionNotRecieved);
            
			iPositioner.NotifyPositionUpdate(iLastPosInfo, iStatus);
			SetActive();
			
            break;
            }
            
        case KErrCancel:
        	{
        	RDebug::Print(_L("Positioning request cancelled"));
        	
        	//setState(EStopped); // Not needed - State already changed in DoCancel
        	break;
        	}
            
        // Unrecoverable errors.
        default:
            {
            RDebug::Print(_L("Error in RunL: %d"), iStatus.Int());
            
            SetState(EStopped);
            
            TRAP_IGNORE(
            		iListener->OnErrorL(iStatus.Int());
            );

            break;
            }
	}
	
	//SetActive(); // Tell scheduler a request is active
	}

/*TInt CPositionRequestor::RunError(TInt aError)
	{
	//return KErrNone;
	return aError;
	}*/
	
inline TInt CPositionRequestor::State() const
	{
	return iState;
	}

void CPositionRequestor::SetState(TInt aState)
	{
	if (iState != aState) {
		if (aState == EPositionRecieved) {
			TRAP_IGNORE(
					iListener->OnConnectedL(); // FixMe: Leave in onConnectedL not cached (why???)
			);
		} else if (iState == EPositionRecieved ||
				iState == EStopped && aState == EPositionNotRecieved) {
			TRAP_IGNORE(
					iListener->OnDisconnectedL();
			);
		}
	}
		
	iState = aState;
	}

/*inline*/ TBool CPositionRequestor::IsRunning() const
	{
	return iState != EStopped;
	}



// CDynamicPositionRequestor

const TUint KMaxSpeedCalculationPeriod	= KSecond * 60;
//const TUint KPointsCachePeriod			= KSecond * 60;
const TReal KDistanceBetweenPoints		= 30.0;
const TUint KPositionMinUpdateInterval	= KSecond * 1;
const TUint KPositionMaxUpdateInterval	= KSecond * /*30*/ 10;

CDynamicPositionRequestor::CDynamicPositionRequestor(MPositionListener *aListener) :
	CPositionRequestor(aListener, KPositionMinUpdateInterval, KPositionMinUpdateInterval + KSecond)
	{
	}

CDynamicPositionRequestor::~CDynamicPositionRequestor()
	{
	delete iPointsCache;
	
	// ToDo: Run parent destructor needed?
	}

CDynamicPositionRequestor* CDynamicPositionRequestor::NewLC(MPositionListener *aPositionListener)
	{
	CDynamicPositionRequestor* self = new (ELeave) CDynamicPositionRequestor(aPositionListener);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	}

CDynamicPositionRequestor* CDynamicPositionRequestor::NewL(MPositionListener *aPositionListener)
	{
	CDynamicPositionRequestor* self = CDynamicPositionRequestor::NewLC(aPositionListener);
	CleanupStack::Pop(); // self;
	return self;
	}

TTimeIntervalMicroSeconds CDynamicPositionRequestor::UpdateInterval()
	{
	return iUpdateOptions.UpdateInterval();
	}

void CDynamicPositionRequestor::ConstructL()
	{
	iPointsCache = new (ELeave) CPointsCache(KMaxSpeedCalculationPeriod);
	
	CPositionRequestor::ConstructL(); // Run initialization in parent class
	}

/*TReal32 CDynamicPositionRequestor::MaxSpeedDuringPeriod()
	{
	
	}*/

void CDynamicPositionRequestor::RunL()
	{
	switch (iStatus.Int())
		{
		// The fix is valid
		case KErrNone:
		//case KPositionQualityLoss:
			{
			TPosition pos;
			iLastPosInfo.GetPosition(pos);
			
			iPointsCache->AddPoint(pos);
			
			//TReal32 speed = iPointsCache->MaxSpeed();
			TReal32 speed;
			TTimeIntervalMicroSeconds updateInterval;
			if (iPointsCache->GetMaxSpeed(speed) != KErrNone)
				{
				updateInterval = KPositionMinUpdateInterval;
				}
			else
				{
				TReal time;
				User::LeaveIfError(Math::Round(time, KDistanceBetweenPoints / speed, 0)); // Round to seconds
														// to prevent too often positioner options updated
				updateInterval = TTimeIntervalMicroSeconds(time * KSecond);
				// Use range restrictions
				updateInterval = Min(
						Max(updateInterval, KPositionMinUpdateInterval),
						KPositionMaxUpdateInterval);
				}
			
			if (updateInterval != iUpdateOptions.UpdateInterval())
				{
				iUpdateOptions.SetUpdateInterval(updateInterval);
				// Update timeout must not be less than update interval
				iUpdateOptions.SetUpdateTimeOut(updateInterval.Int64() + KSecond);
				iPositioner.SetUpdateOptions(iUpdateOptions); // Update positioner settings
				RDebug::Print(_L("Update interval changed to %d ms"), updateInterval.Int64());
				}
			
			break;
			}
		}
	
	CPositionRequestor::RunL();
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

void CPointsCache::AddPoint(const TPosition &aPos)
	{
	ClearOldPoints();
	
	RDebug::Print(_L("Point added"));
	iPoints.Append(aPos);
	}
	
//TReal32 CPointsCache::GetMaxSpeed()
TInt CPointsCache::GetMaxSpeed(TReal32 &aSpeed) 
	{
	ClearOldPoints();
	
	TUint count = iPoints.Count();
	
	if (count < 2)
		{
		RDebug::Print(_L("Can`t calculate max speed - not enough points in cache (%d)!"), count);
		return KErrGeneral; // Can`t calculate speed
			// ToDo: Use any specific error code
		}
	
	//TReal32 maxSpeed = 0;
	aSpeed = 0;
	TReal32 speed;
	for (/*TUint*/ TInt i = /*0*/1; i < count; i++)
		{
		//maxSpeed = Max(maxSpeed, iPoints[i].Speed()));
		iPoints[i].Speed(iPoints[i - 1], speed); // ToDo: What about handling errors?
		aSpeed = Max(speed, aSpeed);
		}
	
	//return maxSpeed;
	RDebug::Print(_L("Max speed: %.1f m/s (total cached points: %d)"), aSpeed, count);
	return KErrNone;
	}

void CPointsCache::ClearOldPoints()
	{
	if (iPoints.Count() == 0)
		return;
	
	TTime time;
	time.UniversalTime();
	time -= iPeriod;
	
#ifdef __WINS__
	TUint deletedCount = 0;
#endif
	for (/*TUint*/ TInt i = iPoints.Count() - 1; i >= 0; i--) // Bypass from the end
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
		RDebug::Print(_L("Deleted %d outdated points"), deletedCount);
#endif
	}

