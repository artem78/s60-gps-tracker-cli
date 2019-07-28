/*
 * PositionListener.h
 *
 *  Created on: 03.11.2018
 *      Author: user
 */

#ifndef POSITIONLISTENER_H_
#define POSITIONLISTENER_H_

#include <lbs.h>


class MPositionListener {
	public:
		// ToDo: Is L really needed in the methods below?
		virtual void OnPositionUpdatedL(const TPositionInfo &aPosInfo) = 0;
		virtual void OnConnectedL() = 0;
		virtual void OnDisconnectedL() = 0;
		virtual void OnErrorL(TInt aErrCode) = 0;
};

#endif /* POSITIONLISTENER_H_ */
