/* 
 * File:   Station.h
 * Author: Kai Niessen
 *
 * Created on 26. Februar 2013, 04:08
 */

#ifndef STATION_H
#define	STATION_H
#include <Message.h>
#include <Bitmap.h>
#include <File.h>
#include <support/String.h>
#include <support/SupportDefs.h>
#include <MimeType.h>
#include <Directory.h>
#include "HttpUtils.h"


// Station flags
#define STATION_HAS_NAME            1    
#define STATION_HAS_URI             2
#define STATION_URI_VALID           4
#define STATION_HAS_ENCODING        8
#define STATION_HAS_BITRATE         16
#define STATION_HAS_FORMAT          32 // frame rate and channels
#define STATION_HAS_META            64 

class StreamPlayer;

class Station {
public:
                                    Station(BString Name, BString Uri = B_EMPTY_STRING);
                                    Station(const Station& orig);
    virtual                         ~Station();
    status_t                        InitCheck();
    status_t                        Save();
    status_t                        RetrieveStreamUrl();
    status_t                        Probe();
    status_t                        ProbeBuffer(BPositionIO* buffer);

    static class Station*           LoadFromPlsFile(BString Name); 
    static class Station*           Load(BString Name, BEntry* entry);
    static class Station*           LoadIndirectUrl(BString& ShoutCastUrl);
    static BDirectory*              StationDirectory();

    inline BString*                 Name()                  { return &fName; }
    void                            SetName(BString name);
    inline BUrl                     StreamUrl()             { return fStreamUrl; }
    inline void                     SetStreamUrl(BUrl uri)  { fStreamUrl = uri; checkFlags(); unsaved = true; }
    inline BUrl                     StationUrl()            { return fStationUrl; }
    inline void                     SetStation(BUrl sUrl)   { fStationUrl = sUrl; checkFlags(); unsaved = true; }
    inline BUrl                     Source()                { return fSource; }
    inline void                     SetSource(BUrl source)  { fSource = source; checkFlags(); unsaved = true; }
    inline BBitmap*                 Logo()                  { return fLogo; }
    void                            SetLogo(BBitmap* logo)  { if (fLogo) delete fLogo; fLogo = logo; unsaved = true; }
    inline BString                  Genre()                 { return fGenre; }
    inline void                     SetGenre(BString genre) { fGenre.SetTo(genre); unsaved = true; }
    inline BString                  Country()               { return fCountry; }
    inline void                     SetCountry(BString country) { fCountry.SetTo(country); unsaved = true; }
    inline BString                  Language()               { return fLanguage; }
    inline void                     SetLanguage(BString language) { fLanguage.SetTo(language); unsaved = true; }
    inline int32                    SampleRate()            { return fSampleRate; }
    inline int32                    BitRate()               { return fBitRate; }
	void							SetBitRate(int32 bitrate) { fBitRate = bitrate; unsaved = true; }
    inline int32                    Channels()              { return fChannels; }
    inline int32                    Encoding()              { return fEncoding; }
    inline size_t                   FrameSize()             { return fFrameSize; }
    inline BMimeType*               Mime()                  { return &fMime; }
    inline int32                    Flags()                 { return fFlags; }
    inline bool                     Flags(int32 flags)      { return (fFlags & flags) == flags; }
    
protected:
    void                            checkFlags();
    static char*                    regFind(const char* Text, const char* Pattern);
    status_t                        parseUrlReference(const char* body, const char* mime);
    void                            cleanName();
    bool                            unsaved;
    BString                         fName;
    BUrl                            fStreamUrl;
    BUrl                            fStationUrl;
    BBitmap*                        fLogo;
    uint32                          fRating;
    BString                         fGenre;
	BString							fCountry;
	BString							fLanguage;
    BUrl                            fSource;
    uint32                          fBitRate;
    uint32                          fSampleRate;
    uint32                          fChannels;
    size_t                          fFrameSize;
    BMimeType                       fMime;
    uint32                          fEncoding;
    uint32                          fMetaInterval;
    uint32                          fFlags;
};
#endif	/* STATION_H */

