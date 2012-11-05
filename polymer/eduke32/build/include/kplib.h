#ifdef EXTERNC
extern "C" {
#endif

	//High-level (easy) picture loading function:
extern void kpzload (const char *, intptr_t *, int32_t *, int32_t *, int32_t *);
	//Low-level PNG/JPG functions:
extern void kpgetdim (const char *, int32_t, int32_t *, int32_t *);
extern int32_t kprender (const char *, int32_t, intptr_t, int32_t, int32_t, int32_t, int32_t, int32_t);

	//ZIP functions:
extern int32_t kzaddstack (const char *);
extern void kzuninit ();
extern int32_t kzopen (const char *);
extern int32_t kzread (void *, int32_t);
extern int32_t kzfilelength ();
extern int32_t kzseek (int32_t, int32_t);
extern int32_t kztell ();
extern int32_t kzgetc ();
extern int32_t kzeof ();
extern void kzclose ();

extern void kzfindfilestart (const char *); //pass wildcard string
extern int32_t kzfindfile (char *); //you alloc buf, returns 1:found,0:~found


#ifdef EXTERNC
}
#endif
