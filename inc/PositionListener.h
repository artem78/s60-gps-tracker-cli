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
		virtual void OnPositionUpdatedL() = 0;
		virtual void OnPositionPartialUpdated() = 0;
		virtual void OnConnectedL() = 0;
		virtual void OnDisconnectedL() = 0;
		virtual void OnErrorL(TInt aErrCode) = 0;
};

#endif /* POSITIONLISTENER_H_ */
