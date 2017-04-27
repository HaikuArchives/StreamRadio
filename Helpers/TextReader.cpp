/* 
 * File:   TextReader.cpp
 * Author: user
 * 
 * Created on 27. Februar 2013, 17:27
 */

#include "TextReader.h"


TextReader::TextReader(BDataIO& stream) : BBufferedDataIO(stream) {
}

TextReader::~TextReader() {
    // free(buffer);
}

status_t TextReader::readLn(BString* s) {
    status_t status;
    char c;
    while ((status = this->Read(&c, 1)) == 1 && c != '\n') 
        if (c != '\r') s += c;
    
    return status;
}

status_t TextReader::writeLn(BString* s) {
    return this->Write((s + '\n')->LockBuffer(-1), s->CountChars()+1);
}
