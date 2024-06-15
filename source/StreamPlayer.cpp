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


#include <Catalog.h>
#include <MediaDecoder.h>
#include <MediaFile.h>
#include <MediaTrack.h>

#include "Debug.h"
#include "StreamIO.h"
#include "StreamPlayer.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StreamPlayer"


StreamPlayer::StreamPlayer(Station* station, BLooper* notify)
	: BLocker("StreamPlayer"),
	  fStation(station),
	  fNotify(notify),
	  fMediaFile(NULL),
	  fPlayer(NULL),
	  fState(StreamPlayer::Stopped),
	  fFlushCount(0)
{
	TRACE("Trying to set player for stream %s\n", station->StreamUrl().UrlString().String());

	fStream = new (std::nothrow) StreamIO(station, notify);
	fInitStatus = fStream->Open();
	if (fInitStatus != B_OK) {
		MSG("Error retrieving stream from %s - %s\n", station->StreamUrl().UrlString().String(),
			strerror(fInitStatus));
		return;
	}
}


StreamPlayer::~StreamPlayer()
{
	//_SetState(StreamPlayer::Stopped);
	if (fInitStatus == B_OK && fPlayer != NULL)
		fPlayer->Stop(true, false);

	delete fPlayer;
	delete fStream;
	delete fMediaFile;
}


status_t
StreamPlayer::Play()
{
	switch (fState) {
		case StreamPlayer::Playing:
		case StreamPlayer::Buffering:
		case StreamPlayer::InActive:
			break;

		case StreamPlayer::Stopped:
		{
			thread_id playThreadId = spawn_thread((thread_func)StreamPlayer::_StartPlayThreadFunc,
				fStation->Name()->String(), B_NORMAL_PRIORITY, this);
			resume_thread(playThreadId);

			break;
		}
	}

	return B_OK;
}


status_t
StreamPlayer::Stop()
{
	switch (fState) {
		case StreamPlayer::Buffering:
			fStopRequested = true;
			break;

		case StreamPlayer::Playing:
		{
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
			_SetState(StreamPlayer::Stopped);

			break;
		}

		case StreamPlayer::Stopped:
		case StreamPlayer::InActive:
			break;
	}
	return B_OK;
}


float
StreamPlayer::Volume()
{
	if (fPlayer != NULL) {
		media_node node;
		int32 paramID;
		float minDB, maxDB;
		fPlayer->GetVolumeInfo(&node, &paramID, &minDB, &maxDB);

		return (fPlayer->VolumeDB(false) - minDB) / (maxDB - minDB);
	} else
		return 0;
}


void
StreamPlayer::SetVolume(float volume)
{
	if (fPlayer != NULL) {
		media_node node;
		int32 paramID;
		float minDB, maxDB;
		fPlayer->GetVolumeInfo(&node, &paramID, &minDB, &maxDB);
		fPlayer->SetVolumeDB(minDB + volume * (maxDB - minDB));
	}
}


void
StreamPlayer::_SetState(StreamPlayer::PlayState state)
{
	if (fState != state)
		fState = state;

	if (fNotify != NULL) {
		BMessage* notification = new BMessage(MSG_PLAYER_STATE_CHANGED);
		BMessenger messenger(fNotify);
		notification->AddPointer("player", this);
		notification->AddInt32("state", fState);
		messenger.SendMessage(notification, fNotify);
	}
}


void
StreamPlayer::_GetDecodedChunk(
	void* cookie, void* buffer, size_t size, const media_raw_audio_format& format)
{
	StreamPlayer* player = (StreamPlayer*)cookie;
	BMediaFile* fMediaFile = player->fMediaFile;

	int64 reqFrames
		= size / format.channel_count / (format.format & media_raw_audio_format::B_AUDIO_SIZE_MASK);
	fMediaFile->TrackAt(0)->ReadFrames(buffer, &reqFrames, &player->fHeader, &player->fInfo);

	if (player->fFlushCount++ > 1000) {
		player->fFlushCount = 0;
		player->fStream->FlushRead();
	}
}


status_t
StreamPlayer::_StartPlayThreadFunc(StreamPlayer* _this)
{
	_this->Lock();

	_this->_SetState(StreamPlayer::Buffering);
	_this->fStopRequested = false;
	_this->fStream->SetLimiter(0x40000);
	_this->fMediaFile = new (std::nothrow) BMediaFile(_this->fStream);

	_this->fInitStatus = _this->fMediaFile->InitCheck();
	if (_this->fInitStatus != B_OK) {
		MSG("Error creating media extractor for %s - %s\n",
			_this->fStation->StreamUrl().UrlString().String(), strerror(_this->fInitStatus));
	}

	if (_this->fInitStatus != B_OK || _this->fStopRequested) {
		delete _this->fStream;
		_this->fStream = NULL;
		delete _this->fMediaFile;
		_this->fMediaFile = NULL;

		_this->Unlock();
		_this->_SetState(StreamPlayer::Stopped);

		return _this->fInitStatus;
	}

	_this->fStream->SetLimiter(0);

	if (_this->fMediaFile->CountTracks() == 0) {
		MSG("No track found inside stream %s - %s\n",
			_this->fStation->StreamUrl().UrlString().String(), strerror(_this->fInitStatus));
	}

	if (_this->fStopRequested) {
		delete _this->fMediaFile;
		_this->fMediaFile = NULL;

		_this->Unlock();
		_this->_SetState(StreamPlayer::Stopped);

		return _this->fInitStatus;
	}

	_this->fInitStatus = _this->fMediaFile->TrackAt(0)->GetCodecInfo(&_this->fCodecInfo);
	if (_this->fInitStatus != B_OK) {
		MSG("Could not create decoder for %s - %s\n",
			_this->fStation->StreamUrl().UrlString().String(), strerror(_this->fInitStatus));
	}

	if (_this->fInitStatus != B_OK || _this->fStopRequested) {
		delete _this->fMediaFile;
		_this->fMediaFile = NULL;

		_this->Unlock();
		_this->_SetState(StreamPlayer::Stopped);

		return _this->fInitStatus;
	}

	_this->fInitStatus = _this->fMediaFile->TrackAt(0)->DecodedFormat(&_this->fDecodedFormat);
	if (_this->fInitStatus != B_OK) {
		MSG("Could not negotiate output format - %s\n", strerror(_this->fInitStatus));
	}

	if (_this->fInitStatus != B_OK || _this->fStopRequested) {
		delete _this->fMediaFile;
		_this->fMediaFile = NULL;

		_this->Unlock();
		_this->_SetState(StreamPlayer::Stopped);

		return _this->fInitStatus;
	}

	TRACE("Found stream with %ld channels with %f Hz\n",
		_this->fDecodedFormat.u.raw_audio.channel_count,
		_this->fDecodedFormat.u.raw_audio.frame_rate);

	_this->fPlayer = new BSoundPlayer(&_this->fDecodedFormat.u.raw_audio,
		_this->fStation->Name()->String(), &StreamPlayer::_GetDecodedChunk, NULL, _this);

	_this->fInitStatus = _this->fPlayer->InitCheck();
	if (_this->fInitStatus != B_OK) {
		MSG("Sound Player failed to initialize (%s)\r\n", strerror(_this->fInitStatus));
	}

	if (_this->fInitStatus != B_OK || _this->fStopRequested) {
		_this->fPlayer->Stop(true, true);
		delete _this->fPlayer;
		_this->fPlayer = NULL;

		_this->Unlock();
		_this->_SetState(StreamPlayer::Stopped);

		return _this->fInitStatus;
	}

	_this->fPlayer->Preroll();

	_this->fInitStatus = _this->fPlayer->Start();
	if (_this->fInitStatus != B_OK) {
		MSG("Sound Player failed to start (%s)\r\n", strerror(_this->fInitStatus));
	}

	if (_this->fInitStatus != B_OK || _this->fStopRequested) {
		delete _this->fPlayer;
		_this->fPlayer = NULL;

		_this->Unlock();
		_this->_SetState(StreamPlayer::Stopped);

		return _this->fInitStatus;
	}

	_this->_SetState(StreamPlayer::Playing);
	_this->Unlock();

	return _this->fInitStatus;
}
