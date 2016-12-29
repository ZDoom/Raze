#ifndef gtkbits_h_
#define gtkbits_h_

#ifdef __cplusplus
extern "C" {
#endif

extern void gtkbuild_init(int32_t *argc, char ***argv);
extern void gtkbuild_exit(int32_t r);
extern int gtkbuild_msgbox(const char *name, const char *msg);
extern int gtkbuild_ynbox(const char *name, const char *msg);

#ifdef __cplusplus
}
#endif

#endif
