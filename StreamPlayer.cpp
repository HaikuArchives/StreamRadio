/* 
 * File:   StreamPlayer.cpp
 * Author: Kai Niessen
 * 
 * Created on January 25, 2016, 6:49 PM
 */

#include <MediaDecoder.h>
#include <MediaFile.h>
#include <MediaTrack.h>

#include "StreamPlayer.h"
#include "Debug.h"
#include "StreamIO.h"

#define milliseconds * 1000
#define seconds * 1000 milliseconds

StreamPlayer::StreamPlayer(Station* station, BLooper* Notify) 
  : BLocker(),
	fStation(station),
    fNotify(Notify),
    fNoNotifyCount(0),
    fMediaFile(NULL),
    fPlayer(NULL),
    fState(Stopped)
{
    TRACE("Trying to set player for stream %s\n", station->StreamUrl().UrlString().String());
	
	fStream = new (std::nothrow) StreamIO(station, Notify);
	fInitStatus = fStream->Open();
	if (fInitStatus != B_OK) {
		MSG("Error retrieving stream from %s - %s\n", 
		station->StreamUrl().UrlString().String(), 
		strerror(fInitStatus));
		return;
	}
}

StreamPlayer::~StreamPlayer() {
    setState(Stopped);
    if (fInitStatus == B_OK && fPlayer)
		fPlayer->Stop(true, false);
    delete fPlayer;
	delete fStream;
    delete fMediaFile;
}

void
StreamPlayer::GetDecodedChunk(void* cookie, void* buffer, size_t size, const media_raw_audio_format& format) {
    StreamPlayer* player = (StreamPlayer*)(cookie);
    BMediaFile* fMediaFile = player->fMediaFile;
    int64 reqFrames = size / format.channel_count / (format.format & media_raw_audio_format::B_AUDIO_SIZE_MASK);
	fMediaFile->TrackAt(0)->ReadFrames(buffer, &reqFrames, &player->fHeader, &player->fInfo);
}

status_t
StreamPlayer::Play() {
	switch(fState) {
	case Playing:
	case Buffering:
		return B_OK;
		break;
	case Stopped:
		thread_id playThreadId = spawn_thread((thread_func)StreamPlayer::StartPlayThreadFunc, fStation->Name()->String(), B_NORMAL_PRIORITY, this);
		resume_thread(playThreadId);
		break;
	}
    
}

#ifdef DEBUG
void dumpMessage(BMessage* msg) {
	int n = msg->CountNames(B_ANY_TYPE);
	type_code typeCode;
	int32 arraySize;
	char* name;
	for (long int i=0; i<n; i++) {
		msg->GetInfo((long unsigned int)B_ANY_TYPE, i, &name, &typeCode, &arraySize);
		MSG("Meta data %s = %s\n", name, name);
	}
}
#else
void inline dumpMessage(BMessage* msg) { }
#endif

status_t 
StreamPlayer::StartPlayThreadFunc(StreamPlayer* _this) {
	_this->Lock();
    _this->setState(Buffering);
	_this->fStopRequested = false;
	_this->fStream->SetLimiter(0x40000);
	_this->fMediaFile = new (std::nothrow) BMediaFile(_this->fStream);
	_this->fInitStatus = _this->fMediaFile->InitCheck();
    if (_this->fInitStatus != B_OK) 
		MSG("Error creating media extractor for %s - %s\n", _this->fStation->StreamUrl().UrlString().String(), strerror(_this->fInitStatus));

	if (_this->fInitStatus != B_OK || _this->fStopRequested) {
		delete _this->fStream;
		_this->fStream = NULL;
		delete _this->fMediaFile;
		_this->fMediaFile = NULL;
		_this->Unlock();
		_this->setState(Stopped);
		return _this->fInitStatus;
    }
	_this->fStream->SetLimiter(0);
    if (_this->fMediaFile->CountTracks() == 0) 
		MSG("No track found inside stream %s - %s\n", _this->fStation->StreamUrl().UrlString().String(), strerror(_this->fInitStatus));

	if (_this->fInitStatus != B_OK || _this->fStopRequested) {
		delete _this->fMediaFile;
		_this->fMediaFile = NULL;
		_this->Unlock();
		_this->setState(Stopped);
		return _this->fInitStatus;
    }

	_this->fInitStatus = _this->fMediaFile->TrackAt(0)->GetCodecInfo(&_this->fCodecInfo);
    if (_this->fInitStatus != B_OK) 
		MSG("Could not create decoder for %s - %s\n", _this->fStation->StreamUrl().UrlString().String(), strerror(_this->fInitStatus));

	if (_this->fInitStatus != B_OK || _this->fStopRequested) {
		delete _this->fMediaFile;
		_this->fMediaFile = NULL;
		_this->Unlock();
		_this->setState(Stopped);
		return _this->fInitStatus;
    }	
	
    _this->fInitStatus = _this->fMediaFile->TrackAt(0)->DecodedFormat(&_this->fDecodedFormat);
	if (_this->fInitStatus != B_OK) 
		MSG("Could not negotiate output format - %s\n", strerror(_this->fInitStatus));
	
	if (_this->fInitStatus != B_OK || _this->fStopRequested) {
		delete _this->fMediaFile;
		_this->fMediaFile = NULL;
		_this->Unlock();
		_this->setState(Stopped);
		return _this->fInitStatus;
	}

	TRACE("Found stream with %ld channels with %f Hz\n", 
	    _this->fDecodedFormat.u.raw_audio.channel_count,
	    _this->fDecodedFormat.u.raw_audio.frame_rate);
	
    _this->fPlayer  = new BSoundPlayer(&_this->fDecodedFormat.u.raw_audio, 
			_this->fStation->Name()->String(), 
			&StreamPlayer::GetDecodedChunk, 
			NULL, _this);

    _this->fStatus  = _this->fPlayer->InitCheck();
    if (_this->fStatus != B_OK) 
        MSG("Sound Player failed to initialize (%s)\r\n", strerror(_this->fStatus));
	
	if (_this->fInitStatus != B_OK || _this->fStopRequested) {
        _this->fPlayer->Stop(true, true);
        delete _this->fPlayer;
        _this->fPlayer      = NULL;
		_this->Unlock();
        _this->setState(Stopped);
        return _this->fStatus;
    }
    
    _this->fPlayer->Preroll();
    
    _this->fInitStatus     = _this->fPlayer->Start();
    if (_this->fInitStatus != B_OK) 
        MSG("Sound Player failed to start (%s)\r\n", strerror(_this->fStatus));
	
	if (_this->fInitStatus != B_OK || _this->fStopRequested) {
        delete _this->fPlayer;
        _this->fPlayer    = NULL;
		_this->Unlock();
        _this->setState(Stopped);
        return _this->fInitStatus;
    }
    
    _this->setState(Playing);
	_this->Unlock();
    return _this->fStatus;
}

status_t
StreamPlayer::Stop() {
	switch (fState) {
	case Buffering:
		fStopRequested = true;
		break;
	case Playing:
		Lock();
		if (fPlayer) {
			fPlayer->Stop(false, true);
			delete fPlayer;
			fPlayer = NULL;
		} 
		if (fMediaFile) {
			fMediaFile->CloseFile();
			delete fMediaFile;
			fMediaFile = NULL;
		}
		Unlock();
		setState(StreamPlayer::Stopped);
		break;
	case Stopped:
		case StreamPlayer::InActive:
		return B_OK;
		break;
	}
}

void 
StreamPlayer::setState(StreamPlayer::PlayState state) {
    if (fState != state) {
        fState = state;
        if (fNotify) {
            BMessage* notification = new BMessage(MSG_PLAYER_STATE_CHANGED);
			BMessenger messenger(fNotify);
            notification->AddPointer("player", this);
            notification->AddInt32("state", fState);
            messenger.SendMessage(notification, fNotify);
        }
    }
}

float
StreamPlayer::Volume() {
	if (fPlayer) {
		media_node node;
		int32 paramID;
		float minDB, maxDB;
		fPlayer->GetVolumeInfo(&node, &paramID, &minDB, &maxDB);
		return (fPlayer->VolumeDB(false) - minDB) / (maxDB - minDB);
	} else
		return 0;
}

void
StreamPlayer::SetVolume(float volume) {
	if (fPlayer) {
		media_node node;
		int32 paramID;
		float minDB, maxDB;
		fPlayer->GetVolumeInfo(&node, &paramID, &minDB, &maxDB);
		fPlayer->SetVolumeDB(minDB + volume * (maxDB - minDB));
	}
}
