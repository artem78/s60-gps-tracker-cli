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
		virtual void OnPositionUpdated() = 0;
		virtual void OnPositionPartialUpdated() = 0;
		virtual void OnConnected() = 0;
		virtual void OnDisconnected() = 0;
		virtual void OnError(TInt aErrCode) = 0;
};

#endif /* POSITIONLISTENER_H_ */
