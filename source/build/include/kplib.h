
#ifndef compat_h_
#include "compat.h"
#endif

#include "vfs.h"

typedef struct
{
    buildvfs_FILE fil;    //0:no file open, !=0:open file (either stand-alone or zip)
    int32_t comptyp; //0:raw data (can be ZIP or stand-alone), 8:PKZIP LZ77 *flate
    int32_t seek0;   //0:stand-alone file, !=0: start of zip compressed stream data
    int32_t compleng;//Global variable for compression FIFO
    int32_t comptell;//Global variable for compression FIFO
    int32_t leng;    //Uncompressed file size (bytes)
    int32_t pos;     //Current uncompressed relative file position (0<=pos<=leng)
    int32_t endpos;  //Temp global variable for kzread
    int32_t jmpplc;  //Store place where decompression paused
    int32_t i;       //For stand-alone/ZIP comptyp#0, this is like "uncomptell"
    //For ZIP comptyp#8&btype==0 "<64K store", this saves i state
    int32_t bfinal;  //LZ77 decompression state (for later calls)
} kzfilestate;

extern kzfilestate kzfs;

extern uint8_t toupperlookup[256];
static inline int32_t filnamcmp(const char *j, const char *i)
{
    // If we reach at the end of both strings, we are done
    while (*i && *j && (toupperlookup[*i] == toupperlookup[*j]))
        i++, j++;
    return *i != '\0' || *j != '\0';
}
