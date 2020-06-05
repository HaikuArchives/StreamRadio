/*
 * File:   Station.cpp
 * Author: Kai Niessen
 *
 * Created on 26. Februar 2013, 04:08
 */
#include "Station.h"
#include "Debug.h"
#include "HttpUtils.h"
#include <Alert.h>
#include <BitmapStream.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <HttpRequest.h>
#include <HttpResult.h>
#include <MediaFile.h>
#include <MediaTrack.h>
#include <Message.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <Path.h>
#include <StringList.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <UrlSynchronousRequest.h>
#include <exception>
#include <fs_attr.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Station"

const char* SubDirStations = "Stations";
const char* MimePls = "audio/x-scpls";


Station::Station(BString name, BString uri)
	:
	fName(name),
	fStreamUrl(uri),
	fStationUrl(B_EMPTY_STRING),
	fGenre(B_EMPTY_STRING),
	fSource(B_EMPTY_STRING),
	fMime(B_EMPTY_STRING),
	fEncoding(0),
	fLogo(NULL),
	fRating(0),
	fBitRate(0),
	fSampleRate(0),
	fMetaInterval(0),
	fChannels(0),
	fFlags(0)
{
	checkFlags();
	if (Flags(STATION_URI_VALID) && !Flags(STATION_HAS_FORMAT))
		Probe();
}


Station::Station(const Station& orig)
	:
	fName(orig.fName),
	fStreamUrl(orig.fStreamUrl),
	fStationUrl(orig.fStationUrl),
	fGenre(orig.fGenre),
	fCountry(orig.fCountry),
	fLanguage(orig.fLanguage),
	fSource(B_EMPTY_STRING),
	fRating(orig.fRating),
	fBitRate(orig.fBitRate),
	fSampleRate(orig.fSampleRate),
	fChannels(orig.fChannels),
	fEncoding(orig.fEncoding),
	fMetaInterval(orig.fMetaInterval)
{
	fMime.SetTo(orig.fMime.Type());
	fLogo = (orig.fLogo) ? new BBitmap(orig.fLogo) : NULL;
	unsaved = true;
}


Station::~Station()
{
	delete fLogo;
}


status_t
Station::InitCheck()
{
	return (fFlags & STATION_HAS_NAME && fFlags & STATION_HAS_URI) ? B_OK
																   : B_ERROR;
}


Station*
Station::LoadFromPlsFile(BString Name)
{
	BEntry stationEntry;
	stationEntry.SetTo(StationDirectory(), Name);
	Station* station = Load(Name, &stationEntry);
	if (station)
		station->unsaved = false;
	return station;
}


status_t
Station::Save()
{
	status_t status;
	BFile stationFile;
	BDirectory* stationDir = StationDirectory();

	if (!stationDir)
		return B_ERROR;

	status = stationDir->CreateFile(fName, &stationFile, false);
	if (status != B_OK)
		return status;
	BString content;
	content << "[playlist]\nNumberOfEntries=1\nFile1=" << fStreamUrl << "\n";
	stationFile.Write(
		content.LockBuffer(-1), content.CountBytes(0, content.CountChars()));
	content.UnlockBuffer();
	status = stationFile.Lock();
	status = stationFile.WriteAttrString("META:url", &fStreamUrl.UrlString());
	status = stationFile.WriteAttr(
		"BEOS:TYPE", B_MIME_TYPE, 0, MimePls, strlen(MimePls));
	status = stationFile.WriteAttr(
		"META:bitrate", B_INT32_TYPE, 0, &fBitRate, sizeof(fBitRate));
	status = stationFile.WriteAttr(
		"META:samplerate", B_INT32_TYPE, 0, &fSampleRate, sizeof(fSampleRate));
	status = stationFile.WriteAttr(
		"META:channels", B_INT32_TYPE, 0, &fChannels, sizeof(fChannels));
	status = stationFile.WriteAttr(
		"META:framesize", B_INT32_TYPE, 0, &fFrameSize, sizeof(fFrameSize));
	status = stationFile.WriteAttr(
		"META:rating", B_INT32_TYPE, 0, &fRating, sizeof(fRating));
	status = stationFile.WriteAttr("META:interval", B_INT32_TYPE, 0,
		&fMetaInterval, sizeof(fMetaInterval));
	status = stationFile.WriteAttrString("META:genre", &fGenre);
	status = stationFile.WriteAttrString("META:country", &fCountry);
	status = stationFile.WriteAttrString("META:language", &fLanguage);
	status = stationFile.WriteAttrString("META:source", &fSource.UrlString());
	status = stationFile.WriteAttrString(
		"META:stationurl", &fStationUrl.UrlString());
	BString mimeType(fMime.Type());
	status = stationFile.WriteAttrString("META:mime", &mimeType);
	status = stationFile.WriteAttr(
		"META:encoding", B_INT32_TYPE, 0, &fEncoding, sizeof(fEncoding));
	status = stationFile.Unlock();
	BNodeInfo stationInfo;
	stationInfo.SetTo(&stationFile);
	if (fLogo) {
		BMessage archive;
		fLogo->Archive(&archive);
		ssize_t archiveSize = archive.FlattenedSize();
		char* archiveBuffer = (char*) malloc(archiveSize);
		archive.Flatten(archiveBuffer, archiveSize);
		stationFile.WriteAttr("logo", 'BBMP', 0LL, archiveBuffer, archiveSize);
		free(archiveBuffer);

		BBitmap* icon = new BBitmap(
			BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1), B_RGB32, true);
		BView* canvas = new BView(icon->Bounds(), "canvas", B_FOLLOW_NONE, 0);
		icon->AddChild(canvas);
		canvas->LockLooper();
		canvas->DrawBitmap(fLogo, fLogo->Bounds(), icon->Bounds());
		canvas->UnlockLooper();
		icon->RemoveChild(canvas);
		stationInfo.SetIcon(icon, B_LARGE_ICON);
		delete icon;

		icon = new BBitmap(BRect(0, 0, 15, 15), B_RGB32, true);
		canvas->ResizeTo(16, 16);
		icon->AddChild(canvas);
		canvas->LockLooper();
		canvas->DrawBitmap(
			fLogo, fLogo->Bounds(), icon->Bounds(), B_FILTER_BITMAP_BILINEAR);
		canvas->UnlockLooper();
		icon->RemoveChild(canvas);
		stationInfo.SetIcon(icon, B_MINI_ICON);
		delete icon;
		delete canvas;
	}
	stationInfo.SetType(MimePls);
	stationFile.Unset();
	return status;
}


status_t
Station::RetrieveStreamUrl()
{
	if (!fSource.IsValid())
		return B_ERROR;

	status_t status = B_ERROR;
	BString contentType("*/*");
	BMallocIO* plsData
		= HttpUtils::GetAll(fSource, NULL, 100000, &contentType, 2000);
	if (plsData)
		status
			= parseUrlReference((const char*) plsData->Buffer(), contentType);
	delete plsData;
	if (status != B_OK && contentType.StartsWith("audio/")) {
		SetStreamUrl(fSource);
		return B_OK;
	}
	return status;
}


status_t
Station::Probe()
{
	BHttpHeaders headers;
	BMallocIO* buffer;
	BUrl* resolvedUrl = new BUrl();
	BString contentType("audio/*");

	// If source URL is a playlist, retrieve the actual stream URL
	// if (fSource.Path().EndsWith(".pls") ||
	//					fSource.Path().EndsWith(".m3u") ||
	//					fSource.Path().EndsWith(".m3u8"))
	RetrieveStreamUrl();

	// FIXME: UGLY HACK!
	// Currently Haiku's HttpRequest is not able to check for connection on a
	// specified IP address/port pair before actually resolving the hostname to
	// that pair. That means, if the first server in the DNS response is down or
	// has its port closed for some reason, the request will simply fail.
	//
	// Creating a socket and trying to connect to the specified IP/port pair is
	// enough for our purposes. That way, if a server is down we can use the
	// next DNS record, and craft a new URL like
	// http://192.168.0.1/path/to/stream
	//
	// On HTTPS, unfortunately, this isn't possible, as the certificate is
	// associated to the hostname, and so we have to make the request using it.
	// The solution to this would be to fix the BHttpRequest class, but, for the
	// moment, we'll just use the hostname directly if HTTPS is used. The number
	// of streams using HTTPS and load balancing between two or more different
	// IP's should be small, anyway.

	if (fStreamUrl.Protocol() == "https")
		buffer = HttpUtils::GetAll(
			fStreamUrl, &headers, 2 * 1000 * 1000, &contentType, 4096);
	else {
		status_t resolveStatus = HttpUtils::CheckPort(fStreamUrl, resolvedUrl);
		if (resolveStatus != B_OK)
			return B_ERROR;
		buffer = HttpUtils::GetAll(
			*resolvedUrl, &headers, 2 * 1000 * 1000, &contentType, 4096);
		delete resolvedUrl;
	}
#ifdef DEBUGGING
	for (int i = 0; i < headers.CountHeaders(); i++)
		TRACE("Header: %s\r\n", headers.HeaderAt(i).Header());
#endif
	if (buffer == NULL) {
		fFlags &= !STATION_URI_VALID;
		return B_ERROR;
	}
	if (headers.CountHeaders() == 0) {
		fFlags &= !STATION_URI_VALID;
		return B_TIMED_OUT;
	}

	int index;
	if ((index = headers.HasHeader("content-type")) >= 0)
		fMime.SetTo(headers.HeaderValue("content-type"));
	// If the station has no name, try to use the Icy-Name header
	if ((index = headers.HasHeader("Icy-Name")) >= 0 && fName.IsEmpty())
		SetName(headers.HeaderValue("Icy-Name"));
	if ((index = headers.HasHeader("Icy-Br")) >= 0)
		fBitRate = atoi(headers[index].Value()) * 1000;
	if ((index = headers.HasHeader("Icy-Genre")) >= 0)
		fGenre = headers[index].Value();
	if ((index = headers.HasHeader("Icy-Url")) >= 0
		&& strlen(headers[index].Value()) > 0)
		fStationUrl.SetUrlString(headers[index].Value());
	if ((index = headers.HasHeader("Ice-Audio-Info")) >= 0) {
		BString audioInfo(headers[index].Value());
		BStringList audioInfoList;
		if (audioInfo.Split(";", false, audioInfoList)) {
			for (int32 i = 0; i < audioInfoList.CountStrings(); i++) {
				BString audioInfoItem = audioInfoList.StringAt(i);
				if (audioInfoItem.StartsWith("ice-samplerate=")) {
					audioInfoItem.Remove(0, 15);
					fSampleRate = atoi(audioInfoItem.String());
				}
				if (audioInfoItem.StartsWith("ice-bitrate=")) {
					audioInfoItem.Remove(0, 12);
					fBitRate = atoi(audioInfoItem.String()) * 1000;
				}
				if (audioInfoItem.StartsWith("ice-channels=")) {
					audioInfoItem.Remove(0, 13);
					fChannels = atoi(audioInfoItem.String());
				}
			}
		}
	}
	if ((index = headers.HasHeader("Icy-metaint")) >= 0)
		fMetaInterval = atoi(headers[index].Value());

	checkFlags();
	unsaved = true;
	ProbeBuffer(buffer);
	delete buffer;
	return B_OK;
}


status_t
Station::ProbeBuffer(BPositionIO* buffer)
{
	status_t status = B_OK;
	BMediaFile mediaFile(buffer);
	if ((status = mediaFile.InitCheck()) != B_OK)
		return status;

	if (mediaFile.CountTracks() < 1)
		return B_ERROR;

	BMediaTrack* mediaTrack = mediaFile.TrackAt(0);

	media_format encodedFormat, decodedFormat;
	media_codec_info codecInfo;

	status = mediaTrack->EncodedFormat(&encodedFormat);
	if (status != B_OK)
		return status;

	fSampleRate = int32(encodedFormat.u.encoded_audio.output.frame_rate);
	fChannels = encodedFormat.u.encoded_audio.output.channel_count;
	fBitRate = int32(encodedFormat.u.encoded_audio.bit_rate);
	fEncoding = encodedFormat.u.encoded_audio.encoding;
	fFrameSize = encodedFormat.u.encoded_audio.frame_size;

	status = mediaTrack->DecodedFormat(&decodedFormat);
	checkFlags();
	return status;
}


status_t
Station::parseUrlReference(const char* body, const char* mime)
{

	const char* patterns[4] = {"^file[0-9]+=([^\r\n]*)[\r\n$]+", // ShoutcastUrl
		"^(http://[^\r\n]*)[\r\n]+$", // Mpeg Url;
		"^([^#]+[^\r\n]*)[\r\n]+$", // Mpeg Url;
		"^title[0-9]+=([^\r\n]*)[\r\n$]+"}; // Shoutcast alternativ;

	for (int i = 0; i < 3; i++) {
		char* match = regFind(body, patterns[i]);
		if (match) {
			fStreamUrl.SetUrlString(match);
			free(match);
			match = regFind(body, patterns[3]);
			if (match) {
				SetName(match);
				free(match);
			}
			return B_OK;
		}
	}
	return B_ERROR;
}


Station*
Station::Load(BString Name, BEntry* entry)
{
	off_t size;
	status_t status;
	Station* station = new Station(Name);
	BFile file;
	file.SetTo(entry, B_READ_ONLY);
	BNodeInfo stationInfo;
	stationInfo.SetTo(&file);

	BString readString;
	status = file.ReadAttrString("META:url", &readString);
	station->fStreamUrl.SetUrlString(readString);
	status = file.ReadAttrString("META:genre", &station->fGenre);
	status = file.ReadAttrString("META:country", &station->fCountry);
	status = file.ReadAttrString("META:language", &station->fLanguage);
	status = file.ReadAttr("META:bitrate", B_INT32_TYPE, 0, &station->fBitRate,
		sizeof(station->fBitRate));
	status = file.ReadAttr("META:rating", B_INT32_TYPE, 0, &station->fRating,
		sizeof(station->fRating));
	status = file.ReadAttr("META:interval", B_INT32_TYPE, 0,
		&station->fMetaInterval, sizeof(station->fMetaInterval));
	status = file.ReadAttr("META:samplerate", B_INT32_TYPE, 0,
		&station->fSampleRate, sizeof(station->fSampleRate));
	status = file.ReadAttr("META:channels", B_INT32_TYPE, 0,
		&station->fChannels, sizeof(station->fChannels));
	status = file.ReadAttr("META:encoding", B_INT32_TYPE, 0,
		&station->fEncoding, sizeof(station->fEncoding));
	status = file.ReadAttr("META:framesize", B_INT32_TYPE, 0,
		&station->fFrameSize, sizeof(station->fFrameSize));
	status = file.ReadAttrString("META:mime", &readString);
	station->fMime.SetTo(readString);
	status = file.ReadAttrString("META:source", &readString);
	station->fSource.SetUrlString(readString);
	status = file.ReadAttrString("META:stationurl", &readString);
	station->fStationUrl.SetUrlString(readString);

	attr_info attrInfo;
	status = file.GetAttrInfo("logo", &attrInfo);
	if (status == B_OK) {
		char* archiveBuffer = (char*) malloc(attrInfo.size);
		file.ReadAttr("logo", attrInfo.type, 0LL, archiveBuffer, attrInfo.size);
		BMessage archive;
		archive.Unflatten(archiveBuffer);
		free(archiveBuffer);
		station->fLogo = (BBitmap*) BBitmap::Instantiate(&archive);
	} else
		station->fLogo = new BBitmap(BRect(0, 0, 32, 32), B_RGB32);
	status = stationInfo.GetIcon(station->fLogo, B_LARGE_ICON);
	if (status != B_OK)
		delete station->fLogo;
	station->fLogo = new BBitmap(BRect(0, 0, 16, 16), B_RGB32);
	status = stationInfo.GetIcon(station->fLogo, B_MINI_ICON);
	if (status != B_OK) {
		delete station->fLogo;
		station->fLogo = NULL;
	}

	if (!station->fSource.IsValid()) {
		BPath path(entry);
		station->fSource.SetUrlString(BString("file://") << path.Path());
	}


	if (!station->fStreamUrl.IsValid()) {
		status = file.GetSize(&size);
		if (size > 10000)
			return NULL;
		char* buffer = (char*) malloc(size);
		file.Read(buffer, size);
		char mime[64];
		stationInfo.GetType(mime);
		station->parseUrlReference(buffer, mime);
		free(buffer);
	}
	if (Name == B_EMPTY_STRING)
		station->fName.SetTo(entry->Name());

	station->checkFlags();

	if (station->InitCheck() != B_OK) {
		delete station;
		return NULL;
	} else
		return station;
}


char*
Station::regFind(const char* Text, const char* Pattern)
{
	regex_t patternBuffer;
	regmatch_t matchBuffer[20];
	int result;
	char* match = NULL;
	memset(&patternBuffer, 0, sizeof(patternBuffer));
	memset(matchBuffer, 0, sizeof(matchBuffer));
	result = regcomp(
		&patternBuffer, Pattern, REG_EXTENDED | REG_NEWLINE | REG_ICASE);
	result = regexec(&patternBuffer, Text, 20, matchBuffer, 0);
	if (result == 0 && matchBuffer[1].rm_eo > -1)
		match = strndup(Text + matchBuffer[1].rm_so,
			matchBuffer[1].rm_eo - matchBuffer[1].rm_so);
	return match;
}


Station*
Station::LoadIndirectUrl(BString& sUrl)
{
	status_t status;
	const char* patternTitle = "<title[^>]*>(.*?)</title[^>]*>";
	const char* patternIcon
		= "<link\\s*rel=\"shortcut icon\"\\s*href=\"([^\"]*?)\".*?";

	BUrl url(sUrl);
	BString contentType("*/*");
	BMallocIO* dataIO = HttpUtils::GetAll(url, NULL, 10000, &contentType, 2000);

	if (dataIO == NULL)
		return NULL;

	Station* station = new Station(B_EMPTY_STRING, B_EMPTY_STRING);
	dataIO->Write("", 1);
	const char* body = (char*) dataIO->Buffer();
	int32 pos = contentType.FindFirst(';');
	if (pos >= 0)
		contentType.Truncate(pos);
	status = station->parseUrlReference(body, contentType.String());
	if (status != B_OK && contentType.StartsWith(("audio/")))
		station->SetStreamUrl(url);
	station->fSource.SetUrlString(sUrl);
	delete dataIO;

	if (!station->fStreamUrl.IsValid()) {
		delete station;
		return NULL;
	}

	/*
	 *  Check for name and logo on same server by calling main page
	 */


	BUrl finalUrl = station->fStationUrl;
	if ((!finalUrl.HasPort() || finalUrl.Port() == 80)
		&& (!finalUrl.HasPath() || finalUrl.Path().IsEmpty()
			|| finalUrl.Path() == "/")) {
		if (station->fName.IsEmpty())
			station->SetName("New Station");
	} else
		finalUrl.SetFragment(NULL);
	finalUrl.SetRequest(NULL);
	finalUrl.SetPath(NULL);
	finalUrl.SetPort(80);
	sUrl.SetTo(finalUrl.UrlString());
	sUrl.RemoveCharsSet("#?");
	finalUrl.SetUrlString(sUrl);

	dataIO = HttpUtils::GetAll(finalUrl);
	if (dataIO) {
		dataIO->Write(&"", 1);
		body = (char*) dataIO->Buffer();
		char* title = regFind(body, patternTitle);
		if (title)
			station->fName.SetTo(title);

		char* icon = regFind(body, patternIcon);
		if (icon)
			finalUrl.SetPath(BString(icon));

		contentType = "image/*";

		BMallocIO* iconIO
			= HttpUtils::GetAll(finalUrl, NULL, 10000, &contentType, 2000);
		if (iconIO) {
			iconIO->Seek(0, SEEK_SET);
			station->fLogo = BTranslationUtils::GetBitmap(iconIO);
			delete iconIO;
		}

		delete dataIO;
	}

	return station;
}


void
Station::SetName(BString name)
{
	if (fName.Compare(name) == 0)
		return;

	BEntry* entry = NULL;
	if (StationDirectory()->Contains(fName, B_FILE_NODE)) {
		entry = new BEntry();
		StationDirectory()->FindEntry(fName, entry, false);
	}

	fName.SetTo(name);
	cleanName();
	if (fName.IsEmpty())
		fFlags &= !STATION_HAS_NAME;
	else
		fFlags |= STATION_HAS_NAME;

	if (entry) {
		entry->Rename(fName);
		delete entry;
		Save();
	} else
		unsaved = true;
}


void
Station::cleanName()
{
	if (fName.Compare("(#", 2) == 0)
		if (0 <= fName.FindFirst(')') < fName.Length())
			fName.Remove(0, fName.FindFirst(')') + 1).Trim();

	fName.RemoveCharsSet("\\/#?");
}


void
Station::checkFlags()
{
	fFlags = 0;
	if (!fName.IsEmpty())
		fFlags |= STATION_HAS_NAME;
	if (fStreamUrl.HasHost())
		fFlags |= STATION_HAS_URI;
	if (fStreamUrl.IsValid())
		fFlags |= STATION_URI_VALID;
	if (fBitRate != 0)
		fFlags |= STATION_HAS_BITRATE;
	if (fMime.IsValid() && fEncoding != 0)
		fFlags |= STATION_HAS_ENCODING;
	if (fChannels != 0 && fSampleRate != 0)
		fFlags |= STATION_HAS_FORMAT;
	if (fMetaInterval != 0)
		fFlags |= STATION_HAS_META;
}

BDirectory* Station::fStationsDirectory = NULL;


BDirectory*
Station::StationDirectory()
{
	if (fStationsDirectory)
		return fStationsDirectory;

	status_t status;
	BPath configPath;
	BDirectory configDir;

	status = find_directory(B_USER_SETTINGS_DIRECTORY, &configPath);
	configDir.SetTo(configPath.Path());
	if (configDir.Contains(SubDirStations, B_DIRECTORY_NODE))
		fStationsDirectory = new BDirectory(&configDir, SubDirStations);
	else {
		fStationsDirectory = new BDirectory();
		configDir.CreateDirectory(SubDirStations, fStationsDirectory);
		BAlert* alert = new BAlert(B_TRANSLATE("Stations directory created"),
			B_TRANSLATE(
				"A directory for saving stations has been created in your "
				"settings folder. Link this directory to your deskbar menu "
				"to play stations directly."),
			B_TRANSLATE("OK"));
		alert->Go();
	}

	return fStationsDirectory;
}
