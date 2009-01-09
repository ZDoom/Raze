#ifndef __grpscan_h__
#define __grpscan_h__

// List of internally-known GRP files
#define numgrpfiles 8
struct grpfile {
	const char *name;
	int32_t crcval;
	int32_t size;
	int32_t game;
	struct grpfile *next;
} grpfiles[numgrpfiles], *foundgrps;

int32_t ScanGroups(void);
void FreeGroups(void);

#endif
