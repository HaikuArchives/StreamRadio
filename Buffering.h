/* 
 * File:   Buffering.h
 * Author: Kai Niessen
 *
 * Created on January 31, 2016, 4:15 PM
 */

#ifndef BUFFERING_H
#define BUFFERING_H

/**
 * Simple class extending an integer to roll over when reaching boundaries 
 */
class rotor {
public:
    rotor(int size, int initialValue = 0) {
        fMax    = size-1;
        fValue  = initialValue;
    }
    rotor(rotor& other) {
        fMax = other.fMax;
        fValue = other.fValue;
    }
    operator int() { return fValue; }
    rotor& operator ++() { if (fValue==fMax) fValue=0; else fValue++; return *this;} 
    rotor& operator ++(int) { rotor* tmp=new rotor(*this); if (fValue==fMax) fValue=0; else fValue++; return *tmp;} 
    rotor& operator --() { if (fValue==0) fValue=fMax; else fValue--; return *this;} 
    rotor& operator --(int) { rotor* tmp=new rotor(*this); if (fValue==0) fValue=fMax; else fValue--; return *tmp;} 
    rotor& operator =(int i) { fValue = i % (fMax+1);}
    int operator =(rotor& r) { return r.fValue;}
    int operator -(rotor& other) {return (fValue-other.fValue + fMax + 1) % (fMax + 1);}
    bool operator ==(int i) { return fValue==i;}
    bool operator !=(int i) { return fValue!=i;}
    bool operator <(int i) { return fValue<i;}
    bool operator >(int i) { return fValue>i;}
private:
    inline void             setValue(int i) { fValue = i; }
    inline int              getValue() { return fValue; }
    int fValue;
    int fMax;
};

class Buffer : public BLocker {
public:
    #define                 limit(requested, max) (requested=(requested > max) ? max : requested)
    friend class BufferGroup;
    typedef enum {
        EMPTY,
        FILLING,
        FILLED,
        READING
    } bufferState;
                            Buffer(size_t size, int index)
                              : BLocker("buffer"), 
                                fIndex(index),
                                fUsed(0),
                                fSize(size),
                                fData((char*)malloc(size)),
                                fState(EMPTY)
                            { }
                            ~Buffer() { free(fData); }
    // Caller handles locking!
    inline size_t           fillable() { return fSize - fUsed; }
    inline char*            fillPos() { return fData + fUsed; }
    inline bool             isFull() { return fUsed == fSize; }
    inline bufferState      state() { return fState; }
    void                    setState(bufferState state) {
                                if (state == EMPTY) fUsed = 0;
                                fState = state;
                            }
    inline char*            data() { return fData; } 
    inline size_t           readable() { return fUsed; }
    size_t                  fill(char* data, size_t size) {
                                if (limit(size, fillable())) {
                                    memcpy(fillPos(), data, size);
                                    fUsed += size;
                                }
                                return size;
                            }
    size_t                  read(char* data, size_t size) {
                                if (limit(size, readable())) {
                                    memcpy(data, fData, size);
                                };
                                return size;
                            }
protected:
    bufferState             fState;
    size_t                  fSize;
    size_t                  fUsed;
    char*                   fData;
    int                     fIndex;
};
typedef Buffer* pBuffer;

class BufferGroup : public BLocker {
public:
    #define                 none -1
    typedef bool            (*LimitFunc)(void* cookie, float ratio, bool isOk);                                 
                            BufferGroup(int numBuffers, size_t size) 
                              : BLocker("buffers"),
                                firstEmpty(numBuffers, 0),
                                firstFilled(numBuffers, none),
                                fNumBuffers(numBuffers)
                            {
                                buffers     = new pBuffer[numBuffers];
                                for (int i = 0; i < numBuffers; i++)
                                    buffers[i] = new Buffer(size, i);
                            }
                            ~BufferGroup() {
                                for (int i = 0; i < fNumBuffers; i++) {
                                    delete buffers[i];
                                }
                                delete buffers;
                            }
    inline int              IndexOf(Buffer* buffer) {
                                return buffer->fIndex;
                            }
    Buffer*                 RequestForFilling(Buffer* previous = NULL) {
                                Buffer* result = NULL;
                                Lock();
                                if (previous) {
                                    if (previous->CountLocks() == 0) previous->Lock();
                                    previous->fState = Buffer::FILLED;
                                    if (firstFilled == none) {
                                        firstFilled = IndexOf(previous);
                                    }
                                    previous->Unlock();
                                }
                                if (firstEmpty != none) {
                                    result = buffers[firstEmpty++];
                                    result->fState = Buffer::FILLING;
                                    if (buffers[firstEmpty]->fState != Buffer::EMPTY)
                                        firstEmpty = none;
                                }
                                Unlock();
                                return result;
                            }
    Buffer*                 RequestForReading(Buffer* previous = NULL) {
                                Buffer* result = NULL;
                                Lock();
                                if (previous) {
                                    if (previous->CountLocks() == 0) previous->Lock();
                                    previous->setState(Buffer::EMPTY);
                                    if (firstEmpty == none)
                                        firstEmpty = IndexOf(previous);
                                    previous->Unlock();
                                } 
                                if (firstFilled != none) {
                                    result = buffers[firstFilled++];
                                    result->fState = Buffer::READING;
                                    if (buffers[firstFilled]->fState != Buffer::FILLED)
                                       firstFilled = none;
                                }
                                Unlock();
                                return result;
                            }
    void                    ReturnBuffer(Buffer* previous) {
                                if (previous->CountLocks() == 0) previous->Lock();
                                if (previous->fState == Buffer::FILLING && previous->fUsed == 0)
                                    previous->setState(Buffer::EMPTY);

                                Lock();
                                switch (previous->fState) {
                                    case Buffer::READING :
                                    case Buffer::EMPTY:
                                        previous->setState(Buffer::EMPTY);
                                        if (firstEmpty == none) firstEmpty = IndexOf(previous);
                                        break;
                                    case Buffer::FILLING:
                                        previous->fState = Buffer::FILLED;
                                    case Buffer::FILLED:
                                        if (firstFilled == none) firstFilled = previous->fIndex;
                                        break;
                                }
                                Unlock();
                                previous->Unlock();
                            }
    size_t                  TotalUsed() {
                                size_t result = 0;
                                for (int i=0; i<fNumBuffers; i++) {
                                    Buffer* b=buffers[i]; 
                                    if (b->fState == Buffer::FILLED || b->fState == Buffer::FILLING) {
                                        result += b->fUsed; 
                                    }
                                }
                                return result;
                            }
    size_t                  TotalCapacity() {
                                return fNumBuffers * buffers[0]->fSize;
                            }
    float                   FillRatio() {
                                if (firstFilled == none) 
                                    return 0.0f;
                                else if (firstEmpty == none)
                                    return 1.0f;
                                else
                                    return float(firstEmpty - firstFilled) / fNumBuffers;  
                            }
private:
    int                     fNumBuffers;
    pBuffer*                buffers;
    rotor                   firstEmpty;
    rotor                   firstFilled;
};

#endif /* BUFFERING_H */

