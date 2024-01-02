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
#ifndef _STREAM_IO_H
#define _STREAM_IO_H


#include <HttpRequest.h>
#include <Looper.h>
#include <UrlProtocolRoster.h>

#include "AdapterIO.h"

#include "override.h"

using namespace BPrivate::Network;

#define MSG_META_CHANGE 'META'


class Station;
class StreamIO;


typedef ssize_t (StreamIO::*DataFunc)(const char* data, size_t size, int next);


class DataFuncs {
public:
	DataFuncs()
		:
		fSize(10),
		fLast(-1)
	{
		for (int i = 0; i < fSize; i++)
			fFuncs[i] = NULL;
	};


	void
	Add(DataFunc f)
	{
		fFuncs[++fLast] = f;
	}


	void
	Add(DataFunc f, int before)
	{
		for (int i = fLast; i >= before; i--)
			fFuncs[i + 1] = fFuncs[i];
		fFuncs[before] = f;
		fLast++;
	};


	void
	Remove(int index)
	{
		for (int i = index; i < fLast; i++)
			fFuncs[i] = fFuncs[i + 1];
		fFuncs[fLast--] = NULL;
	};


			DataFunc			Item(int index) { return fFuncs[index]; };
			DataFunc			First() { return fFuncs[0]; };

private:
			DataFunc			fFuncs[10];
			int					fSize;
			int					fLast;
};


class StreamIO : public BAdapterIO, BUrlProtocolListener {
public:
								StreamIO(Station* station,
									BLooper* metaListener = NULL);
								~StreamIO();

			void				SetLimiter(size_t limit = 0);

// BAdapterIO
			status_t			Open() override;

private:
// BAdapterIO
			void				GetFlags(int32* flags) const override;

			ssize_t				Write(const void* buffer,
									size_t size) override;
			ssize_t				WriteAt(off_t position, const void* buffer,
									size_t size) override;
			ssize_t				ReadAt(off_t position, void* buffer,
									size_t size) override;

			status_t			SetSize(off_t size) override;

			bool				IsRunning() const override;

// BUrlProtocolListener
			void				HeadersReceived(BUrlRequest* request) override;
			void				RequestCompleted(BUrlRequest* request,
									bool success) override;
			void				DebugMessage(BUrlRequest* caller,
									BUrlProtocolDebugMessage type,
									const char* text) override;

//
			ssize_t				_DataWithMetaReceived(
									const char* data, size_t size, int next);
			ssize_t				_DataUnsyncedReceived(
									const char* data, size_t size, int next);
			ssize_t				_DataSyncedReceived(
									const char* data, size_t size, int next);
			void				_ProcessMeta();

private:
			enum				FrameSync {none, first, done};

			Station*			fStation;
			BHttpRequest*		fReq;
			thread_id			fReqThread;
			size_t				fUnsynched;
			size_t				fMetaInt;
			size_t				fMetaSize;
			off_t				fUntilMetaStart;
			off_t				fUntilMetaEnd;
			BLooper*			fMetaListener;
			FrameSync			fFrameSync;
			size_t				fLimit;
			size_t				fBuffered;

			DataFuncs			fDataFuncs;
			BInputAdapter*		fInputAdapter;

			const char*			fIcyName;
			bool				fIsMutable;
			char				fMetaBuffer[512];

};


#endif // _STREAM_IO_H
