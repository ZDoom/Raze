// On-screen display (ie. console)
// for the Build Engine
// by Jonathon Fowler (jonof@edgenetwk.com)

#ifndef __osd_h__
#define __osd_h__


typedef struct {
	int32_t numparms;
	const char *name;
	const char **parms;
	const char *raw;
} osdfuncparm_t;

typedef struct _symbol
{
    const char *name;
    struct _symbol *next;

    const char *help;
    int32_t (*func)(const osdfuncparm_t *);
} symbol_t;

symbol_t *symbols;

const char *stripcolorcodes(char *out, const char *in);

enum cvartypes
{
    CVAR_FLOAT = 1,
    CVAR_INT = 2,
    CVAR_UINT = 4,
    CVAR_BOOL = 8,
    CVAR_STRING = 16,
    CVAR_DOUBLE = 32,
    CVAR_NOMULTI = 128,
    CVAR_MULTI = 256,
    CVAR_NOSAVE = 512,
    CVAR_FUNCPTR = 1024,
};

typedef struct
{
    char *name;
    char *helpstr;
    void *var;
    int32_t type;       // 0 = integer, 1 = unsigned integer, 2 = boolean, 3 = string, |128 = not in multiplayer, |256 = update multi
    int32_t extra;      // for string, is the length
    int32_t min;
    int32_t max;
} cvar_t;


#define OSD_ALIAS 1337
#define OSD_UNALIASED 1338

#define OSDCMD_OK	0
#define OSDCMD_SHOWHELP 1

int32_t OSD_ParsingScript(void);

int32_t OSD_OSDKey(void);
int32_t OSD_GetTextMode(void);
void OSD_SetTextMode(int32_t mode);

int32_t OSD_Exec(const char *szScript);

char *OSD_GetFmt(char *ptr);
char *OSD_GetTextPtr(void);
char *OSD_GetFmtPtr(void);

int32_t OSD_GetCols(void);

// initializes things
void OSD_Init(void);

// sets the file to echo output to
void OSD_SetLogFile(char *fn);

// sets the functions the OSD will call to interrogate the environment
void OSD_SetFunctions(
		void (*drawchar)(int32_t,int32_t,char,int32_t,int32_t),
		void (*drawstr)(int32_t,int32_t,char*,int32_t,int32_t,int32_t),
		void (*drawcursor)(int32_t,int32_t,int32_t,int32_t),
		int32_t (*colwidth)(int32_t),
		int32_t (*rowheight)(int32_t),
		void (*clearbg)(int32_t,int32_t),
		int32_t (*gettime)(void),
		void (*onshow)(int32_t)
	);

// sets the parameters for presenting the text
void OSD_SetParameters(
		int32_t promptshade, int32_t promptpal,
		int32_t editshade, int32_t editpal,
		int32_t textshade, int32_t textpal
	);

// sets the scancode for the key which activates the onscreen display
void OSD_CaptureKey(int32_t sc);

// handles keyboard input when capturing input. returns 0 if key was handled
// or the scancode if it should be handled by the game.
int32_t  OSD_HandleScanCode(int32_t sc, int32_t press);
int32_t  OSD_HandleChar(char ch);

// handles the readjustment when screen resolution changes
void OSD_ResizeDisplay(int32_t w,int32_t h);

// captures and frees osd input
void OSD_CaptureInput(int32_t cap);

// sets the console version string
void OSD_SetVersionString(const char *version, int32_t shade, int32_t pal);

// shows or hides the onscreen display
void OSD_ShowDisplay(int32_t onf);

// draw the osd to the screen
void OSD_Draw(void);

// just like printf
void OSD_Printf(const char *fmt, ...);
#define printOSD OSD_Printf

// executes buffered commands
void OSD_DispatchQueued(void);

// executes a string
int32_t OSD_Dispatch(const char *cmd);

// registers a function
//   name = name of the function
//   help = a short help string
//   func = the entry point to the function
int32_t OSD_RegisterFunction(const char *name, const char *help, int32_t (*func)(const osdfuncparm_t*));

int32_t osdcmd_cvar_set(const osdfuncparm_t *parm);
int32_t OSD_RegisterCvar(const cvar_t *cvar);
void OSD_WriteCvars(FILE *fp);

// these correspond to the Duke palettes, so they shouldn't really be here
// ...but I don't care

#define OSDTEXT_BLUE "^00"
#define OSDTEXT_GOLD "^07"
#define OSDTEXT_DARKRED "^10"
#define OSDTEXT_GREEN "^11"
#define OSDTEXT_GRAY "^12"
#define OSDTEXT_DARKGRAY "^13"
#define OSDTEXT_DARKGREEN "^14"
#define OSDTEXT_BROWN "^15"
#define OSDTEXT_DARKBLUE "^16"
#define OSDTEXT_RED "^21"
#define OSDTEXT_YELLOW "^23"

#define OSDTEXT_BRIGHT "^S0"

#define OSD_ERROR OSDTEXT_DARKRED OSDTEXT_BRIGHT
#define TEXTSIZE 32768

#endif // __osd_h__

