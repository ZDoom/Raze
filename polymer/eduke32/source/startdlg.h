// List of internally-known GRP files
#define numgrpfiles 6
struct grpfile {
	const char *name;
	int crcval;
	int size;
	struct grpfile *next;
} grpfiles[numgrpfiles], *foundgrps;

int ScanGroups(void);
void FreeGroups(void);

