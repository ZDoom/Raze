
// constants for A_PlaySound
enum ESoundFlags
{
	CHAN_AUTO = 0,
	CHAN_WEAPON = 1,
	CHAN_VOICE = 2,
	CHAN_ITEM = 3,
	CHAN_BODY = 4,
	CHAN_5 = 5,
	CHAN_6 = 6,
	CHAN_7 = 7,
	
	// modifier flags
	CHAN_LISTENERZ = 8,
	CHAN_MAYBE_LOCAL = 16,
	CHAN_UI = 32,
	CHAN_NOPAUSE = 64,
	CHAN_LOOP = 256,
	CHAN_PICKUP = (CHAN_ITEM|CHAN_MAYBE_LOCAL), // Do not use this with A_StartSound! It would not do what is expected.
	CHAN_NOSTOP = 4096,
	CHAN_OVERLAP = 8192,

	// Same as above, with an F appended to allow better distinction of channel and channel flags.
	CHANF_DEFAULT = 0,	// just to make the code look better and avoid literal 0's.
	CHANF_LISTENERZ = 8,
	CHANF_MAYBE_LOCAL = 16,
	CHANF_UI = 32,
	CHANF_NOPAUSE = 64,
	CHANF_LOOP = 256,
	CHANF_NOSTOP = 4096,
	CHANF_OVERLAP = 8192,
	CHANF_LOCAL = 16384,


	CHANF_LOOPING = CHANF_LOOP | CHANF_NOSTOP, // convenience value for replicating the old 'looping' boolean.

};

// sound attenuation values
const ATTN_NONE = 0;
const ATTN_NORM = 1;
const ATTN_IDLE = 1.001;
const ATTN_STATIC = 3;


enum ERenderStyle
{
	STYLE_None,				// Do not draw
	STYLE_Normal,			// Normal; just copy the image to the screen
	STYLE_Fuzzy,			// Draw silhouette using "fuzz" effect
	STYLE_SoulTrans,		// Draw translucent with amount in r_transsouls
	STYLE_OptFuzzy,			// Draw as fuzzy or translucent, based on user preference
	STYLE_Stencil,			// Fill image interior with alphacolor
	STYLE_Translucent,		// Draw translucent
	STYLE_Add,				// Draw additive
	STYLE_Shaded,			// Treat patch data as alpha values for alphacolor
	STYLE_TranslucentStencil,
	STYLE_Shadow,
	STYLE_Subtract,			// Actually this is 'reverse subtract' but this is what normal people would expect by 'subtract'.
	STYLE_AddStencil,		// Fill image interior with alphacolor
	STYLE_AddShaded,		// Treat patch data as alpha values for alphacolor
	STYLE_Multiply,			// Multiply source with destination (HW renderer only.)
	STYLE_InverseMultiply,	// Multiply source with inverse of destination (HW renderer only.)
	STYLE_ColorBlend,		// Use color intensity as transparency factor
	STYLE_Source,			// No blending (only used internally)
	STYLE_ColorAdd,			// Use color intensity as transparency factor and blend additively.

};


enum ETranslationTable
{
	TRANSLATION_Invalid,
	TRANSLATION_Basepalette,
	TRANSLATION_Remap,
};

enum EGameState
{
	GS_LEVEL,
	GS_INTERMISSION,
	GS_FINALE,
	GS_DEMOSCREEN,
	GS_FULLCONSOLE,
	GS_HIDECONSOLE,
	GS_STARTUP,
	GS_TITLELEVEL,
}

const TEXTCOLOR_BRICK			= "\034A";
const TEXTCOLOR_TAN				= "\034B";
const TEXTCOLOR_GRAY			= "\034C";
const TEXTCOLOR_GREY			= "\034C";
const TEXTCOLOR_GREEN			= "\034D";
const TEXTCOLOR_BROWN			= "\034E";
const TEXTCOLOR_GOLD			= "\034F";
const TEXTCOLOR_RED				= "\034G";
const TEXTCOLOR_BLUE			= "\034H";
const TEXTCOLOR_ORANGE			= "\034I";
const TEXTCOLOR_WHITE			= "\034J";
const TEXTCOLOR_YELLOW			= "\034K";
const TEXTCOLOR_UNTRANSLATED	= "\034L";
const TEXTCOLOR_BLACK			= "\034M";
const TEXTCOLOR_LIGHTBLUE		= "\034N";
const TEXTCOLOR_CREAM			= "\034O";
const TEXTCOLOR_OLIVE			= "\034P";
const TEXTCOLOR_DARKGREEN		= "\034Q";
const TEXTCOLOR_DARKRED			= "\034R";
const TEXTCOLOR_DARKBROWN		= "\034S";
const TEXTCOLOR_PURPLE			= "\034T";
const TEXTCOLOR_DARKGRAY		= "\034U";
const TEXTCOLOR_CYAN			= "\034V";
const TEXTCOLOR_ICE				= "\034W";
const TEXTCOLOR_FIRE			= "\034X";
const TEXTCOLOR_SAPPHIRE		= "\034Y";
const TEXTCOLOR_TEAL			= "\034Z";

const TEXTCOLOR_NORMAL			= "\034-";
const TEXTCOLOR_BOLD			= "\034+";

const TEXTCOLOR_CHAT			= "\034*";
const TEXTCOLOR_TEAMCHAT		= "\034!";


enum EMonospacing
{
	Mono_Off = 0,
	Mono_CellLeft = 1,
	Mono_CellCenter = 2,
	Mono_CellRight = 3
};

enum EPrintLevel
{
	PRINT_LOW,		// pickup messages
	PRINT_MEDIUM,	// death messages
	PRINT_HIGH,		// critical messages
	PRINT_CHAT,		// chat messages
	PRINT_TEAMCHAT,	// chat messages from a teammate
	PRINT_LOG,		// only to logfile
	PRINT_BOLD = 200,				// What Printf_Bold used
	PRINT_TYPES = 1023,		// Bitmask.
	PRINT_NONOTIFY = 1024,	// Flag - do not add to notify buffer
	PRINT_NOLOG = 2048,		// Flag - do not print to log file
};
