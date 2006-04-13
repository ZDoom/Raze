	//High-level (easy) picture loading function:
extern void kpzload (const char *, long *, long *, long *, long *);
	//Low-level PNG/JPG functions:
extern void kpgetdim (const char *, long, long *, long *);
extern long kprender (const char *, long, long, long, long, long, long, long);

	//ZIP functions:
extern long kzaddstack (const char *);
extern void kzuninit ();
extern long kzopen (const char *);
extern long kzread (void *, long);
extern long kzfilelength ();
extern long kzseek (long, long);
extern long kztell ();
extern long kzgetc ();
extern long kzeof ();
extern void kzclose ();

extern void kzfindfilestart (const char *); //pass wildcard string
extern long kzfindfile (char *); //you alloc buf, returns 1:found,0:~found

