/*
 * KeyboardListener.h
 *
 *  Created on: 26.07.2019
 *      Author: user
 */

#ifndef KEYBOARDLISTENER_H_
#define KEYBOARDLISTENER_H_

#include <e32keys.h>

class MKeyboardListener
	{
public:
	virtual void OnKeyPressed(TKeyCode aKeyCode) = 0;
	};

#endif /* KEYBOARDLISTENER_H_ */
