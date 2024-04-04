/*
 * Copyright (C) 2017 Kai Niessen <kai.niessen@online.de>
 * Copyright (C) 2020 Jacob Secunda
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
#ifndef _STATION_H
#define _STATION_H


#include <Bitmap.h>
#include <Directory.h>
#include <File.h>
#include <Message.h>
#include <MimeType.h>
#include <String.h>
#include <SupportDefs.h>

#include "HttpUtils.h"


// Station flags
#define STATION_HAS_NAME 1
#define STATION_HAS_URI 2
#define STATION_URI_VALID 4
#define STATION_HAS_ENCODING 8
#define STATION_HAS_BITRATE 16
#define STATION_HAS_FORMAT 32  // frame rate and channels
#define STATION_HAS_META 64
#define STATION_HAS_IDENTIFIER 128


class StreamPlayer;


class Station {
public:
	Station(BString name, BString uri = B_EMPTY_STRING);
	Station(const Station& orig);
	virtual ~Station();

	status_t InitCheck();
	status_t Save();
	status_t RetrieveStreamUrl();
	status_t Probe();
	status_t ProbeBuffer(BPositionIO* buffer);

	static class Station* LoadFromPlsFile(BString name);
	static class Station* Load(BString name, BEntry* entry);
	static class Station* LoadIndirectUrl(BString& shoutCastUrl);

	static BDirectory* StationDirectory();

	inline BString* Name() { return &fName; }
	void SetName(BString name);

	inline BUrl StreamUrl() { return fStreamUrl; }
	inline void SetStreamUrl(BUrl uri)
	{
		fStreamUrl = uri;
		CheckFlags();
		fUnsaved = true;
	}

	inline BUrl StationUrl() { return fStationUrl; }
	inline void SetStation(BUrl url)
	{
		fStationUrl = url;
		CheckFlags();
		fUnsaved = true;
	}

	inline BUrl Source() { return fSource; }
	inline void SetSource(BUrl source)
	{
		fSource = source;
		CheckFlags();
		fUnsaved = true;
	}

	inline BBitmap* Logo() { return fLogo; }
	inline void SetLogo(BBitmap* logo)
	{
		delete fLogo;

		fLogo = logo;
		fUnsaved = true;
	}

	inline BString Genre() { return fGenre; }
	inline void SetGenre(BString genre)
	{
		fGenre.SetTo(genre);
		fUnsaved = true;
	}

	inline BString Country() { return fCountry; }
	inline void SetCountry(BString country)
	{
		fCountry.SetTo(country);
		fUnsaved = true;
	}

	inline BString Language() { return fLanguage; }
	inline void SetLanguage(BString language)
	{
		fLanguage.SetTo(language);
		fUnsaved = true;
	}

	inline int32 SampleRate() { return fSampleRate; }
	inline int32 BitRate() { return fBitRate; }
	inline void SetBitRate(int32 bitrate)
	{
		fBitRate = bitrate;
		fUnsaved = true;
	}

	inline BString UniqueIdentifier() { return fUniqueIdentifier; }
	inline void SetUniqueIdentifier(BString uniqueIdentifier)
	{
		fUniqueIdentifier.SetTo(uniqueIdentifier);
		fUnsaved = true;
	}

	inline int32 Channels() { return fChannels; }
	inline int32 Encoding() { return fEncoding; }
	inline size_t FrameSize() { return fFrameSize; }
	inline BMimeType* Mime() { return &fMime; }

	inline int32 Flags() { return fFlags; }
	inline bool Flags(uint32 flags) { return (fFlags & flags) == flags; }

protected:
	void CheckFlags();
	static char* RegFind(const char* text, const char* pattern);
	status_t ParseUrlReference(const char* body, const BUrl& baseUrl);
	void CleanName();

	BString fName;
	BUrl fStreamUrl;
	BUrl fStationUrl;
	BString fGenre;
	BString fCountry;
	BString fLanguage;
	BUrl fSource;
	BMimeType fMime;
	uint32 fEncoding;
	BBitmap* fLogo;
	uint32 fRating;
	uint32 fBitRate;
	uint32 fSampleRate;
	BString fUniqueIdentifier;
	uint32 fMetaInterval;
	uint32 fChannels;
	uint32 fFlags;
	size_t fFrameSize;

	static BDirectory* sStationsDirectory;

private:
	bool fUnsaved;
};


#endif	// _STATION_H
