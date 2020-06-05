/*
 * File:   HttpPlayer.h
 * Author: Kai Niessen
 *
 * Created on January 25, 2016, 6:49 PM
 */

#ifndef STREAMPLAYER_H
#define STREAMPLAYER_H
//#include <MediaExtractor.h>
//#include <DecoderPlugin.h>


#include "Station.h"
#include "StreamIO.h"
#include <MediaFile.h>
#include <MediaIO.h>
#include <SoundPlayer.h>

// Notification Messages
#define MSG_PLAYER_STATE_CHANGED \
	'mPSC' // "player" = StreamPlayer*, "state" = int32(playState)
#define MSG_PLAYER_BUFFER_LEVEL \
	'mPBL' // "player" = StreamPlayer*, "level" = float percent buffer filled

class Station;

class StreamPlayer : BLocker
{
public:
	StreamPlayer(Station* station, BLooper* Notify = NULL);
	virtual ~StreamPlayer();
	inline status_t InitCheck() { return this->fInitStatus; }
	inline Station* GetStation() { return this->fStation; }
	status_t Play();
	status_t Stop();
	float Volume();
	void SetVolume(float volume);
	enum PlayState { InActive = -1, Stopped, Playing, Buffering };
	inline PlayState State() { return fState; }

private:
	PlayState fState;
	void setState(PlayState state);
	status_t fStatus, fInitStatus;
	BLooper* fNotify;
	int fNoNotifyCount;
	Station* fStation;
	StreamIO* fStream;
	BMediaFile* fMediaFile;
	BSoundPlayer* fPlayer;
	media_codec_info fCodecInfo;
	media_format fDecodedFormat;
	media_header fHeader;
	media_decode_info fInfo;
	BLocker fLock;
	bool fStopRequested;
	static status_t StartPlayThreadFunc(StreamPlayer* _this);
	static void GetDecodedChunk(void* cookie, void* buffer, size_t size,
		const media_raw_audio_format& format);
};

#endif /* STREAMPLAYER_H */
