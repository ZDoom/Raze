#ifndef __grpscan_h__
#define __grpscan_h__

// List of internally-known GRP files
#define numgrpfiles 8
struct grpfile {
	const char *name;
	int crcval;
	int size;
	int game;
	struct grpfile *next;
} grpfiles[numgrpfiles], *foundgrps;

int ScanGroups(void);
void FreeGroups(void);

#endif
