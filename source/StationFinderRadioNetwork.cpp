/*
 * Copyright (C) 2017 Kai Niessen <kai.niessen@online.de>
 * Copyright (C) 2020 Jacob Secunda
 * Copyright 2023 Haiku, Inc. All rights reserved.
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


#include "StationFinderRadioNetwork.h"

#include <Catalog.h>
#include <Country.h>
#include <Json.h>
#include <NetworkAddressResolver.h>



#include "HttpUtils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StationFinderRadioNetwork"

BString StationFinderRadioNetwork::sBaseUrl("https://de1.api.radio-browser.info/");

const char* StationFinderRadioNetwork::serviceNameInternal = "Community Radio Browser";


IconLookup::IconLookup(Station* station, BUrl iconUrl)
	: fStation(station),
	  fIconUrl(iconUrl)
{
}


BStringList*
StationFinderRadioNetwork::_GetKeywords(const char* path, int32 minStations) {
	BString keywordUrlString(sBaseUrl);
	keywordUrlString.Append(path);
	BUrl keywordUrl(keywordUrlString, false);

	BMessage* parsedData = HttpUtils::GetMsgFromREST(&keywordUrl);
	if (parsedData != NULL) {
		BStringList* keywords = new BStringList(true);
		char* name;
		uint32 type;
		int32 count;
		for (int32 index = 0; parsedData->GetInfo(B_MESSAGE_TYPE, index, &name, &type, &count) == B_OK; index++) {
			BMessage keywordMessage;
			if (parsedData->FindMessage(name, &keywordMessage) == B_OK) {
				const char* keyword = keywordMessage.FindString("name");
				double stationCount = keywordMessage.FindDouble("stationcount");
				if (stationCount > minStations)
					keywords->Add(keyword);
			}
		}
		delete parsedData;
		return keywords;
	}
	return NULL;
};

StationFinderRadioNetwork::StationFinderRadioNetwork()
	: StationFinderService(),
	  fIconLookupThread(-1),
#if B_HAIKU_VERSION > B_HAIKU_VERSION_1_BETA_5
	  fIconLookupList(100)
#else
	  fIconLookupList(100, true)
#endif
{
	serviceName.SetTo(B_TRANSLATE(serviceNameInternal));
	serviceHomePage.SetUrlString("https://www.radio-browser.info", false);

	BStringList* keywords;

	// Register different search capabilities
	searchCapabilityName = RegisterSearchCapability("Name");
	
	// Tag Search (dropdown)
	keywords = _GetKeywords("json/tags?limit=1000000&hidebroken=true", 20);
	if (keywords != NULL) {
		searchCapabilityTag = RegisterSearchCapability("Tag", keywords);
		delete keywords;
	}
	
	// Language Search (dropdown)
	keywords = _GetKeywords("json/languages?limit=1000000&hidebroken=true", 20);
	if (keywords != NULL) {
		searchCapabilityLanguage = RegisterSearchCapability("Language", keywords);
		delete keywords;
	}
	
	keywords = _GetKeywords("json/countries?limit=1000000&hidebroken=true", 10);
	if (keywords != NULL) {
		searchCapabilityCountry = RegisterSearchCapability("Country", keywords);
		delete keywords;
	}
	
	keywords = _GetKeywords("json/codecs?limit=1000000&hidebroken=true", 10);
	if (keywords != NULL && !keywords->IsEmpty()) {
		searchCapabilityCodec = RegisterSearchCapability("Codec", keywords);
		delete keywords;
	}
	
	searchCapabilityUuid = RegisterSearchCapability("Unique identifier");
}


StationFinderRadioNetwork::~StationFinderRadioNetwork()
{
	_WaitForIconLookupThread();
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
	Register(new BString(B_TRANSLATE(serviceNameInternal)), &StationFinderRadioNetwork::Instantiate);
}


StationList*
StationFinderRadioNetwork::FindBy(
	int capabilityIndex, const char* searchFor, BLooper* resultUpdateTarget)
{
	_WaitForIconLookupThread();

	fIconLookupList.MakeEmpty(true);
	fIconLookupNotify = resultUpdateTarget;

	StationList* result = new StationList();
	if (result == NULL)
		return result;
	
	BString urlString(sBaseUrl);

	// Add the format and station section...
	urlString.Append("json/stations/");

	if (capabilityIndex == searchCapabilityName)  // Name search
		urlString.Append("byname/");
	else if (capabilityIndex == searchCapabilityTag) // Tag search
		urlString.Append("bytag/");
	else if (capabilityIndex == searchCapabilityLanguage) // Language search
		urlString.Append("bylanguage/");
	else if (capabilityIndex == searchCapabilityCountry) // Country search
		urlString.Append("bycountry/");
	else if (capabilityIndex == searchCapabilityCodec) // Codec search
		urlString.Append("bycodec/");
	else if (capabilityIndex == searchCapabilityUuid) // Unique identifier search
		urlString.Append("byuuid/");
	else  // A very bad kind of search? Just do a name search...
		urlString.Append("byname/");

	BString searchForString(searchFor);
	searchForString = BUrl::UrlEncode(searchForString, true, true);
	urlString.Append(searchForString);
	urlString.Append("?limit=2000000&hidebroken=true");
	BUrl finalUrl(urlString, true);

	BMessage* parsedData = HttpUtils::GetMsgFromREST(&finalUrl, NULL, 5000);
	if (parsedData != NULL) {
		char* name;
		uint32 type;
		int32 count;
		for (int32 index = 0;
			parsedData->GetInfo(B_MESSAGE_TYPE, index, &name, &type, &count) == B_OK; index++) {
			BMessage stationMessage;
			if (parsedData->FindMessage(name, &stationMessage) == B_OK) {
				Station* station = new Station("unknown");
				if (station == NULL)
					continue;

				station->SetUniqueIdentifier(
					stationMessage.GetString("stationuuid", B_EMPTY_STRING));

				station->SetName(stationMessage.GetString("name", "unknown"));

				station->SetSource(stationMessage.GetString("url", B_EMPTY_STRING));

				station->SetStation(stationMessage.GetString("homepage", B_EMPTY_STRING));

				BString iconUrl;

				if (stationMessage.FindString("favicon", &iconUrl) == B_OK && !iconUrl.IsEmpty())
					fIconLookupList.AddItem(new IconLookup(station, BUrl(iconUrl, false)));


				station->SetGenre(stationMessage.GetString("tags", B_EMPTY_STRING));

				BString countryCode;
				if (stationMessage.FindString("countrycode", &countryCode) == B_OK) {
					BCountry* country = new BCountry(countryCode);
					BString countryName;
					if (country != NULL && country->GetName(countryName) == B_OK)
						station->SetCountry(countryName);

					delete country;
				}

				station->SetLanguage(stationMessage.GetString("language", B_EMPTY_STRING));

				station->SetBitRate(stationMessage.GetDouble("bitrate", 0) * 1000);

				// Set source URL as stream URL
				// If the source is a playlist, this setting will be
				// overridden when probing the station.
				// station->SetStreamUrl((const BUrl)station->Source());
				result->AddItem(station);
			}
		}
		delete parsedData;

		if (!fIconLookupList.IsEmpty()) {
			fIconLookupThread = spawn_thread(&_IconLookupFunc, "iconlookup", B_LOW_PRIORITY, this);
			resume_thread(fIconLookupThread);
		}
	} else {
		delete result;
		result = NULL;
	}

	return result;
}


int32
StationFinderRadioNetwork::_IconLookupFunc(void* data)
{
	StationFinderRadioNetwork* _this = (StationFinderRadioNetwork*)data;
	while (_this->fIconLookupThread >= 0 && !_this->fIconLookupList.IsEmpty()) {
		IconLookup* item = _this->fIconLookupList.FirstItem();
		BBitmap* logo = _this->RetrieveLogo(&item->fIconUrl);
		if (logo != NULL && logo->IsValid()) {
			item->fStation->SetLogo(logo);

			BMessage* notification = new BMessage(MSG_UPDATE_STATION);
			notification->AddPointer("station", item->fStation);
			if (_this->fIconLookupThread >= 0 && _this->fIconLookupNotify->LockLooper()) {
				_this->fIconLookupNotify->PostMessage(notification);
				_this->fIconLookupNotify->UnlockLooper();
			}
		}
		_this->fIconLookupList.RemoveItem(item, true);
	}

	_this->fIconLookupThread = -1;

	return B_OK;
}


void
StationFinderRadioNetwork::_WaitForIconLookupThread()
{
	if (fIconLookupThread >= 0) {
		status_t status;
		thread_id tid = fIconLookupThread;
		fIconLookupThread = -1;
		wait_for_thread(tid, &status);
	}
}
