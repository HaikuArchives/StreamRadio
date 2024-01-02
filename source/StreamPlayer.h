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
#ifndef _STREAM_PLAYER_H
#define _STREAM_PLAYER_H


#include <MediaFile.h>
#include <SoundPlayer.h>

#include <MediaIO.h>

#include "Station.h"
#include "StreamIO.h"


// Notification Messages
#define MSG_PLAYER_STATE_CHANGED \
	'mPSC' // "player" = StreamPlayer*, "state" = int32(playState)
#define MSG_PLAYER_BUFFER_LEVEL \
	'mPBL' // "player" = StreamPlayer*, "level" = float percent buffer filled


class Station;


class StreamPlayer : private BLocker {
public:
								StreamPlayer(Station* station,
									BLooper* notify = NULL);
	virtual						~StreamPlayer();

	inline	status_t			InitCheck() { return fInitStatus; }
	inline	Station*			GetStation() { return fStation; }
			status_t			Play();
			status_t			Stop();
			float				Volume();
			void				SetVolume(float volume);

	enum	PlayState 			{ InActive = -1, Stopped, Playing, Buffering };
	inline	PlayState 			State() { return fState; }

private:
			void				_SetState(PlayState state);

	static	status_t			_StartPlayThreadFunc(StreamPlayer* _this);
	static	void				_GetDecodedChunk(void* cookie, void* buffer,
									size_t size,
									const media_raw_audio_format& format);

private:
			Station*			fStation;
			BLooper*			fNotify;
			BMediaFile*			fMediaFile;
			StreamIO*			fStream;
			BSoundPlayer*		fPlayer;
			PlayState			fState;

			status_t			fInitStatus;
			bool				fStopRequested;

			media_codec_info	fCodecInfo;
			media_format		fDecodedFormat;
			media_header		fHeader;
			media_decode_info	fInfo;
			int32				fFlushCount;
};


#endif // _STREAM_PLAYER_H
