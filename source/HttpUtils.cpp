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

#include "Debug.h"
#include "HttpUtils.h"
#include "Utils.h"
#include "override.h"


class DataLimit : public BUrlProtocolListener, public BDataIO {
public:
	DataLimit(BDataIO* sink, size_t limit) : fSink(sink), fLimit(limit) {}

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


/**
 * Loops on the DNS responses looking for connectivity on specified port.
 */
status_t
HttpUtils::CheckPort(BUrl url, BUrl* newUrl, uint32 flags)
{
	uint16 port;
	if (url.HasPort())
		port = url.Port();
	else
		port = 80;

	BReference<const BNetworkAddressResolver> resolver =
		BNetworkAddressResolver::Resolve(url.Host(), port, flags);
	if (resolver.Get() == NULL)
		return B_NO_MEMORY;
	status_t status = resolver->InitCheck();
	if (status != B_OK)
		return status;

	BString newUrlString;
	BNetworkAddress ipAddress;
	BSocket* socket;
	uint32 cookie = 0;

	status_t portStatus = B_ERROR;
	while (portStatus != B_OK) {
		status = resolver->GetNextAddress(AF_INET6, &cookie, ipAddress);
		if (status != B_OK)
			status = resolver->GetNextAddress(&cookie, ipAddress);
		if (status != B_OK)
			return status;
		socket = new (std::nothrow) BSocket();
		portStatus = socket->Connect(ipAddress);
		delete socket;
	}

	// If port number is 80, do not add it to the final URL
	// Then, prepend the appropiate protocol
	newUrlString =
		ipAddress.ToString(ipAddress.Port() != 80).Prepend("://").Prepend(url.Protocol());
	if (url.HasPath())
		newUrlString.Append(url.Path());
	newUrl->SetUrlString(newUrlString.String());

	return B_OK;
}


/**
 * Helper to make Http request and return body
 * @param url           Url to request
 * @param accept        Requested content type
 * @param contentType   Out: Received content type
 * @return              BMallocIO* filled with retrieved content
 */
BMallocIO*
HttpUtils::GetAll(BUrl url, BHttpHeaders* responseHeaders, bigtime_t timeOut, BString* contentType,
	size_t sizeLimit)
{
	BMallocIO* data = new BMallocIO();
	if (data == NULL)
		return data;

	DataLimit reader(data, sizeLimit);
	BHttpRequest* request;
	if (sizeLimit)
		request = dynamic_cast<BHttpRequest*>(
			BUrlProtocolRoster::MakeRequest(url.UrlString().String(), &reader, &reader, NULL));
	else
		request = dynamic_cast<BHttpRequest*>(
			BUrlProtocolRoster::MakeRequest(url.UrlString().String(), data, NULL, NULL));

	if (request == NULL)
		return NULL;

	if (contentType && !contentType->IsEmpty()) {
		BHttpHeaders* requestHeaders = new BHttpHeaders();
		requestHeaders->AddHeader("accept", contentType->String());
		request->AdoptHeaders(requestHeaders);
	}

	request->SetAutoReferrer(true);
	request->SetFollowLocation(true);
	request->SetTimeout(timeOut);
	request->SetUserAgent(Utils::UserAgent());

	thread_id threadId = request->Run();
	status_t status;
	wait_for_thread(threadId, &status);

	BHttpResult& result = (BHttpResult&)request->Result();
	int32 statusCode = result.StatusCode();
	size_t bufferLen = data->BufferLength();
	if (!(statusCode == 0 || request->IsSuccessStatusCode(statusCode)) || bufferLen == 0) {
		delete data;
		data = NULL;
	} else if (contentType != NULL)
		contentType->SetTo(result.ContentType());

	if (responseHeaders != NULL)
		*responseHeaders = result.Headers();

	delete request;
	return data;
}
