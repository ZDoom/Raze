// List of internally-known GRP files
#define numgrpfiles 7
struct grpfile {
	const char *name;
	int crcval;
	int size;
	int game;
	struct grpfile *next;
} grpfiles[numgrpfiles], *foundgrps;

#define GAMEDUKE 0
#define GAMENAM 1

int ScanGroups(void);
void FreeGroups(void);

