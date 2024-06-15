#include "StationFinder.h"
#include <Alert.h>
#include <Catalog.h>
#include <Cursor.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TranslationUtils.h>
#include <View.h>
#include "HttpUtils.h"
#include "RadioApp.h"
#include "Utils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StationFinder"


std::vector<std::pair<char*, InstantiateFunc> > StationFinderServices::sServices;


StationFinderServices::~StationFinderServices()
{
	sServices.clear();
}


void
StationFinderServices::Register(char* serviceName, InstantiateFunc instantiate)
{
	std::pair<char*, InstantiateFunc> service(serviceName, instantiate);
	sServices.push_back(service);
}


int32
StationFinderServices::CountItems()
{
	return sServices.size();
}


StationFinderService*
StationFinderServices::Instantiate(char* name)
{
	for (uint32 i = 0; i < sServices.size(); i++) {
		std::pair<char*, InstantiateFunc> service = sServices[i];
		if (!strcmp(service.first, name))
			return service.second();
	}

	return NULL;
}


char*
StationFinderServices::Name(int i)
{
	return sServices[i].first;
}


FindByCapability::FindByCapability(char* name)
	: fName(name),
	  fKeywords()
{
}


FindByCapability::FindByCapability(char* name, char* keyWords, char* delimiter)
	: fName(name),
	  fKeywords()
{
	SetKeyWords(keyWords, delimiter);
}


FindByCapability::~FindByCapability()
{
	fKeywords.MakeEmpty();
}


bool
FindByCapability::HasKeyWords()
{
	return !fKeywords.IsEmpty();
}


const BStringList*
FindByCapability::KeyWords()
{
	return &fKeywords;
}


const char*
FindByCapability::Name()
{
	return fName.String();
}


void
FindByCapability::SetKeyWords(char* keyWords, char* delimiter)
{
	BString tmp(keyWords);
	tmp.Split(delimiter, true, fKeywords);
}


StationFinderService::StationFinderService()
	: serviceName("unknown"),
	  serviceHomePage(""),
	  serviceLogo(NULL),
	  findByCapabilities(5, true)
{
}


StationFinderService::~StationFinderService()
{
	delete serviceLogo;
	findByCapabilities.MakeEmpty(true);
}


void
StationFinderService::Register(char* name, InstantiateFunc instantiate)
{
	StationFinderServices::Register(name, instantiate);
}


BBitmap*
StationFinderService::RetrieveLogo(BUrl url)
{
	BBitmap* bm = NULL;
	BString contentType("image/*");

	BMallocIO* bmData = HttpUtils::GetAll(url, NULL, 3000, &contentType);
	if (bmData != NULL) {
		bm = BTranslationUtils::GetBitmap(bmData);
		if (bm != NULL && bm->InitCheck() != B_OK) {
			delete bm;
			bm = NULL;
		}

		delete bmData;
	}

	return bm;
}


FindByCapability*
StationFinderService::RegisterSearchCapability(char* name)
{
	FindByCapability* newCapability = new FindByCapability(name);
	findByCapabilities.AddItem(newCapability);

	return newCapability;
}


FindByCapability*
StationFinderService::RegisterSearchCapability(char* name, char* keywords, char* delimiter)
{
	FindByCapability* newCapability = new FindByCapability(name, keywords, delimiter);
	findByCapabilities.AddItem(newCapability);

	return newCapability;
}


StationFinderWindow::StationFinderWindow(BWindow* parent)
	: BWindow(
		BRect(0, 0, 300, 150), B_TRANSLATE("Find stations"), B_TITLED_WINDOW, B_CLOSE_ON_ESCAPE),
	  fCurrentService(NULL)
{
	fMessenger = new BMessenger(parent);

	CenterIn(parent->Frame());

	fTxSearch = new BTextControl(NULL, NULL, new BMessage(MSG_TXSEARCH));

	fKwSearch = new BOptionPopUp("fKwSearch", NULL, new BMessage(MSG_KWSEARCH));

	fBnSearch = new BButton("fBnSearch", NULL, new BMessage(MSG_BNSEARCH));
	fBnSearch->SetIcon(Utils::ResourceBitmap(RES_BN_SEARCH));

	fDdServices = new BOptionPopUp("fDdServices", NULL, new BMessage(MSG_SELECT_SERVICE));
	int currentServiceIndex = 0;
	const char* settingsServiceName = ((RadioApp*)be_app)->Settings.StationFinderName();
	for (int32 i = 0; i < StationFinderServices::CountItems(); i++) {
		const char* serviceName = StationFinderServices::Name(i);
		if (settingsServiceName && !strcmp(serviceName, settingsServiceName))
			currentServiceIndex = i;
		fDdServices->AddOption(serviceName, i);
	}

	fBnVisit = new BButton("fBnVisit", "", new BMessage(MSG_VISIT_SERVICE));
	fBnVisit->SetIcon(Utils::ResourceBitmap(RES_BN_WEB));

	fDdSearchBy = new BOptionPopUp("fDdSearchBy", NULL, new BMessage(MSG_SEARCH_BY));

	fResultView = new StationListView();
	fResultView->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT));

	fBnAdd = new BButton("fBnAdd", B_TRANSLATE("Add"), new BMessage(MSG_ADD_STATION));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.AddGrid(3, 3, 0.0)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(new BStringView("lbServices", B_TRANSLATE("Service")), 0, 0)
		.Add(fDdServices, 1, 0)
		.Add(fBnVisit, 2, 0)

		.Add(new BStringView("lbSearchBy", B_TRANSLATE("Search by")), 0, 1)
		.Add(fDdSearchBy, 1, 1)

		.Add(new BStringView("lbSearchFor", B_TRANSLATE("Search for")), 0, 2)
		.Add(fTxSearch, 1, 2)
		.Add(fKwSearch, 1, 2)
		.Add(fBnSearch, 2, 2)
		.GetLayout(&fSearchGrid)
		.End()

		.Add(new BScrollView("srollResult", fResultView, 0, false, true), 0.9)

		.AddGrid(3, 1, 0.0)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fBnAdd, 2, 0)
		.SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_VERTICAL_UNSET))
		.End()
		.End();

	fBnAdd->SetEnabled(false);

	fResultView->SetInvocationMessage(new BMessage(MSG_ADD_STATION));
	fResultView->SetSelectionMessage(new BMessage(MSG_SELECT_STATION));

	Layout(true);
	ResizeToPreferred();
	ResizeBy(100, 0);

	fTxSearch->MakeFocus(true);
	fBnSearch->MakeDefault(true);
	fTxSearch->StartWatchingAll(this);

	SelectService(currentServiceIndex);
	fDdServices->SetValue(currentServiceIndex);
}


StationFinderWindow::~StationFinderWindow()
{
	delete fCurrentService;

	fTxSearch->StopWatchingAll(this);

	delete fMessenger;
}


bool
StationFinderWindow::QuitRequested()
{
	fMessenger->SendMessage(new BMessage(MSG_SEARCH_CLOSE));
	return true;
}


void
StationFinderWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_BNSEARCH:
		{
			if (fTxSearch->IsEnabled()) {
				if (fTxSearch->Text()[0]) {
					DoSearch(fTxSearch->Text());
					fResultView->MakeFocus(true);
				}
			} else if (fKwSearch->Value() >= 0) {
				DoSearch(fCurrentService->Capability(fDdSearchBy->Value())
							 ->KeyWords()
							 ->StringAt(fKwSearch->Value()));
			}

			break;
		}

		case MSG_ADD_STATION:
		{
			int32 index = msg->GetInt32("index", -1);
			if (index < 0)
				index = fResultView->CurrentSelection(0);

			if (index >= 0) {
				Station* station = fResultView->StationAt(index);

				status_t probeStatus = station->Probe();
				if (probeStatus == B_OK) {
					BMessage* dispatch = new BMessage(msg->what);
					dispatch->AddPointer("station", station);

					if (fMessenger->SendMessage(dispatch) == B_OK) {
						StationListViewItem* item
							= (StationListViewItem*)fResultView->RemoveItem(index);
						item->ClearStation();
						delete item;
					}
				} else {
					BString msg;
					msg.SetToFormat(B_TRANSLATE("Station %s did not respond correctly and "
												"could not be added"),
						station->Name()->String());

					(new BAlert(B_TRANSLATE("Add station failed"), msg, B_TRANSLATE("OK")))->Go();
				}
			}

			break;
		}

		case MSG_UPDATE_STATION:
		{
			Station* st = NULL;
			if (msg->FindPointer("station", (void**)&st) == B_OK) {
				fResultView->InvalidateItem(fResultView->IndexOf(fResultView->Item(st)));
			}

			break;
		}

		case MSG_SELECT_STATION:
		{
			bool inResults = (msg->GetInt32("index", -1) >= 0);
			fBnAdd->SetEnabled(inResults);
			fBnAdd->MakeDefault(inResults);

			break;
		}

		case MSG_SELECT_SERVICE:
			SelectService(fDdServices->Value());
			break;

		case MSG_SEARCH_BY:
			SelectCapability(fDdSearchBy->Value());
			break;

		case MSG_VISIT_SERVICE:
		{
			if (fCurrentService != NULL && fCurrentService->serviceHomePage != NULL
				&& fCurrentService->serviceHomePage.IsValid())
				fCurrentService->serviceHomePage.OpenWithPreferredApplication(true);
			break;
		}

		case B_OBSERVER_NOTICE_CHANGE:
			fBnSearch->MakeDefault(true);
			break;

		case MSG_TXSEARCH:
			fBnAdd->MakeDefault(false);
			fBnSearch->MakeDefault(true);
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}


void
StationFinderWindow::SelectService(int index)
{
	char* serviceName = StationFinderServices::Name(index);
	if (serviceName == NULL)
		return;

	StationFinderService* selectedService = StationFinderServices::Instantiate(serviceName);
	if (selectedService == NULL)
		return;

	if (fCurrentService != NULL) {
		delete fCurrentService;
		fCurrentService = NULL;
	}

	fCurrentService = selectedService;

	((RadioApp*)be_app)->Settings.SetStationFinderName(serviceName);

	while (fDdSearchBy->CountOptions() > 0)
		fDdSearchBy->RemoveOptionAt(0);

	for (int32 i = 0; i < fCurrentService->CountCapabilities(); i++)
		fDdSearchBy->AddOption(fCurrentService->Capability(i)->Name(), i);

	if (fDdSearchBy->CountOptions() != 0)
		SelectCapability(0);
}


void
StationFinderWindow::SelectCapability(int index)
{
	if (fCurrentService == NULL)
		return;

	FindByCapability* capability = fCurrentService->Capability(index);
	if (capability == NULL)
		return;

	if (capability->HasKeyWords()) {
		while (fKwSearch->CountOptions() > 0)
			fKwSearch->RemoveOptionAt(0);

		for (int32 i = 0; i < capability->KeyWords()->CountStrings(); i++)
			fKwSearch->AddOption(capability->KeyWords()->StringAt(i), i);

		fSearchGrid->RemoveView(fTxSearch);
		fSearchGrid->AddView(fKwSearch, 1, 2);

		fKwSearch->SetEnabled(true);
		fTxSearch->SetEnabled(false);

		Layout(true);
	} else {
		fSearchGrid->RemoveView(fKwSearch);
		fSearchGrid->AddView(fTxSearch, 1, 2);

		fKwSearch->SetEnabled(false);
		fTxSearch->SetEnabled(true);

		Layout(true);
	}
}


void
StationFinderWindow::DoSearch(const char* text)
{
	fResultView->MakeEmpty();
	if (fCurrentService == NULL)
		return;

	be_app->SetCursor(new BCursor(B_CURSOR_ID_PROGRESS));

	BObjectList<Station>* result = fCurrentService->FindBy(fDdSearchBy->Value(), text, this);
	if (result != NULL) {
		for (int32 i = 0; i < result->CountItems(); i++)
			fResultView->AddItem(new StationListViewItem(result->ItemAt(i)));

		result->MakeEmpty(false);
		delete result;
	}

	be_app->SetCursor(B_CURSOR_SYSTEM_DEFAULT);
}
