#ifndef __gtkbits_h__
#define __gtkbits_h__

extern void gtkbuild_init(int *argc, char ***argv);
extern void gtkbuild_exit(int r);
extern int gtkbuild_msgbox(char *name, char *msg);
extern int gtkbuild_ynbox(char *name, char *msg);
extern void gtkbuild_create_startwin(void);
extern void gtkbuild_settitle_startwin(const char *title);
extern void gtkbuild_puts_startwin(const char *str);
extern void gtkbuild_close_startwin(void);
extern void gtkbuild_update_startwin(void);
extern void *gtkbuild_get_app_icon(void);

#endif
