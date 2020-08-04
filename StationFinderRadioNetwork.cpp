/*
 * File:   StationFinderRadioNetwork.cpp
 * Author: user, Jacob Secunda
 *
 * Created on 9. Oktober 2015, 22:51
 */

#include "StationFinderRadioNetwork.h"
#include "Debug.h"
#include "HttpUtils.h"
#include "StationFinderListenLive.h"
#include <Catalog.h>
#include <Country.h>

#include <Json.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StationFinderRadioNetwork"


const char* StationFinderRadioNetwork::kBaseUrl =
	"https://all.api.radio-browser.info/";

BString StationFinderRadioNetwork::sCachedServerUrl = B_EMPTY_STRING;


IconLookup::IconLookup(Station* station, BUrl iconUrl)
	:
	fStation(station),
	fIconUrl(iconUrl)
{
}


StationFinderRadioNetwork::StationFinderRadioNetwork()
	:
	StationFinderService(),
	fIconLookupThread(0),
	fIconLookupList()
{
	serviceName.SetTo(B_TRANSLATE("Community Radio Browser"));
	serviceHomePage.SetUrlString("https://www.radio-browser.info");

	RegisterSearchCapability("Name");
	RegisterSearchCapability("Tag");
	RegisterSearchCapability("Language");
	RegisterSearchCapability("Country");
	RegisterSearchCapability("Country code");
	RegisterSearchCapability("State/Region");
	RegisterSearchCapability("Unique identifier");
}


StationFinderRadioNetwork::~StationFinderRadioNetwork()
{
	fIconLookupList.MakeEmpty(true);
}


StationFinderService*
StationFinderRadioNetwork::Instantiate()
{
	return new StationFinderRadioNetwork();
}


void
StationFinderRadioNetwork::RegisterSelf()
{
	Register(
		"Community Radio Browser", &StationFinderRadioNetwork::Instantiate);
}


BObjectList<Station>*
StationFinderRadioNetwork::FindBy(
	int capabilityIndex, const char* searchFor, BLooper* resultUpdateTarget)
{
	if (fIconLookupThread)
		suspend_thread(fIconLookupThread);

	fIconLookupList.MakeEmpty(true);
	fIconLookupNotify = resultUpdateTarget;

	BObjectList<Station>* result = new BObjectList<Station>();
	if (result == NULL)
		return result;

	if (_CheckServer() != B_OK)
		return result;

	printf("Connected to server: %s \n", sCachedServerUrl.String());

	BString urlString(sCachedServerUrl);

	// Add the format and station section...
	urlString.Append("json/stations/");

	switch (capabilityIndex) {
		case 0: // Name search
			urlString.Append("byname/");
			break;
		case 1: // Tag search
			urlString.Append("bytag/");
			break;
		case 2: // Language search
			urlString.Append("bylanguage/");
			break;
		case 3: // Country search
			urlString.Append("bycountry/");
			break;
		case 4: // Country code search
			urlString.Append("bycountrycodeexact/");
			break;
		case 5: // State/Region search
			urlString.Append("bystate/");
			break;
		case 6: // Unique identifier search
			urlString.Append("byuuid/");
			break;
		default: // A very bad kind of search? Just do a name search...
			urlString.Append("byname/");
			break;
	}

	BString searchForString(searchFor);
	searchForString = BUrl::UrlEncode(searchForString, true, true);
	urlString.Append(searchForString);
	BUrl finalUrl(urlString);

	BMessage parsedData;
	BMallocIO* data = HttpUtils::GetAll(finalUrl);
	if (data != NULL
		&& BJson::Parse((const char*)data->Buffer(), data->BufferLength(),
			parsedData) == B_OK) {
		delete data;

		char* name;
		uint32 type;
		int32 count;
		for (int32 index = 0; parsedData.GetInfo(B_MESSAGE_TYPE, index,
			&name, &type, &count) == B_OK; index++) {
			BMessage stationMessage;
			if (parsedData.FindMessage(name, &stationMessage) == B_OK) {
				Station* station = new Station("unknown");
				if (station == NULL)
					continue;

				station->SetUniqueIdentifier(stationMessage.GetString(
					"stationuuid", B_EMPTY_STRING));

				station->SetName(stationMessage.GetString("name", "unknown"));

				station->SetSource(stationMessage.GetString("url",
					B_EMPTY_STRING));

				station->SetStation(stationMessage.GetString("homepage",
					B_EMPTY_STRING));

				BString iconUrl;
				if (stationMessage.FindString("favicon", &iconUrl) == B_OK) {
					fIconLookupList.AddItem(
						new IconLookup(station, BUrl(iconUrl)));
				}

				station->SetGenre(stationMessage.GetString("tags",
					B_EMPTY_STRING));

				BString countryCode;
				if (stationMessage.FindString("countrycode", &countryCode)
					== B_OK) {
					BCountry* country = new BCountry(countryCode);
					BString countryName;
					if (country != NULL
						&& country->GetName(countryName) == B_OK)
						station->SetCountry(countryName);

					delete country;
				}

				station->SetLanguage(stationMessage.GetString("language",
					B_EMPTY_STRING));

				station->SetBitRate(stationMessage.GetDouble("bitrate", 0)
					* 1000);

				// Set source URL as stream URL
				// If the source is a playlist, this setting will be
				// overridden when probing the station.
				// station->SetStreamUrl((const BUrl)station->Source());
				result->AddItem(station);
			}
		}

		if (!fIconLookupList.IsEmpty()) {
			if (!fIconLookupThread) {
				fIconLookupThread = spawn_thread(
					&IconLookupFunc, "iconlookup", B_LOW_PRIORITY, this);
			}
			resume_thread(fIconLookupThread);
		}
	} else {
		delete result;
		result = NULL;
	}

	return result;
}


int32
StationFinderRadioNetwork::IconLookupFunc(void* data)
{
	StationFinderRadioNetwork* _this = (StationFinderRadioNetwork*)data;
	while (!_this->fIconLookupList.IsEmpty()) {
		IconLookup* item = _this->fIconLookupList.FirstItem();
		BBitmap* logo = _this->logo(item->fIconUrl);
		if (logo)
			item->fStation->SetLogo(logo);
		BMessage* notification = new BMessage(MSG_UPDATE_STATION);
		notification->AddPointer("station", item->fStation);
		if (_this->fIconLookupNotify->LockLooper()) {
			_this->fIconLookupNotify->PostMessage(notification);
			_this->fIconLookupNotify->UnlockLooper();
		}
		_this->fIconLookupList.RemoveItem(item, true);
	}

	_this->fIconLookupThread = 0;
	return B_OK;
}


status_t
StationFinderRadioNetwork::_CheckServer()
{
	// Just a quick check up on our cached server...if it exists.
	BUrl cachedServerUrl(sCachedServerUrl);
	if (!sCachedServerUrl.IsEmpty()
		&& HttpUtils::CheckPort(cachedServerUrl, &cachedServerUrl, 0) == B_OK) {
		// It's still there!
		return B_OK;
	}

	// Try to find an active server!
	BUrl testServerUrl(kBaseUrl);
	status_t result = HttpUtils::CheckPort(testServerUrl, &testServerUrl, 0);
	if (result != B_OK) {
		// Oh no...this is, uh, pretty bad.
		return result;
	}

	// Cache it!
	sCachedServerUrl.SetTo(testServerUrl.UrlString());

	return B_OK;
}
