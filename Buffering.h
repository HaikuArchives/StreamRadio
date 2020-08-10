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
#ifndef _BUFFERING_H
#define _BUFFERING_H


/**
 * Simple class extending an integer to roll over when reaching boundaries
 */
class rotor {
public:
	rotor(int size, int initialValue = 0)
	{
		fMax = size - 1;
		fValue = initialValue;
	}


	rotor(rotor& other)
	{
		fMax = other.fMax;
		fValue = other.fValue;
	}


	operator int() { return fValue; }


	rotor& operator++()
	{
		if (fValue == fMax)
			fValue = 0;
		else
			fValue++;

		return *this;
	}


	rotor& operator++(int)
	{
		rotor* tmp = new rotor(*this);
		if (fValue == fMax)
			fValue = 0;
		else
			fValue++;

		return *tmp;
	}


	rotor& operator--()
	{
		if (fValue == 0)
			fValue = fMax;
		else
			fValue--;

		return *this;
	}


	rotor& operator--(int)
	{
		rotor* tmp = new rotor(*this);
		if (fValue == 0)
			fValue = fMax;
		else
			fValue--;

		return *tmp;
	}


	rotor& operator=(int i)
	{
		fValue = i % (fMax + 1);
	}


	int operator=(rotor& r)
	{
		return r.fValue;
	}


	int operator-(rotor& other)
	{
		return (fValue - other.fValue + fMax + 1) % (fMax + 1);
	}


	bool operator==(int i)
	{
		return fValue == i;
	}


	bool operator!=(int i)
	{
		return fValue != i;
	}


	bool operator<(int i)
	{
		return fValue < i;
	}


	bool operator>(int i)
	{
		return fValue > i;
	}

private:
	inline	void 				_SetValue(int i) { fValue = i; }
	inline	int					_GetValue() { return fValue; }

private:
			int					fValue;
			int					fMax;
};


class Buffer : public BLocker {
public:
#define limit(requested, max) (requested = (requested > max) ? max : requested)

	friend class BufferGroup;

	typedef enum {EMPTY, FILLING, FILLED, READING} bufferState;


	Buffer(size_t size, int index)
		:
		BLocker("Buffer"),
		fIndex(index),
		fUsed(0),
		fSize(size),
		fData((char*)malloc(size)),
		fState(EMPTY)
	{};

	~Buffer()
	{
		free(fData);
	}


	// Caller handles locking!
	inline	size_t				fillable() { return fSize - fUsed; }
	inline	char*				fillPos() { return fData + fUsed; }
	inline	bool				isFull() { return fUsed == fSize; }


	inline	bufferState			state() { return fState; }

	void
	setState(bufferState state)
	{
		if (state == EMPTY)
			fUsed = 0;
		fState = state;
	}


	inline	char*				data() { return fData; }
	inline	size_t				readable() { return fUsed; }


	size_t
	fill(char* data, size_t size)
	{
		if (limit(size, fillable())) {
			memcpy(fillPos(), data, size);
			fUsed += size;
		}

		return size;
	}


	size_t
	read(char* data, size_t size)
	{
		if (limit(size, readable()))
			memcpy(data, fData, size);

		return size;
	}

protected:
			bufferState			fState;
			size_t				fSize;
			size_t				fUsed;
			char*				fData;
			int					fIndex;
};


typedef Buffer* pBuffer;


class BufferGroup : public BLocker {
public:
	#define kNone -1

	typedef	bool (*LimitFunc)(void* cookie, float ratio, bool isOk);

	BufferGroup(int numBuffers, size_t size)
		:
		BLocker("BufferGroup"),
		firstEmpty(numBuffers, 0),
		firstFilled(numBuffers, kNone),
		fNumBuffers(numBuffers)
	{
		fBuffers = new pBuffer[numBuffers];
		for (int i = 0; i < numBuffers; i++)
			fBuffers[i] = new Buffer(size, i);
	}


	~BufferGroup()
	{
		for (int i = 0; i < fNumBuffers; i++)
			delete fBuffers[i];
		delete fBuffers;
	}


	inline int
	IndexOf(Buffer* buffer)
	{
		return buffer->fIndex;
	}


	Buffer*
	RequestForFilling(Buffer* previous = NULL)
	{
		Buffer* result = NULL;
		Lock();

		if (previous != NULL) {
			if (previous->CountLocks() == 0)
				previous->Lock();
			previous->fState = Buffer::FILLED;
			if (firstFilled == kNone)
				firstFilled = IndexOf(previous);
			previous->Unlock();
		}

		if (firstEmpty != kNone)
			result = fBuffers[firstEmpty++];
		result->fState = Buffer::FILLING;

		if (fBuffers[firstEmpty]->fState != Buffer::EMPTY)
			firstEmpty = kNone;

		Unlock();
		return result;
	}


	Buffer*
	RequestForReading(Buffer* previous = NULL)
	{
		Buffer* result = NULL;
		Lock();

		if (previous != NULL) {
			if (previous->CountLocks() == 0)
				previous->Lock();
			previous->setState(Buffer::EMPTY);
			if (firstEmpty == kNone)
				firstEmpty = IndexOf(previous);
			previous->Unlock();
		}

		if (firstFilled != kNone)
			result = fBuffers[firstFilled++];
		result->fState = Buffer::READING;

		if (fBuffers[firstFilled]->fState != Buffer::FILLED)
			firstFilled = kNone;

		Unlock();

		return result;
	}


	void
	ReturnBuffer(Buffer* previous)
	{
		if (previous->CountLocks() == 0)
			previous->Lock();

		if (previous->fState == Buffer::FILLING && previous->fUsed == 0)
			previous->setState(Buffer::EMPTY);

		Lock();

		switch (previous->fState) {
			case Buffer::READING:
			case Buffer::EMPTY:
			{
				previous->setState(Buffer::EMPTY);
				if (firstEmpty == kNone)
					firstEmpty = IndexOf(previous);
				break;
			}

			case Buffer::FILLING:
				previous->fState = Buffer::FILLED;
			case Buffer::FILLED:
			{
				if (firstFilled == kNone)
					firstFilled = previous->fIndex;
				break;
			}
		}

		Unlock();
		previous->Unlock();
	}


	size_t
	TotalUsed()
	{
		size_t result = 0;
		for (int i = 0; i < fNumBuffers; i++)
			Buffer* b = fBuffers[i];
		if (b->fState == Buffer::FILLED || b->fState == Buffer::FILLING)
			result += b->fUsed;

		return result;
	}


	size_t
	TotalCapacity()
	{
		return fNumBuffers * fBuffers[0]->fSize;
	}


	float
	FillRatio()
	{
		if (firstFilled == kNone)
			return 0.0f;
		else if (firstEmpty == kNone)
			return 1.0f;
		else
			return float(firstEmpty - firstFilled) / fNumBuffers;
	}

private:
			rotor				fFirstFilled;
			rotor				fFirstEmpty;
			int32				fNumBuffers;
			pBuffer*			fBuffers;
};


#endif // _BUFFERING_H
