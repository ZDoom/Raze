//
// Definitions of common non-engine data structures/functions
// (and declarations of data appearing in both)
// for EDuke32 and Mapster32
//

#ifndef EDUKE32_COMMON_H_
#define EDUKE32_COMMON_H_

#include "scriptfile.h"


//// TYPES
struct strllist
{
    struct strllist *next;
    char *str;
};

typedef struct
{
    const char *text;
    int32_t tokenid;
}
tokenlist;

enum
{
    T_EOF = -2,
    T_ERROR = -1,
};


//// EXTERN DECLS
extern struct strllist *CommandPaths, *CommandGrps;


//// FUNCTIONS
void G_AddGroup(const char *buffer);
void G_AddPath(const char *buffer);

int32_t getatoken(scriptfile *sf, const tokenlist *tl, int32_t ntokens);

int32_t check_file_exist(const char *fn);

#endif
