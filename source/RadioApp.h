/*
 * Copyright (C) 2017 Kai Niessen <kai.niessen@online.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef _RADIO_APP_H
#define _RADIO_APP_H


#include <Application.h>
#include <Message.h>

#include "MainWindow.h"
#include "RadioSettings.h"


#define kAppSignature "application/x-vnd.Fishpond-StreamRadio"


class RadioApp : BApplication {
public:
	RadioApp();
	~RadioApp();

	virtual void ReadyToRun();
	virtual void RefsReceived(BMessage* message);
	virtual void ArgvReceived(int32 argc, char** argv);
	virtual void AboutRequested();

	MainWindow* mainWindow;

	RadioSettings Settings;

private:
	BMessage* fArgvMessage;
};


#endif	// _RADIO_APP_H
