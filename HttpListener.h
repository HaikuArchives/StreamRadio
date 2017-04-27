/* 
 * File:   HttpListener.h
 * Author: user
 *
 * Created on December 8, 2014, 9:21 PM
 */

#ifndef HTTPLISTENER_H
#define	HTTPLISTENER_H

#include <String.h>
#include <UrlProtocolListener.h>

class HttpListener : public BUrlProtocolListener {
public:
    BString data;
    HttpListener();
    HttpListener(const HttpListener& orig);
    virtual void DataReceived(BUrlRequest* caller,
            const char* data, off_t position,
            ssize_t size);    
    virtual void HeadersReceived(BUrlRequest* caller);
    virtual ~HttpListener();
private:
};

#endif	/* HTTPLISTENER_H */

