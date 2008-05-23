	//High-level (easy) picture loading function:
extern void kpzload (const char *, int *, intptr_t *, int *, int *);
	//Low-level PNG/JPG functions:
extern void kpgetdim (const char *, int, int *, int *);
extern int kprender (const char *, int, intptr_t, int, int, int, int, int);

	//ZIP functions:
extern int kzaddstack (const char *);
extern void kzuninit ();
extern int kzopen (const char *);
extern int kzread (void *, int);
extern int kzfilelength ();
extern int kzseek (int, int);
extern int kztell ();
extern int kzgetc ();
extern int kzeof ();
extern void kzclose ();

extern void kzfindfilestart (const char *); //pass wildcard string
extern int kzfindfile (char *); //you alloc buf, returns 1:found,0:~found

