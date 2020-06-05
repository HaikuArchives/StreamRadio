#include "StationFinder.h"
#include "HttpUtils.h"
#include "RadioApp.h"
#include "Utils.h"
#include <Alert.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TranslationUtils.h>
#include <View.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StationFinder"


StationFinderServices::~StationFinderServices()
{
	fServices.clear();
}


void
StationFinderServices::Register(char* serviceName, InstantiateFunc instantiate)
{
	pair<char*, InstantiateFunc> service(serviceName, instantiate);
	fServices.push_back(service);
}


int
StationFinderServices::CountItems()
{
	return fServices.size();
}


StationFinderService*
StationFinderServices::Instantiate(char* name)
{
	for (int i = 0; i < fServices.size(); i++) {
		pair<char*, InstantiateFunc> service = fServices[i];
		if (!strcmp(service.first, name))
			return service.second();
	}
	return NULL;
}


char*
StationFinderServices::Name(int i)
{
	return fServices[i].first;
}

vector<pair<char*, InstantiateFunc> > StationFinderServices::fServices;


FindByCapability::FindByCapability(char* name)
	:
	fName(name),
	fKeywords()
{
}


FindByCapability::FindByCapability(char* name, char* KeyWords, char* delimiter)
	:
	fName(name),
	fKeywords()
{
	SetKeyWords(KeyWords, delimiter);
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
FindByCapability::SetKeyWords(char* KeyWords, char* delimiter)
{
	BString tmp(KeyWords);
	tmp.Split(delimiter, true, fKeywords);
}


StationFinderService::StationFinderService()
	:
	serviceName("unknown"),
	serviceHomePage(""),
	serviceLogo(NULL),
	findByCapabilities(5, true)
{
}


StationFinderService::~StationFinderService()
{
	if (serviceLogo)
		delete serviceLogo;
	findByCapabilities.MakeEmpty(true);
}


void
StationFinderService::Register(char* name, InstantiateFunc instantiate)
{
	StationFinderServices::Register(name, instantiate);
}


BBitmap*
StationFinderService::logo(BUrl url)
{
	BBitmap* bm = NULL;
	BString contentType("image/*");
	BMallocIO* bmData = HttpUtils::GetAll(url, NULL, 3000, &contentType);

	if (bmData) {

		bm = BTranslationUtils::GetBitmap(bmData);
		if (bm == NULL || bm->InitCheck() != B_OK)
			bm == NULL;
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
StationFinderService::RegisterSearchCapability(
	char* name, char* keywords, char* delimiter)
{
	FindByCapability* newCapability
		= new FindByCapability(name, keywords, delimiter);
	findByCapabilities.AddItem(newCapability);
	return newCapability;
}


StationFinderWindow::StationFinderWindow(BWindow* parent)
	:
	BWindow(BRect(0, 0, 300, 150), B_TRANSLATE("Find stations"),
		B_TITLED_WINDOW, B_CLOSE_ON_ESCAPE),
	currentService(NULL)
{
	messenger = new BMessenger(parent);

	CenterIn(parent->Frame());

	txSearch = new BTextControl(NULL, NULL, new BMessage(MSG_TXSEARCH));
	kwSearch = new BOptionPopUp("kwSearch", NULL, new BMessage(MSG_KWSEARCH));
	bnSearch = new BButton("bnSearch", NULL, new BMessage(MSG_BNSEARCH));
	bnSearch->SetIcon(Utils::ResourceBitmap(RES_BN_SEARCH));


	ddServices = new BOptionPopUp(
		"ddServices", NULL, new BMessage(MSG_SELECT_SERVICE));
	int currentServiceIndex = 0;
	const char* settingsServiceName
		= ((RadioApp*) be_app)->Settings.StationFinderName();
	for (int i = 0; i < StationFinderServices::CountItems(); i++) {
		const char* serviceName = StationFinderServices::Name(i);
		if (settingsServiceName && !strcmp(serviceName, settingsServiceName))
			currentServiceIndex = i;
		ddServices->AddOption(serviceName, i);
	}
	bnVisit = new BButton("bnVisit", "", new BMessage(MSG_VISIT_SERVICE));
	bnVisit->SetIcon(Utils::ResourceBitmap(RES_BN_WEB));

	ddSearchBy
		= new BOptionPopUp("ddSearchBy", NULL, new BMessage(MSG_SEARCH_BY));
	resultView = new StationListView();
	resultView->SetExplicitAlignment(
		BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT));
	bnAdd = new BButton(
		"bnAdd", B_TRANSLATE("Add"), new BMessage(MSG_ADD_STATION));
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.AddGrid(3, 3, 0.0)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(new BStringView("lbServices", B_TRANSLATE("Service")), 0, 0)
		.Add(ddServices, 1, 0)
		.Add(bnVisit, 2, 0)

		.Add(new BStringView("lbSearchBy", B_TRANSLATE("Search by")), 0, 1)
		.Add(ddSearchBy, 1, 1)

		.Add(new BStringView("lbSearchFor", B_TRANSLATE("Search for")), 0, 2)
		.Add(txSearch, 1, 2)
		.Add(kwSearch, 1, 2)
		.Add(bnSearch, 2, 2)
		.GetLayout(&searchGrid)
		.End()

		.Add(new BScrollView(
				 "srollResult", resultView, 0, B_FOLLOW_ALL_SIDES, false, true),
			0.9)
		.AddGrid(3, 1, 0.0)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(bnAdd, 2, 0)
		.SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_VERTICAL_UNSET))
		.End()
		.End();

	bnAdd->SetEnabled(false);
	resultView->SetInvocationMessage(new BMessage(MSG_ADD_STATION));
	resultView->SetSelectionMessage(new BMessage(MSG_SELECT_STATION));
	Layout(true);
	ResizeToPreferred();
	ResizeBy(100, 0);

	txSearch->MakeFocus(true);
	bnSearch->MakeDefault(true);
	txSearch->StartWatchingAll(this);
	SelectService(currentServiceIndex);
	ddServices->SetValue(currentServiceIndex);
}


StationFinderWindow::~StationFinderWindow()
{
	if (currentService)
		delete currentService;
	resultView->MakeEmpty();
	txSearch->StopWatchingAll(this);
	delete messenger;
}


bool
StationFinderWindow::QuitRequested()
{
	messenger->SendMessage(new BMessage(MSG_SEARCH_CLOSE));
	return true;
}


void
StationFinderWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_BNSEARCH:
		{
			if (txSearch->IsEnabled()) {
				if (txSearch->Text()[0]) {
					DoSearch(txSearch->Text());
					resultView->MakeFocus(true);
				}
			} else if (kwSearch->Value() >= 0)
				DoSearch(currentService->Capability(ddSearchBy->Value())
							 ->KeyWords()
							 ->StringAt(kwSearch->Value()));
			break;
		}
		case MSG_ADD_STATION:
		{
			int32 index = msg->GetInt32("index", -1);
			if (index < 0)
				index = resultView->CurrentSelection(0);
			if (index >= 0) {
				Station* station = resultView->StationAt(index);
				status_t probeStatus = station->Probe();
				if (probeStatus == B_OK) {
					BMessage* dispatch = new BMessage(msg->what);
					dispatch->AddPointer("station", station);
					if (messenger->SendMessage(dispatch) == B_OK)
						resultView->RemoveItem(index);
				} else {
					BString msg;
					msg.SetToFormat(
						B_TRANSLATE("Station %s did not respond correctly and "
									"could not be added"),
						station->Name()->String());
					(new BAlert(B_TRANSLATE("Add station failed"), msg,
						 B_TRANSLATE("OK")))
						->Go();
				}
			}
			break;
		}

		case MSG_UPDATE_STATION:
		{
			Station* st = NULL;
			status_t status = msg->FindPointer("station", (void**) &st);
			if (status == B_OK)
				resultView->InvalidateItem(
					resultView->IndexOf(resultView->Item(st)));
			break;
		}

		case MSG_SELECT_STATION:
		{
			bool inResults = (msg->GetInt32("index", -1) >= 0);
			bnAdd->SetEnabled(inResults);
			bnAdd->MakeDefault(inResults);
			break;
		}

		case MSG_SELECT_SERVICE:
			SelectService(ddServices->Value());
			break;

		case MSG_SEARCH_BY:
			SelectCapability(ddSearchBy->Value());
			break;

		case MSG_VISIT_SERVICE:
		{
			if (currentService && currentService->serviceHomePage
				&& currentService->serviceHomePage.IsValid())
				currentService->serviceHomePage.OpenWithPreferredApplication(
					true);
			break;
		}
		case B_OBSERVER_NOTICE_CHANGE:
			bnSearch->MakeDefault(true);
			break;

		case MSG_TXSEARCH:
			bnAdd->MakeDefault(false);
			bnSearch->MakeDefault(true);
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}


void
StationFinderWindow::SelectService(int index)
{
	if (currentService) {
		delete currentService;
		currentService = NULL;
	}

	char* serviceName = StationFinderServices::Name(index);
	if (!serviceName)
		return;

	currentService = StationFinderServices::Instantiate(serviceName);
	((RadioApp*) be_app)->Settings.SetStationFinderName(serviceName);
	while (ddSearchBy->CountOptions())
		ddSearchBy->RemoveOptionAt(0);

	for (int i = 0; i < currentService->CountCapabilities(); i++)
		ddSearchBy->AddOption(currentService->Capability(i)->Name(), i);

	if (ddSearchBy->CountOptions())
		SelectCapability(0);
}


void
StationFinderWindow::SelectCapability(int index)
{
	if (!currentService)
		return;

	FindByCapability* capability = currentService->Capability(index);
	if (!capability)
		return;

	if (capability->HasKeyWords()) {
		while (kwSearch->CountOptions())
			kwSearch->RemoveOptionAt(0);
		for (int i = 0; i < capability->KeyWords()->CountStrings(); i++)
			kwSearch->AddOption(capability->KeyWords()->StringAt(i), i);

		searchGrid->RemoveView(txSearch);
		searchGrid->AddView(kwSearch, 1, 2);
		kwSearch->SetEnabled(true);
		txSearch->SetEnabled(false);
		Layout(true);
	} else {
		searchGrid->RemoveView(kwSearch);
		searchGrid->AddView(txSearch, 1, 2);
		kwSearch->SetEnabled(false);
		txSearch->SetEnabled(true);
		Layout(true);
	}
}


void
StationFinderWindow::DoSearch(const char* text)
{
	resultView->MakeEmpty();
	if (!currentService)
		return;

	BObjectList<Station>* result
		= currentService->FindBy(ddSearchBy->Value(), text, this);
	if (result) {
		for (unsigned i = 0; i < result->CountItems(); i++)
			resultView->AddItem(new StationListViewItem(result->ItemAt(i)));
		result->MakeEmpty(false);
		delete result;
	}
}
