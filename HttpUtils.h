/*
 * Copyright (C) 2017 Kai Niessen <kai.niessen@online.de>
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
#ifndef _HTTP_UTILS_H
#define _HTTP_UTILS_H


#include <DataIO.h>
#include <HttpRequest.h>
#include <Socket.h>
#include <StringList.h>
#include <Url.h>


class HttpRequest : public BHttpRequest {
public:
	HttpRequest(const BUrl& url, bool ssl = false,
		const char* protocolName = "HTTP",
		BUrlProtocolListener* listener = NULL, BUrlContext* context = NULL)
		:
		BHttpRequest(url, ssl, protocolName, listener, context)
	{};


	virtual	BHttpResult&
	Result() const
	{
		return (BHttpResult&)BHttpRequest::Result();
	}
};


class HttpUtils {
public:
	static	status_t			CheckPort(BUrl url, BUrl* newUrl,
									uint32 flags = 0);

	static	BMallocIO*			GetAll(BUrl url,
									BHttpHeaders* returnHeaders = NULL,
									bigtime_t timeOut = 3000,
									BString* contentType = NULL,
									size_t sizeLimit = 0);

	static	status_t			GetStreamHeader(BUrl url,
									BHttpHeaders* headers);
};


#endif // _HTTP_UTILS_H
