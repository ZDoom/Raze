#ifndef __gtkbits_h__
#define __gtkbits_h__

extern void gtkbuild_init(int32_t *argc, char ***argv);
extern void gtkbuild_exit(int32_t r);
extern int32_t gtkbuild_msgbox(char *name, char *msg);
extern int32_t gtkbuild_ynbox(char *name, char *msg);

#endif
