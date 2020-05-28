/* 
 * File:   RadioApp.h
 * Author: Kai Niessen
 *
 * Created on 26. Februar 2013, 02:40
 */

#ifndef MAIN_H
#define	MAIN_H

#include <Application.h>
#include <Message.h>
#include "MainWindow.h"
#include "RadioSettings.h"

#define appSignature "application/x-vnd.Fishpond-Radio"

/**
 * Application class
 */
class RadioApp : BApplication {
public:
								RadioApp(); 
								~RadioApp();
    virtual void				ReadyToRun();
    virtual void				RefsReceived(BMessage* message);
	virtual void				ArgvReceived(int32 argc, char** argv);
	virtual void				AboutRequested();
    MainWindow*					mainWindow;
	RadioSettings				Settings;
private:
	BMessage*					fArgvMessage;
};
#endif	/* MAIN_H */
