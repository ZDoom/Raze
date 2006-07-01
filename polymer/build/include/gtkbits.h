#ifndef __gtkbits_h__
#define __gtkbits_h__

extern void gtkbuild_init(int *argc, char ***argv);
extern void gtkbuild_exit(int r);
extern int gtkbuild_msgbox(char *name, char *msg);
extern int gtkbuild_ynbox(char *name, char *msg);

#endif
