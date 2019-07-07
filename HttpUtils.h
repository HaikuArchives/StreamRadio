/* 
 * File:   HttpUtils.h
 * Author: user
 *
 * Created on 12. Oktober 2015, 23:14
 */

#ifndef HTTPUTILS_H
#define	HTTPUTILS_H

#include <DataIO.h>
#include <HttpRequest.h>
#include <Url.h>
#include <StringList.h>
#include <Socket.h>

class HttpRequest : public BHttpRequest {
public:
                            HttpRequest(const BUrl& url,
                                        bool ssl = false,
                                        const char* protocolName = "HTTP",
                                        BUrlProtocolListener* listener = NULL,
                                        BUrlContext* context = NULL) 
                              : BHttpRequest(url, ssl, protocolName, listener, context) { };
    virtual	BHttpResult&    Result() const {
                                return (BHttpResult&)BHttpRequest::Result();
                            };
};

class HttpUtils {
public:
	static status_t			CheckPort(BUrl url, BUrl* newUrl, uint32 flags = 0);
    static BMallocIO*       GetAll(BUrl url, BHttpHeaders* returnHeaders = NULL, bigtime_t timeOut=3000, BString* contentType = NULL, size_t sizeLimit = 0);
    static status_t         GetStreamHeader(BUrl url, BHttpHeaders* headers);
};



#endif	/* HTTPUTILS_H */

