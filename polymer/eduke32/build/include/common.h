//
// Definitions of common non-engine data structures/functions
// (and declarations of data appearing in both)
// for EDuke32 and Mapster32
//

#ifndef EDUKE32_COMMON_H_
#define EDUKE32_COMMON_H_


//// TYPES
struct strllist
{
    struct strllist *next;
    char *str;
};


//// EXTERN DECLS
extern struct strllist *CommandPaths, *CommandGrps;


//// FUNCTIONS
void G_AddGroup(const char *buffer);
void G_AddPath(const char *buffer);

#endif
