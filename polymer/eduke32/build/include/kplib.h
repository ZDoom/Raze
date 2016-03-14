#ifdef __cplusplus
extern "C" {
#endif

#ifndef compat_h_
#include "compat.h"
#endif

typedef struct
{
    FILE *fil;    //0:no file open, !=0:open file (either stand-alone or zip)
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

	//High-level (easy) picture loading function:
extern void kpzdecode (int32_t, intptr_t *, int32_t *, int32_t *);
extern void kpzload (const char *, intptr_t *, int32_t *, int32_t *);
	//Low-level PNG/JPG functions:
extern void kpgetdim (const char *, int32_t, int32_t *, int32_t *);
extern int32_t kprender (const char *, int32_t, intptr_t, int32_t, int32_t, int32_t);

	//ZIP functions:
extern int32_t kzaddstack (const char *);
extern void kzuninit ();
extern intptr_t kzopen (const char *);
extern int32_t kzread (void *, int32_t);
extern int32_t kzseek (int32_t, int32_t);

static inline int32_t kztell(void) { return kzfs.fil ? kzfs.pos : -1; }
static inline int32_t kzeof(void) { return kzfs.fil ? kzfs.pos >= kzfs.leng : -1; }
static inline int32_t kzfilelength(void) { return kzfs.fil ? kzfs.leng : 0; }
static inline int32_t kzgetc(void) { char ch; return kzread(&ch, 1) ? ch : -1; }
static inline void kzclose(void) { MAYBE_FCLOSE_AND_NULL(kzfs.fil); }

extern void kzfindfilestart (const char *); //pass wildcard string
extern int32_t kzfindfile (char *); //you alloc buf, returns 1:found,0:~found

//like stricmp(st0,st1) except: '/' == '\'

extern char toupperlookup[256];
static inline int32_t filnamcmp(const char *j, const char *i)
{
    // If we reach at the end of both strings, we are done
    while (*i && *j && (toupperlookup[*i] == toupperlookup[*j]))
        i++, j++;
    return *i != '\0' || *j != '\0';
}
extern int32_t wildmatch(const char *match, const char *wild);

#ifdef __cplusplus
}
#endif
