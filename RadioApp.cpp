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


#include <AboutWindow.h>
#include <Catalog.h>

#include "RadioApp.h"
#include "StationFinder.h"
#include "StationFinderListenLive.h"
#include "StationFinderRadioNetwork.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RadioApp"


RadioApp::RadioApp()
	:
	BApplication(kAppSignature),
	fArgvMessage(NULL)
{
}


RadioApp::~RadioApp()
{
}


void
RadioApp::ReadyToRun()
{
	mainWindow = new MainWindow();
	mainWindow->Show();
	if (fArgvMessage != NULL)
		mainWindow->PostMessage(fArgvMessage);
}


void
RadioApp::RefsReceived(BMessage* message)
{
	mainWindow->PostMessage(message);
}


void
RadioApp::ArgvReceived(int32 argc, char** argv)
{
	fArgvMessage = new BMessage(B_REFS_RECEIVED);

	int32 count = 0;
	for (int32 i = 1; i < argc; i++) {
		char* arg = argv[i];
		if (!strncmp(arg, "--help", 7)) {
			printf(B_TRANSLATE("Usage: StreamRadio <filename>\n"
				"<filename> should be a Shoutcast playlist file.\n"
				"If the station already exists, it is made to play "
				"otherwise it is added.\n"));
			continue;
		}

		BEntry entry(arg);
		if (entry.Exists()) {
			entry_ref entryRef;
			entry.GetRef(&entryRef);
			fArgvMessage->AddRef("refs", &entryRef);
			count++;
		}
	}

	if (count != 0) {
		if (mainWindow != NULL)
			mainWindow->PostMessage(fArgvMessage);
	} else {
		delete fArgvMessage;
		fArgvMessage = NULL;
	}
}


void
RadioApp::AboutRequested()
{
	BAboutWindow * about = new BAboutWindow(
		B_TRANSLATE_SYSTEM_NAME("StreamRadio"), kAppSignature);

	const char* kAuthors[] = {
		"Fishpond",
		"Humdinger",
		"Jacob Secunda",
		"Javier Steinaker",
		NULL
	};

	const char* kCopyright = "The HaikuArchives team";

	about->AddDescription(B_TRANSLATE("A player for online radio."));
	about->AddAuthors(kAuthors);
	about->AddCopyright(2017, kCopyright, NULL);

	about->Show();
}


/**
 * Application entry point
 */
int
main(int argc, char* argv[])
{
	StationFinderRadioNetwork::RegisterSelf();

	// FIXME: "listenlive.eu" no longer seems to exist, though it looks like
	// "radiomap.eu" might be its successor? This plugin crashes after searching
	// anyway...
	// StationFinderListenLive::RegisterSelf();

	new RadioApp();
	be_app->Run();

	delete be_app;
}
