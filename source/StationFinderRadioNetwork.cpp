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

#include "Debug.h"
#include "HttpUtils.h"


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
	fIconLookupThread(-1),
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
	Register(
		"Community Radio Browser", &StationFinderRadioNetwork::Instantiate);
}


BObjectList<Station>*
StationFinderRadioNetwork::FindBy(int capabilityIndex, const char* searchFor,
	BLooper* resultUpdateTarget)
{
	_WaitForIconLookupThread();

	fIconLookupList.MakeEmpty(true);
	fIconLookupNotify = resultUpdateTarget;

	BObjectList<Station>* result = new BObjectList<Station>();
	if (result == NULL)
		return result;

	if (_CheckServer() != B_OK)
		return result;

	printf("Connected to server: %s\n", sCachedServerUrl.String());

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
					if (!iconUrl.IsEmpty()) {
						fIconLookupList.AddItem(
							new IconLookup(station, BUrl(iconUrl)));
					}
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
			fIconLookupThread = spawn_thread(
				&_IconLookupFunc, "iconlookup", B_LOW_PRIORITY, this);
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
	while (_this->fIconLookupThread >=0 && !_this->fIconLookupList.IsEmpty()) {
		IconLookup* item = _this->fIconLookupList.FirstItem();
		BBitmap* logo = _this->RetrieveLogo(item->fIconUrl);
		if (logo != NULL) {
			item->fStation->SetLogo(logo);

			BMessage* notification = new BMessage(MSG_UPDATE_STATION);
			notification->AddPointer("station", item->fStation);
			if (_this->fIconLookupThread >=0 && _this->fIconLookupNotify->LockLooper()) {
				_this->fIconLookupNotify->PostMessage(notification);
				_this->fIconLookupNotify->UnlockLooper();
			}
		}
		_this->fIconLookupList.RemoveItem(item, true);
	}

	_this->fIconLookupThread = -1;

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
