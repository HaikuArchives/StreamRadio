#include "StationFinder.h"
#include <Alert.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Cursor.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TranslationUtils.h>
#include <View.h>
#include "RadioApp.h"
#include "Utils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StationFinder"


FindByCapability::FindByCapability(const char* name)
	: BLocker(),
	  fKeywords()
{
	fName.SetTo(name);
}


FindByCapability::FindByCapability(const char* name, BStringList* keywords)
	: FindByCapability(name)
{
	fKeywords.Add(*keywords);
}

FindByCapability::FindByCapability(const char* name, char* keywords, char* delimiter)
	: FindByCapability(name)
{
	BString tmp(keywords);
	tmp.Split(delimiter, true, fKeywords);
}

FindByCapability::~FindByCapability()
{
	fKeywords.MakeEmpty();
}

void
FindByCapability::UpdateKeywords(const BStringList* keywords)
{
	if (Lock()) {
		fKeywords.MakeEmpty();
		if (keywords != NULL)
			fKeywords.Add(*keywords);
		Unlock();
	}
}


StationFinderService::StationFinderService()
	: serviceName("unknown"),
	  serviceHomePage("", true),
	  serviceLogo(NULL),
	  findByCapabilities(5)
{
}


StationFinderService::~StationFinderService()
{
	delete serviceLogo;
	findByCapabilities.MakeEmpty(true);
}


void
StationFinderService::Register(BString* name, InstantiateFunc instantiate)
{
	stationFinderServices.Register(name, instantiate);
}


BBitmap*
StationFinderService::RetrieveLogo(BUrl* url)
{
	BBitmap* bm = NULL;
	BString contentType("image/*");

	BMallocIO* bmData = HttpUtils::GetAll(url, NULL, 3000, &contentType);
	if (bmData != NULL) {
		bm = BTranslationUtils::GetBitmap(bmData);
		if (bm != NULL && (bm->InitCheck() != B_OK || !bm->IsValid())) {
			delete bm;
			bm = NULL;
		}

		delete bmData;
	}

	return bm;
}


uint32
StationFinderService::RegisterSearchCapability(const char* name)
{
	FindByCapability* newCapability = new FindByCapability(name);
	if (findByCapabilities.AddItem(newCapability))
		return findByCapabilities.IndexOf(newCapability);
	else
		return -1;
}


uint32
StationFinderService::RegisterSearchCapability(const char* name, BStringList* keywords)
{
	FindByCapability* newCapability = new FindByCapability(name, keywords);
	if (findByCapabilities.AddItem(newCapability))
		return findByCapabilities.IndexOf(newCapability);
	else
		return -1;
}


void
StationFinderService::UpdateKeywords(uint32 searchCapability, const BStringList* keywords)
{
	FindByCapability* capability = findByCapabilities.ItemAt(searchCapability);
	if (capability != NULL) {
		capability->UpdateKeywords(keywords);

		BMessage* msg = new BMessage(MSG_UPDATE_KEYWORDS);
		msg->AddUInt32("capability", searchCapability);
		msg->AddSpecifier("Window", B_TRANSLATE("Find stations"));
		be_app->PostMessage(msg);
	}
}


StationFinderWindow::StationFinderWindow(BWindow* parent)
	: BWindow(
		BRect(0, 0, 300, 150), B_TRANSLATE("Find stations"), B_TITLED_WINDOW, B_CLOSE_ON_ESCAPE),
	  fCurrentService(NULL),
	  fCurrentFindByCapability(NULL)
{
	fMessenger = new BMessenger(parent);

	CenterIn(parent->Frame());

	fTxSearch = new BTextControl(NULL, NULL, new BMessage(MSG_TXSEARCH));

	fKwSearch = new BOptionPopUp("fKwSearch", NULL, new BMessage(MSG_KWSEARCH));

	fBnSearch = new BButton("fBnSearch", NULL, new BMessage(MSG_BNSEARCH));
	fBnSearch->SetIcon(Utils::ResourceBitmap(
		RES_BN_SEARCH, be_control_look->ComposeIconSize(B_MINI_ICON).Height() - 1));
	float height;
	fKwSearch->GetPreferredSize(NULL, &height);
	BSize size(height, height);
	fBnSearch->SetExplicitSize(size);

	fDdServices = new BOptionPopUp("fDdServices", NULL, new BMessage(MSG_SELECT_SERVICE));
	int currentServiceIndex = 0;
	const char* settingsServiceName = ((RadioApp*)be_app)->Settings.StationFinderName();
	for (int32 i = 0; i < stationFinderServices.CountItems(); i++) {
		BString* serviceName = stationFinderServices.Name(i);
		if (settingsServiceName && serviceName->Compare(settingsServiceName))
			currentServiceIndex = i;
		fDdServices->AddOption(serviceName->String(), i);
	}

	fBnVisit = new BButton("fBnVisit", "", new BMessage(MSG_VISIT_SERVICE));
	fBnVisit->SetIcon(Utils::GetBrowserIcon());
	fBnVisit->SetExplicitSize(size);

	fDdSearchBy = new BOptionPopUp("fDdSearchBy", NULL, new BMessage(MSG_SEARCH_BY));

	fResultView = new StationListView();
	fResultView->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT));

	fBnAdd = new BButton("fBnAdd", B_TRANSLATE("Add"), new BMessage(MSG_ADD_STATION));

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.AddGrid(3, 3, 0.0)
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

		.Add(new BScrollView("scrollResult", fResultView, 0, false, true), 0.9)

		.AddGroup(B_HORIZONTAL)
		.AddGlue()
		.Add(fBnAdd)
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
			if (fTxSearch->Text()[0]) {
				DoSearch(fTxSearch->Text());
				fResultView->MakeFocus(true);
			}

			break;
		}

		case MSG_KWSEARCH:
		{
			if (fKwSearch->Value() < 0)
				break;

			FindByCapability* capability = fCurrentService->Capability(fDdSearchBy->Value());
			if (capability->Lock()) {
				int32 firstValue = 0;
				if (fKwSearch->GetOptionAt(0, NULL, &firstValue) && firstValue < 0)
					fKwSearch->RemoveOptionAt(0);

				DoSearch(capability->Keywords()->StringAt(fKwSearch->Value()));
				capability->Unlock();
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

		case MSG_UPDATE_KEYWORDS:
		{
			uint32 capability;
			if (msg->FindUInt32("capability", &capability) == B_OK) {
				UpdateCapability(capability);
			} else if (fCurrentService != NULL) {
				for (capability = 0; capability < fCurrentService->CountCapabilities();
					capability++)
					UpdateCapability(capability);
			}
			break;
		}

		default:
			BWindow::MessageReceived(msg);
	}
}


void
StationFinderWindow::SelectService(int index)
{
	BString* serviceName = stationFinderServices.Name(index);
	if (serviceName == NULL)
		return;

	if (fCurrentService != NULL && *fCurrentService->Name() == *serviceName)
		return;

	StationFinderService* selectedService = stationFinderServices.Instantiate(index);
	if (selectedService == NULL)
		return;

	delete fCurrentService;
	fCurrentService = selectedService;

	((RadioApp*)be_app)->Settings.SetStationFinderName(serviceName->String());

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
	if (capability == NULL || capability == fCurrentFindByCapability)
		return;

	fCurrentFindByCapability = capability;
	_RebuildSearchForWidget();
}


void
StationFinderWindow::UpdateCapability(int index)
{
	if (fCurrentService == NULL)
		return;

	FindByCapability* capability = fCurrentService->Capability(index);
	if (capability == NULL || capability != fCurrentFindByCapability)
		return;

	if (!capability->Lock())
		return;

	const char* widgetValue = "";
	if (fKwSearch->IsEnabled()) {
		int32 value = -1;
		fKwSearch->SelectedOption(&widgetValue, &value);
		if (value < 0)
			widgetValue = "";
	} else {
		widgetValue = fTxSearch->Text();
	}

	char* term = strdup(widgetValue);
	_RebuildSearchForWidget();
	if (capability->HasKeywords()) {
		if (term[0] != '\0') {
			int32 newValue = capability->Keywords()->IndexOf(term);
			if (newValue >= 0) {
				fKwSearch->RemoveOptionAt(0);
				fKwSearch->SetValue(newValue);
			}
		}
	} else {
		fTxSearch->SetText(term);
	}
	free(term);

	capability->Unlock();
}


void
StationFinderWindow::DoSearch(const char* text)
{
	if (text == NULL || text[0] == '\0')
		return;

	fResultView->MakeEmpty();
	if (fCurrentService == NULL)
		return;

	be_app->SetCursor(new BCursor(B_CURSOR_ID_PROGRESS));

	StationList* result = fCurrentService->FindBy(fDdSearchBy->Value(), text, this);
	if (result != NULL) {
		for (int32 i = 0; i < result->CountItems(); i++)
			fResultView->AddItem(new StationListViewItem(result->ItemAt(i)));

		result->MakeEmpty(false);
		delete result;
	}

	be_app->SetCursor(B_CURSOR_SYSTEM_DEFAULT);
}


void
StationFinderWindow::_RebuildSearchForWidget()
{
	if (!fCurrentFindByCapability->Lock())
		return;

	if (fCurrentFindByCapability->HasKeywords()) {
		while (fKwSearch->CountOptions() > 0)
			fKwSearch->RemoveOptionAt(0);

		fKwSearch->AddOption(B_TRANSLATE("Choose term"), -1);
		for (int32 i = 0; i < fCurrentFindByCapability->Keywords()->CountStrings(); i++)
			fKwSearch->AddOption(fCurrentFindByCapability->Keywords()->StringAt(i), i);

		fSearchGrid->RemoveView(fTxSearch);
		fSearchGrid->AddView(fKwSearch, 1, 2);

		fKwSearch->SetEnabled(true);
		fTxSearch->SetEnabled(false);
		fBnSearch->SetEnabled(false);

		Layout(true);
	} else {
		fSearchGrid->RemoveView(fKwSearch);
		fSearchGrid->AddView(fTxSearch, 1, 2);

		fKwSearch->SetEnabled(false);
		fTxSearch->SetEnabled(true);
		fBnSearch->SetEnabled(true);

		Layout(true);
	}

	fCurrentFindByCapability->Unlock();
}
