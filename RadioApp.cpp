#include <Catalog.h>

#include "About.h"
#include "RadioApp.h"
#include "StationFinder.h"
#include "StationFinderListenLive.h"
#include "StationFinderRadioNetwork.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RadioApp"


RadioApp::RadioApp()
	:
	BApplication(appSignature),
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
	if (fArgvMessage)
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
	if (count) {
		if (mainWindow)
			mainWindow->PostMessage(fArgvMessage);
	} else {
		delete fArgvMessage;
		fArgvMessage = NULL;
	}
}


void
RadioApp::AboutRequested()
{
	About* about = new About();
	about->Show();
}
/**
 * Application entry point
 */
int
main(int argc, char* argv[])
{
	StationFinderRadioNetwork::RegisterSelf();
	StationFinderListenLive::RegisterSelf();

	new RadioApp();
	be_app->Run();
	delete be_app;
}
