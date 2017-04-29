/* 
 * File:   HttpUtils.cpp
 * Author: user
 * 
 * Created on 12. Oktober 2015, 23:14
 */

#include "HttpUtils.h"

class HttpIOReader : public BUrlProtocolListener {
public:
    HttpIOReader(BPositionIO* data, BHttpHeaders* responseHeaders, size_t limit) 
	  : fData(data),
	    fLimit(limit),
	    fHeaders(responseHeaders),
		fIsRedirect(false)
	{
    }
	  
    ~HttpIOReader() { }
	
	virtual void HeadersReceived(BUrlRequest* caller, const BUrlResult& result) {
		const BHttpResult* httpResult = dynamic_cast<const BHttpResult*>(&result);
		if (httpResult && fHeaders) {
			*fHeaders = httpResult->Headers();
			fIsRedirect = BHttpRequest::IsRedirectionStatusCode(httpResult->StatusCode());
		}
	}
    virtual void DataReceived(BUrlRequest* caller, const char* data, off_t position, ssize_t size) {
		if (fIsRedirect)
			return;
		
		if (fData == NULL) {
			caller->Stop();
			return;
		}

        fData->Write(data, size);
		if (fLimit) {
			if (fLimit < size) 
				caller->Stop();
			else
				fLimit -= size;
		}
    }
private:
    BPositionIO* fData;
	BHttpHeaders* fHeaders;
	bool		 fIsRedirect;
	
	size_t		 fLimit;
};

class HttpHeaderReader : public BUrlProtocolListener {
public:
                        HttpHeaderReader(char* buffer = NULL, ssize_t size = 0)
                          : BUrlProtocolListener(),
                            fBuffer(buffer),
                            fSize(size),
                            fPos(0)
                        { }  
                                  
    virtual void        HeadersReceived(BUrlRequest* caller) {
                            if (fBuffer == NULL && caller->IsRunning()) 
                                caller->Stop();
                        }
    virtual void        DataReceived(BUrlRequest* caller, const char* data, off_t position, ssize_t size) {
							if (fSize == 0 || fBuffer == NULL) {
								if (caller->IsRunning())
									caller->Stop();
								return;
							}

                            ssize_t receive = fSize - fPos;
                            if (size < receive) 
                                receive = size;
                            memcpy(fBuffer + fPos, data, receive);
                            fPos += receive;
                            if (fPos >= fSize && caller->IsRunning())
                                caller->Stop();
                        }
private:
    char*               fBuffer;
    ssize_t             fSize;
    off_t               fPos;
};

/**
 * Helper to make Http request and return body
 * @param url           Url to request
 * @param accept        Requested content type
 * @param contentType   Out: Received content type
 * @return              BMallocIO* filled with retrieved content
 */
BMallocIO* HttpUtils::GetAll(BUrl url, BHttpHeaders* responseHeaders, bigtime_t timeOut, BString* contentType, size_t sizeLimit) {
    BMallocIO* data = new BMallocIO();
    HttpIOReader reader(data, responseHeaders, sizeLimit);
    HttpRequest request(url, false, "HTTP", &reader);
	if (contentType && !contentType->IsEmpty()) {
		BHttpHeaders* requestHeaders = new BHttpHeaders();
		requestHeaders->AddHeader("accept", contentType->String());
		request.AdoptHeaders(requestHeaders);
	}
    request.SetAutoReferrer(true);
    request.SetFollowLocation(true);
    request.SetTimeout(timeOut);
    thread_id threadId = request.Run();
    status_t status = B_OK;
    wait_for_thread(threadId, &status);
    int32 statusCode = request.Result().StatusCode();
    size_t bufferLen = data->BufferLength();
    if (!(statusCode == 0 || request.IsSuccessStatusCode(statusCode)) || bufferLen == 0) {
        delete data;
        data = NULL;
    } else {
        if (contentType)
            contentType->SetTo(request.Result().ContentType());
    }
    return data;
}

status_t HttpUtils::GetStreamHeader(BUrl url, BHttpHeaders* responseHeaders) {
    status_t status;
    HttpIOReader reader(NULL, responseHeaders, 0);
    HttpRequest request(url, false, "HTTP", &reader);
	request.SetMethod("GET");
    request.SetAutoReferrer(true);
    request.SetFollowLocation(true);
    request.SetTimeout(10000);
    request.SetDiscardData(true);
    request.SetUserAgent("WinampMPEG/5.55, Ultravox/2.1");
    BHttpHeaders* requestHeaders = new BHttpHeaders();
    requestHeaders->AddHeader("Icy-MetaData", 1);
    request.AdoptHeaders(requestHeaders);
    thread_id threadId = request.Run();
    wait_for_thread(threadId, &status);
    
    if (status == B_OK) {
        BHttpResult result = request.Result();
        if (result.StatusCode() == 200 || result.StatusCode() == B_OK) {
            *responseHeaders = result.Headers();
        } else
            status = B_ERROR;
    }
    return status;
}

