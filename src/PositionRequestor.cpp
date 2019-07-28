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


//const TInt KGPSModuleID = 270526858;
//const TInt KSecond = 1000000;
//const TInt KDefaultPositionUpdateInterval = /* 5 * */ KSecond;
//const TInt KDefaultPositionUpdateTimeOut = /*15 * KSecond*/ KPositionUpdateInterval * 5; // TODO: Set value in constructor

_LIT(KRequestorString, "MyRequestor"); // ToDo: Change


CPositionRequestor::CPositionRequestor(MPositionListener *aListener,
		TTimeIntervalMicroSeconds aUpdateInterval,
		TTimeIntervalMicroSeconds aUpdateTimeOut) :
	CActive(EPriorityStandard), // Standard priority
	iState(EStopped),
	iListener(aListener),
	iUpdateInterval(aUpdateInterval),
	iUpdateTimeOut(aUpdateTimeOut)
	{
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
	TPositionUpdateOptions updateOpts;
	// Update the position every 30 sec
	updateOpts.SetUpdateInterval(iUpdateInterval);
	// Set update request timeout to 15 seconds
	updateOpts.SetUpdateTimeOut(iUpdateTimeOut);
	//updateOpts.SetMaxUpdateAge();
	//updateOpts.SetAcceptPartialUpdates(ETrue);
	// Set the options
	User::LeaveIfError(iPositioner.SetUpdateOptions(updateOpts));
	
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
        	//setState(EStopped); // Not needed - State already changed in DoCancel
        	break;
        	}
            
        // Unrecoverable errors.
        default:
            {            
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
