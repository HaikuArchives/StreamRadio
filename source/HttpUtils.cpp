/*
 * Copyright (C) 2017 Kai Niessen <kai.niessen@online.de>
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


#include <DataIO.h>
#include <NetworkAddressResolver.h>
#include <private/shared/Json.h>
#include <Roster.h>
#include <AppFileInfo.h>
#include <Application.h>

#include "Debug.h"
#include "HttpUtils.h"
#include "Utils.h"
#include "override.h"


class DataLimit : public BUrlProtocolListener, public BDataIO {
public:
	DataLimit(BDataIO* sink, size_t limit)
		: fSink(sink),
		  fLimit(limit)
	{
	}

	ssize_t Write(const void* buffer, size_t size) override
	{
		if (size > fLimit)
			size = fLimit;

		ssize_t written = fSink->Write(buffer, size);
		if (written > 0)
			fLimit -= written;

		return written;
	}

	void BytesWritten(BUrlRequest* caller, size_t size) override
	{
		if (fLimit == 0)
			caller->Stop();
	}

private:
	BDataIO* fSink;
	size_t fLimit;
};


BString
HttpUtils::jsonType("application/json");


/**
 * Helper to make Http request and return body
 * @param url           Url to request
 * @param accept        Requested content type
 * @param contentType   Out: Received content type
 * @return              BMallocIO* filled with retrieved content
 */
BMallocIO*
HttpUtils::GetAll(BUrl* url, BHttpHeaders* responseHeaders, bigtime_t timeOut, BString* contentType,
	size_t sizeLimit)
{
	BMallocIO* data = new BMallocIO();
	if (data == NULL)
		return data;

	DataLimit reader(data, sizeLimit);
	BHttpRequest* request;
	if (sizeLimit)
		request = dynamic_cast<BHttpRequest*>(
			BUrlProtocolRoster::MakeRequest(url->UrlString().String(), &reader, &reader, NULL));
	else
		request = dynamic_cast<BHttpRequest*>(
			BUrlProtocolRoster::MakeRequest(url->UrlString().String(), data, NULL, NULL));

	if (request == NULL) {
		delete data;
		return NULL;
	}

	if (contentType && !contentType->IsEmpty()) {
		BHttpHeaders requestHeaders;
		requestHeaders.AddHeader("accept", contentType->String());
		request->SetHeaders(requestHeaders);
	}

	request->SetAutoReferrer(true);
	request->SetFollowLocation(true);
	request->SetTimeout(timeOut);
	request->SetUserAgent(UserAgent());

	thread_id threadId = request->Run();
	status_t status;
	wait_for_thread(threadId, &status);

	const BHttpResult& result	= dynamic_cast<const BHttpResult&>(request->Result());
	int32 statusCode			= result.StatusCode();
	size_t bufferLen			= data->BufferLength();
	if (!(statusCode == 0 || request->IsSuccessStatusCode(statusCode)) || bufferLen == 0) {
		delete data;
		data = NULL;
	} else {
		data->Seek(0, SEEK_SET);
		if (contentType != NULL)
			contentType->SetTo(result.ContentType());
	}

	if (responseHeaders != NULL)
		*responseHeaders = result.Headers();

	delete request;
	return data;
}


/**
 * Helper class for scanning simple JSONs
 */
class JsonPathExtractor : public BJsonEventListener {
public:

	/**
	* Listens to JSON parsing events and extracts specific path
	* @param path          Dot-separated location of element in JSON hierarchy
	*					   [].{}.address.{}.street will get street within an
	*					   address object within the unnamed objects of the array 
	* @param result        Initialized BStringList to capture all occurrences
	*/	
	JsonPathExtractor(BString* path, BStringList* result) : 
			BJsonEventListener(),
			pathElements(true) 
	{
		path->Split(".", true, pathElements);
		this->result = result;
	}
	
	~JsonPathExtractor() {
		pathElements.MakeEmpty();
	}
	
	virtual bool Handle(const BJsonEvent& event) {
		switch (event.EventType()) {
			case B_JSON_NUMBER :
		    case B_JSON_TRUE :
			case B_JSON_FALSE :
			case B_JSON_NULL	 :
				if (awaitValue) 
					awaitValue = false;
				if (offIndex > 0)
					offIndex--;
				else
					index--;
				break;
				
			case B_JSON_STRING :
				if (awaitValue) {
					result->Add(event.Content());
					awaitValue = false;
				} 
				if (offIndex > 0)
					offIndex--;
				else 
					index--;
				break;
			
			case B_JSON_OBJECT_START :
				if (index < pathElements.CountStrings() && 
					pathElements.StringAt(index) == "{}" &&
					offIndex == 0)
					index++;
				else 
					offIndex++;
				break;
				
			case B_JSON_OBJECT_END :
				if (offIndex > 0)
					offIndex--;
				else 
					index--;
				break;
				
			case B_JSON_OBJECT_NAME :
				if (index < pathElements.CountStrings() && 
					pathElements.StringAt(index) == event.Content() &&
					offIndex == 0)
				{
					index++;
					if (index == pathElements.CountStrings())
						awaitValue = true;
				}
				else 
					offIndex++;
				break;
				
			case B_JSON_ARRAY_START :
				if (index < pathElements.CountStrings() && 
					pathElements.StringAt(index) == "[]" &&
					offIndex == 0)
					index++;
				else 
					offIndex++;
				break;
				
			case B_JSON_ARRAY_END :
				if (offIndex > 0)
					offIndex--;
				else {
					index--;
				}
				break;
		}
		return true;
	}
	
	virtual void HandleError(status_t status, int32 line, const char* message) { 
		printf("JsonParser line %d: %s", line, message);
	}
	virtual void Complete() { }
	
private:
	int index 					= 0;
	int offIndex 				= 0;
	bool awaitValue				= false;
	BString name;
	BStringList* result 		= NULL;
	
	// path like "[].{}.name" extracts all elements called "name" in root array of unnamed objects
	BStringList pathElements;		
};

/**
 * Helper to make retrieve list of occurrences of specific element in JSON
 * @param url           REST-Url to request
 * @param path          Dot-separated location of element in JSON hierarchy
 * 						[].{}.address.{}.street will get street within an
 *						address object within the unnamed objects of the array 
 * @param contentType   In/Out: Received headers
 * @param timeout		Timeout of request in ms, default set to 3 seconds
 * @return              BStringList* filled with all occurrences of element described by path
 */
BMessage* 
HttpUtils::GetMsgFromREST(BUrl* url, BHttpHeaders* responseHeaders, bigtime_t timeOut) {
	BMallocIO* json = GetAll(url, responseHeaders, timeOut, &jsonType, 0);
	if (json == NULL)
		return NULL;
		
	BMessage* data = new BMessage();
	
	status_t status = BJson::Parse((const char*)json->Buffer(), json->BufferLength(), *data);
	delete json;
	
	if (status != B_OK) {
		delete data;
		data = NULL;
	}

	return data;
} 

/**
 * Helper to make retrieve list of occurrences of specific element in JSON
 * @param url            	REST-Url to request
 * @param path           	Dot-separated location of element in JSON hierarchy
 * 							[].{}.address.{}.street will get street within an
 *							address object within the unnamed objects of the root array 
 * @param responseHeaders	Unless NULL, populated with HTTP headers of response
 * @param contentType   	In/Out: Received headers
 * @param timeout			Timeout of request in ms, default set to 3 seconds
 * @return              	BStringList* filled with all occurrences of element described by path
 */
BStringList* 
HttpUtils::GetStringsFromREST(BUrl* url, BString* path, BHttpHeaders* responseHeaders, 
		bigtime_t timeout) {
	BMallocIO* json = GetAll(url, responseHeaders, timeout, &jsonType, 0);
	if (json == NULL)
		return NULL;
		
	if (json->BufferLength() == 0) {
		delete json;
		return NULL;
	}
		
	BStringList* result = new BStringList();
	JsonPathExtractor extractor(path, result);
	BJson::Parse(json, &extractor);
	delete json;
	
	if (result->IsEmpty()) {
		delete result;
		result = NULL;
	}

	return result;
}

char HttpUtils::sUserAgent[50] = "\0";

const char*
HttpUtils::UserAgent()
{
	if (sUserAgent[0] != 0)
		return sUserAgent;

	app_info appInfo;
	be_app->GetAppInfo(&appInfo);
	BFile file(&appInfo.ref, B_READ_ONLY);
	if (file.InitCheck() == B_OK) {
		BAppFileInfo appFileInfo(&file);
		struct version_info version;
		if (appFileInfo.GetVersionInfo(&version, B_APP_VERSION_KIND) == B_OK)
			snprintf(sUserAgent, sizeof(sUserAgent),
				"StreamRadio/%" B_PRIu32 ".%" B_PRIu32 ".%" B_PRIu32, version.major, version.middle,
				version.minor);
	}

	if (sUserAgent[0] == 0) {
		// There was an error trying to get the version info
		sprintf(sUserAgent, "StreamRadio");
	}

	return sUserAgent;
}





