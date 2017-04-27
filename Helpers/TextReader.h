/* 
 * File:   TextReader.h
 * Author: user
 *
 * Created on 27. Februar 2013, 17:27
 */

#ifndef TEXTREADER_H
#define	TEXTREADER_H
#include <support/BufferedDataIO.h>
#include <String.h>


class TextReader : public BBufferedDataIO {
public:
    TextReader(BDataIO& stream);
    virtual ~TextReader();
    status_t readLn(BString* s);
    status_t writeLn(BString* s);
private:
};

#endif	/* TEXTREADER_H */

