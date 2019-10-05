#pragma once

// Just to let the ZDoom-based code print to the Build console without changing it all

#if defined __GNUC__ || defined __clang__
# define ATTRIBUTE(attrlist) __attribute__(attrlist)
#else
# define ATTRIBUTE(attrlist)
#endif

void OSD_Printf(const char *fmt, ...) ATTRIBUTE((format(printf,1,2)));
#define Printf OSD_Printf	

#if 0
#define TEXTCOLOR_ESCAPE		'\034'
#define TEXTCOLOR_ESCAPESTR		"\034"

#define TEXTCOLOR_BRICK			"\034A"
#define TEXTCOLOR_TAN			"\034B"
#define TEXTCOLOR_GRAY			"\034C"
#define TEXTCOLOR_GREY			"\034C"
#define TEXTCOLOR_GREEN			"\034D"
#define TEXTCOLOR_BROWN			"\034E"
#define TEXTCOLOR_GOLD			"\034F"
#define TEXTCOLOR_RED			"\034G"
#define TEXTCOLOR_BLUE			"\034H"
#define TEXTCOLOR_ORANGE		"\034I"
#define TEXTCOLOR_WHITE			"\034J"
#define TEXTCOLOR_YELLOW		"\034K"
#define TEXTCOLOR_UNTRANSLATED	"\034L"
#define TEXTCOLOR_BLACK			"\034M"
#define TEXTCOLOR_LIGHTBLUE		"\034N"
#define TEXTCOLOR_CREAM			"\034O"
#define TEXTCOLOR_OLIVE			"\034P"
#define TEXTCOLOR_DARKGREEN		"\034Q"
#define TEXTCOLOR_DARKRED		"\034R"
#define TEXTCOLOR_DARKBROWN		"\034S"
#define TEXTCOLOR_PURPLE		"\034T"
#define TEXTCOLOR_DARKGRAY		"\034U"
#define TEXTCOLOR_CYAN			"\034V"
#define TEXTCOLOR_ICE			"\034W"
#define TEXTCOLOR_FIRE			"\034X"
#define TEXTCOLOR_SAPPHIRE		"\034Y"
#define TEXTCOLOR_TEAL			"\034Z"

#define TEXTCOLOR_NORMAL		"\034-"
#define TEXTCOLOR_BOLD			"\034+"

#define TEXTCOLOR_CHAT			"\034*"
#define TEXTCOLOR_TEAMCHAT		"\034!" 

#else
	
#define TEXTCOLOR_BRICK			""
#define TEXTCOLOR_TAN			""
#define TEXTCOLOR_GRAY			""
#define TEXTCOLOR_GREY			""
#define TEXTCOLOR_GREEN			""
#define TEXTCOLOR_BROWN			""
#define TEXTCOLOR_GOLD			""
#define TEXTCOLOR_RED			""
#define TEXTCOLOR_BLUE			""
#define TEXTCOLOR_ORANGE		""
#define TEXTCOLOR_WHITE			""
#define TEXTCOLOR_YELLOW		""
#define TEXTCOLOR_UNTRANSLATED	""
#define TEXTCOLOR_BLACK			""
#define TEXTCOLOR_LIGHTBLUE		""
#define TEXTCOLOR_CREAM			""
#define TEXTCOLOR_OLIVE			""
#define TEXTCOLOR_DARKGREEN		""
#define TEXTCOLOR_DARKRED		""
#define TEXTCOLOR_DARKBROWN		""
#define TEXTCOLOR_PURPLE		""
#define TEXTCOLOR_DARKGRAY		""
#define TEXTCOLOR_CYAN			""
#define TEXTCOLOR_ICE			""
#define TEXTCOLOR_FIRE			""
#define TEXTCOLOR_SAPPHIRE		""
#define TEXTCOLOR_TEAL			""

#define TEXTCOLOR_NORMAL		""
#define TEXTCOLOR_BOLD			""

#define TEXTCOLOR_CHAT			""
#define TEXTCOLOR_TEAMCHAT		"" 

#endif