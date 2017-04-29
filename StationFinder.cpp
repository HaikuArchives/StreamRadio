#include "StationFinder.h"
#include <LayoutBuilder.h>
#include <StringView.h>
#include <ScrollView.h>
#include <TranslationUtils.h>
#include <View.h>
#include <Alert.h>
#include "HttpUtils.h"
#include "RadioApp.h"
#include "Utils.h"

StationFinderServices::~StationFinderServices() {
	fServices.clear();
}

void
StationFinderServices::Add(char* name, InstantiateFunc instantiate) {
	pair<char*, InstantiateFunc> service(name, instantiate);
	fServices.push_back(service);
}

int
StationFinderServices::CountItems() {
	return fServices.size();
}

StationFinderService*
StationFinderServices::Instantiate(char* name) {
	for (int i = 0; i < fServices.size(); i++) {
		pair<char*, InstantiateFunc> service = fServices[i];
		if (!strcmp(service.first, name))
			return service.second();
	}
	return NULL;
}

char*
StationFinderServices::Name(int i) {
	return fServices[i].first;
}



const char*
StationFinderService::FindByCapabilityName[] = {
	"Name",
	"Country",
	"Genre",
	"Keyword"
};

StationFinderWindow::StationFinderWindow(BWindow* parent) 
  : BWindow(BRect(0,0,300,150), "Find Stations", B_TITLED_WINDOW, B_CLOSE_ON_ESCAPE),
	currentService(NULL)
{
    messenger   = new BMessenger(parent);
    
    CenterIn(parent->Frame());
	
    txSearch    = new BTextControl("Search", NULL, new BMessage(MSG_TXSEARCH));
	bnSearch    = new BButton("bnSearch", NULL, new BMessage(MSG_BNSEARCH));
	bnSearch->SetIcon(Utils::ResourceBitmap(RES_BN_SEARCH));
	
	
    ddServices  = new BOptionPopUp("ddServices", "", new BMessage(MSG_SELECT_SERVICE));
	int currentServiceIndex = 0;
	const char* settingsServiceName = ((RadioApp*)be_app)->Settings.StationFinderName();
	for (int i = 0; i < StationFinderService::stationFinderServices.CountItems(); i++) {
		const char* serviceName = StationFinderService::stationFinderServices.Name(i);
		if (settingsServiceName && !strcmp(serviceName, settingsServiceName)) {
			currentServiceIndex = i;
		}
        ddServices->AddOption(serviceName, i);
    }
    bnVisit		= new BButton("bnVisit", "", new BMessage(MSG_VISIT_SERVICE));
	bnVisit->SetIcon(Utils::ResourceBitmap(RES_BN_WEB));

	ddSearchBy  = new BOptionPopUp("ddSearchBy", NULL, new BMessage(MSG_SEARCH_BY));
    resultView  = new StationListView();
    resultView->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT));
    bnAdd       = new BButton("bnAdd", "Add", new BMessage(MSG_ADD_STATION));
    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .AddGrid(3, 3, 0.0)
            .SetInsets(B_USE_WINDOW_INSETS)
            .Add(new BStringView("lbServices", "Service"), 0, 0)
            .Add(ddServices, 1, 0)
		    .Add(bnVisit, 2, 0)
			
			.Add(new BStringView("lbSearchBy", "Search by"), 0, 1)
			.Add(ddSearchBy, 1, 1)
			
			.AddTextControl(txSearch, 0, 2)
            .Add(bnSearch, 2, 2) 
        .End()
	
        .Add(new BScrollView("srollResult", resultView, 0, B_FOLLOW_ALL_SIDES, false, true), 0.9)
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

StationFinderWindow::~StationFinderWindow() {
	if (currentService)
		delete currentService;
    resultView->MakeEmpty();
	txSearch->StopWatchingAll(this);
    delete messenger;
}

bool StationFinderWindow::QuitRequested() {
    messenger->SendMessage(new BMessage(MSG_SEARCH_CLOSE));
    return true;
}

void StationFinderWindow::MessageReceived(BMessage* msg) {
    switch (msg->what) {
        case MSG_BNSEARCH : {
                if (txSearch->Text()[0]) {
                    DoSearch(txSearch->Text());
                    resultView->MakeFocus(true);
                }
            }
            break;
                
        case MSG_ADD_STATION : {
            int32 index = msg->GetInt32("index", -1);
            if (index < 0) index = resultView->CurrentSelection(0);
            if (index >= 0) {
                Station* station = resultView->StationAt(index);
				status_t probeStatus = station->Probe();
                if (probeStatus == B_OK) {
                    BMessage* dispatch = new BMessage(msg->what);
                    dispatch->AddPointer("station", station);
                    if (messenger->SendMessage(dispatch) == B_OK) {
                        resultView->RemoveItem(index);
                    }
                } else {
					BString msg;
					msg.SetToFormat("Station %s did not respond correctly and could not be added", 
							station->Name()->String());
					(new BAlert("Add Station Failed", msg, "Ok"))->Go();
				}
            }
            break;
        }
	
		case MSG_UPDATE_STATION : {
			Station* st = NULL;
			status_t status = msg->FindPointer("station", (void**)&st);
			if (status == B_OK) {
				resultView->InvalidateItem(resultView->IndexOf(resultView->Item(st)));
			}
			break;
		}
        
		case MSG_SELECT_STATION : {
			bool inResults = (msg->GetInt32("index", -1) >= 0);
			bnAdd->SetEnabled(inResults);
			bnAdd->MakeDefault(inResults);
			break;
		}
		
		case MSG_SELECT_SERVICE :
			SelectService(ddServices->Value());
			break;
			
		case MSG_VISIT_SERVICE : {
			if (currentService && currentService->serviceHomePage && 
					currentService->serviceHomePage.IsValid()) {
				currentService->serviceHomePage.OpenWithPreferredApplication(true);
			}
			break;
		}
		case B_OBSERVER_NOTICE_CHANGE : 
			bnSearch->MakeDefault(true);
			break;
			
		case MSG_TXSEARCH : 
			bnAdd->MakeDefault(false);
			bnSearch->MakeDefault(true);
			break;
    
        default:
            BWindow::MessageReceived(msg);
    }
}

void StationFinderWindow::SelectService(int index) {
	if (currentService) {
		delete currentService;
		currentService = NULL;
	}

	char* serviceName = StationFinderServices::Name(index);
	if (!serviceName) 
		return;
	
	currentService = StationFinderServices::Instantiate(serviceName);
	((RadioApp*)be_app)->Settings.SetStationFinderName(serviceName);
	while (this->ddSearchBy->CountOptions())
		ddSearchBy->RemoveOptionAt(0);
		
	set<int> caps = currentService->Capabilities();
	for (set<int>::const_iterator i = caps.begin(); 
			i != caps.end(); i++) {
		ddSearchBy->AddOption(StationFinderService::FindByCapabilityName[*i], *i);
	}
}

void StationFinderWindow::DoSearch(const char* text) {
    resultView->MakeEmpty();
	if (!currentService) 
		return;
    
    BObjectList<Station>* result = currentService->FindBy((StationFinderService::FindByCapability)ddSearchBy->Value(), text, this);
    if (result) {
        for (unsigned i = 0; i < result->CountItems(); i++) {
            resultView->AddItem(new StationListViewItem(result->ItemAt(i)));
        }
        result->MakeEmpty(false);
        delete result;
    }
}

StationFinderService::StationFinderService() 
  : serviceName("unknown"),
    serviceHomePage(""),
    serviceLogo(NULL)
{ }


vector<pair<char*, InstantiateFunc> >
StationFinderServices::fServices;

void 
StationFinderService::Register(char* name, InstantiateFunc instantiate) {
	stationFinderServices.Add(name, instantiate);
}

BBitmap* 
StationFinderService::logo(BUrl url) {
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