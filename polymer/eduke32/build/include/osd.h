// On-screen display (ie. console)
// for the Build Engine
// by Jonathon Fowler (jf@jonof.id.au)

#ifndef osd_h_
#define osd_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "mutex.h"

typedef struct {
	int32_t numparms;
	const char *name;
	const char **parms;
	const char *raw;
} osdfuncparm_t;

const char *OSD_StripColors(char *out, const char *in);

#define OSDDEFAULTMAXLINES  128
#define OSDEDITLENGTH       512
#define OSDMINHISTORYDEPTH  32
#define OSDMAXHISTORYDEPTH  256
#define OSDBUFFERSIZE       32768
#define OSDDEFAULTROWS      20
#define OSDDEFAULTCOLS      60
#define OSDLOGCUTOFF        131072
#define OSDMAXSYMBOLS       512

enum cvartype_t
{
    CVAR_FLOAT       = 0x00000001,
    CVAR_INT         = 0x00000002,
    CVAR_UINT        = 0x00000004,
    CVAR_BOOL        = 0x00000008,
    CVAR_STRING      = 0x00000010,
    CVAR_DOUBLE      = 0x00000020,
    CVAR_LOCKED      = 0x00000040,
    CVAR_MULTI       = 0x00000080,
    CVAR_NOSAVE      = 0x00000100,
    CVAR_FUNCPTR     = 0x00000200,
    CVAR_RESTARTVID  = 0x00000400,
    CVAR_INVALIDATEALL  = 0x00000800,
    CVAR_INVALIDATEART = 0x00001000,
};

typedef struct _symbol
{
    const char *name;
    struct _symbol *next;

    const char *help;
    int32_t(*func)(const osdfuncparm_t *);
} symbol_t;

typedef struct
{
    const char *name;
    const char *desc;
    void *vptr;
    int32_t type;   // see cvartype_t
    int32_t min;
    int32_t max;    // for string, is the length
} cvar_t;

typedef struct
{
    cvar_t c;

    // default value for cvar, assigned when var is registered
    union 
    {
        int32_t i;
        uint32_t uint;
        float f;
        double d;
    } dval;
} osdcvar_t;

// version string
typedef struct
{
    char *buf;

    uint8_t len;
    uint8_t shade;
    uint8_t pal;
} osdstr_t;

// command prompt editing
typedef struct
{
    char *buf;// [OSDEDITLENGTH+1]; // editing buffer
    char *tmp;// [OSDEDITLENGTH+1]; // editing buffer temporary workspace

    int16_t len, pos; // length of characters and position of cursor in buffer
    int16_t start, end;
} osdedit_t;

// main text buffer
typedef struct
{
    // each character in the buffer also has a format byte containing shade and color
    char *buf;
    char *fmt;

    int32_t pos;      // position next character will be written at
    int32_t lines;    // total number of lines in buffer
    int32_t maxlines; // max lines in buffer
} osdtext_t;

// history display
typedef struct
{
    char *buf[OSDMAXHISTORYDEPTH];

    int32_t maxlines;   // max entries in buffer, ranges from OSDMINHISTORYDEPTH to OSDMAXHISTORYDEPTH
    int32_t pos;        // current buffer position
    int32_t lines;      // entries currently in buffer
    int32_t total;      // total number of entries
    int32_t exec;       // number of lines from the head of the history buffer to execute
} osdhist_t;

// active display parameters
typedef struct
{
    int32_t promptshade, promptpal;
    int32_t editshade, editpal;
    int32_t textshade, textpal;
    int32_t mode;

    int32_t rows; // # lines of the buffer that are visible
    int32_t cols; // width of onscreen display in text columns
    int32_t head; // topmost visible line number
} osddraw_t;

typedef struct
{
    BFILE *fp;
    int32_t cutoff;
    int32_t errors;
    int32_t lines;
} osdlog_t;

typedef struct
{
    osdtext_t text;
    osdedit_t editor;
    osdhist_t history;
    osddraw_t draw;
    osdstr_t verstr;

    uint32_t flags; // controls initialization, etc
    osdcvar_t *cvars;
    uint32_t numcvars;

    symbol_t *symbptrs[OSDMAXSYMBOLS];
    int32_t numsymbols;
    int32_t execdepth; // keeps track of nested execution
    mutex_t mutex;
    int32_t keycode;

    osdlog_t log;
} osdmain_t;

extern osdmain_t *osd;

extern BFILE *osdlog;
extern const char* osdlogfn;

enum osdflags_t 
{
//    OSD_INITIALIZED = 0x00000001,
    OSD_DRAW        = 0x00000002,
    OSD_CAPTURE     = 0x00000004,
    OSD_OVERTYPE    = 0x00000008,
    OSD_SHIFT       = 0x00000010,
    OSD_CTRL        = 0x00000020,
    OSD_CAPS        = 0x00000040
};

#define OSD_ALIAS     (int32_t (*)(const osdfuncparm_t*))0x1337
#define OSD_UNALIASED (int32_t (*)(const osdfuncparm_t*))0xDEAD

#define OSDCMD_OK	0
#define OSDCMD_SHOWHELP 1

int32_t OSD_ParsingScript(void);

int32_t OSD_OSDKey(void);
int32_t OSD_GetTextMode(void);
void OSD_SetTextMode(int32_t mode);

int32_t OSD_Exec(const char *szScript);

// Get shade and pal index from the OSD format buffer.
void OSD_GetShadePal(const char *ch, int32_t *shadeptr, int32_t *palptr);

int32_t OSD_GetCols(void);
int32_t OSD_IsMoving(void);
int32_t OSD_GetRowsCur(void);

// initializes things
void OSD_Init(void);

// cleans things up. these comments are retarded.
void OSD_Cleanup(void);

// sets the file to echo output to
void OSD_SetLogFile(const char *fn);

// sets the functions the OSD will call to interrogate the environment
void OSD_SetFunctions(
		void (*drawchar)(int32_t,int32_t,char,int32_t,int32_t),
		void (*drawstr)(int32_t,int32_t,const char*,int32_t,int32_t,int32_t),
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
void OSD_SetVersion(const char *version, int32_t shade, int32_t pal);

// shows or hides the onscreen display
void OSD_ShowDisplay(int32_t onf);

// draw the osd to the screen
void OSD_Draw(void);

// just like printf
void OSD_Printf(const char *fmt, ...) ATTRIBUTE((format(printf,1,2)));

// just like puts
void OSD_Puts(const char *str);

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
void OSD_WriteAliases(FILE *fp);
void OSD_WriteCvars(FILE *fp);

static inline void OSD_SetHistory(int32_t histIdx, const char *src)
{
    osd->history.buf[histIdx] = (char *)Xmalloc(OSDEDITLENGTH);
    Bstrncpyz(osd->history.buf[histIdx], src, OSDEDITLENGTH);
}

// these correspond to the Duke palettes, so they shouldn't really be here
// ...but I don't care

#define OSDTEXT_BLUE      "^00"
#define OSDTEXT_GOLD      "^07"
#define OSDTEXT_DARKRED   "^10"
#define OSDTEXT_GREEN     "^11"
#define OSDTEXT_GRAY      "^12"
#define OSDTEXT_DARKGRAY  "^13"
#define OSDTEXT_DARKGREEN "^14"
#define OSDTEXT_BROWN     "^15"
#define OSDTEXT_DARKBLUE  "^16"
#define OSDTEXT_RED       "^21"
#define OSDTEXT_YELLOW    "^23"

#define OSDTEXT_BRIGHT    "^S0"

#define OSD_ERROR OSDTEXT_DARKRED OSDTEXT_BRIGHT


extern int32_t osdcmd_restartvid(const osdfuncparm_t *parm);


extern void M32RunScript(const char *s);

#ifdef __cplusplus
}
#endif

#endif // osd_h_

