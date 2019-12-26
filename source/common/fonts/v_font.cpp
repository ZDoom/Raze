/*
** v_font.cpp
** Font management
**
**---------------------------------------------------------------------------
** Copyright 1998-2016 Randy Heit
** Copyright 2005-2019 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

// HEADER FILES ------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "templates.h"
#include "m_swap.h"
#include "v_font.h"
#include "cmdlib.h"
#include "sc_man.h"
#include "v_text.h"
#include "image.h"
#include "utf8.h"

#include "m_png.h"
#include "printf.h"
#include "filesystem.h"

#include "fontinternals.h"

// MACROS ------------------------------------------------------------------

#define DEFAULT_LOG_COLOR	PalEntry(223,223,223)

// TYPES -------------------------------------------------------------------
int V_GetColor(const char* cstr);

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------
FFont* SmallFont, * SmallFont2, * BigFont, * BigUpper, * ConFont, * IntermissionFont, * NewConsoleFont, * NewSmallFont, * CurrentConsoleFont, * OriginalSmallFont, * AlternativeSmallFont, * OriginalBigFont;

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static int TranslationMapCompare (const void *a, const void *b);
//void UpdateGenericUI(bool cvar);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern int PrintColors[];

// PUBLIC DATA DEFINITIONS -------------------------------------------------

FFont *FFont::FirstFont = nullptr;
int NumTextColors;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

TArray<TranslationParm> TranslationParms[2];
TArray<TranslationMap> TranslationLookup;
TArray<PalEntry> TranslationColors;

// CODE --------------------------------------------------------------------

FFont *V_GetFont(const char *name, const char *fontlumpname)
{
	FFont *font = FFont::FindFont (name);
	if (font == nullptr)
	{
		auto lumpy = fileSystem.OpenFileReader(fontlumpname, 0);
		if (!lumpy.isOpen()) return nullptr;
		uint32_t head;
		lumpy.Read (&head, 4);
		if ((head & MAKE_ID(255,255,255,0)) == MAKE_ID('F','O','N',0) ||
			head == MAKE_ID(0xE1,0xE6,0xD5,0x1A))
		{
			FFont *CreateSingleLumpFont (const char *fontname, const char *lump);
			lumpy.Close();
			return CreateSingleLumpFont (name, fontlumpname);
		}
	}
	return font;
}

//==========================================================================
//
// V_InitCustomFonts
//
// Initialize a list of custom multipatch fonts
//
//==========================================================================

void V_InitCustomFonts()
{
#if 0
	FScanner sc;
	FTexture *lumplist[256];
	bool notranslate[256];
	bool donttranslate;
	FString namebuffer, templatebuf;
	int i;
	int llump,lastlump=0;
	int format;
	int start;
	int first;
	int count;
	int spacewidth;
	int kerning;
	char cursor = '_';

	while ((llump = Wads.FindLump ("FONTDEFS", &lastlump)) != -1)
	{
		sc.OpenLumpNum(llump);
		while (sc.GetString())
		{
			memset (lumplist, 0, sizeof(lumplist));
			memset (notranslate, 0, sizeof(notranslate));
			donttranslate = false;
			namebuffer = sc.String;
			format = 0;
			start = 33;
			first = 33;
			count = 223;
			spacewidth = -1;
			kerning = 0;

			sc.MustGetStringName ("{");
			while (!sc.CheckString ("}"))
			{
				sc.MustGetString();
				if (sc.Compare ("TEMPLATE"))
				{
					if (format == 2) goto wrong;
					sc.MustGetString();
					templatebuf = sc.String;
					format = 1;
				}
				else if (sc.Compare ("BASE"))
				{
					if (format == 2) goto wrong;
					sc.MustGetNumber();
					start = sc.Number;
					format = 1;
				}
				else if (sc.Compare ("FIRST"))
				{
					if (format == 2) goto wrong;
					sc.MustGetNumber();
					first = sc.Number;
					format = 1;
				}
				else if (sc.Compare ("COUNT"))
				{
					if (format == 2) goto wrong;
					sc.MustGetNumber();
					count = sc.Number;
					format = 1;
				}
				else if (sc.Compare ("CURSOR"))
				{
					sc.MustGetString();
					cursor = sc.String[0];
				}
				else if (sc.Compare ("SPACEWIDTH"))
				{
					if (format == 2) goto wrong;
					sc.MustGetNumber();
					spacewidth = sc.Number;
					format = 1;
				}
				else if (sc.Compare("DONTTRANSLATE"))
				{
					donttranslate = true;
				}
				else if (sc.Compare ("NOTRANSLATION"))
				{
					if (format == 1) goto wrong;
					while (sc.CheckNumber() && !sc.Crossed)
					{
						if (sc.Number >= 0 && sc.Number < 256)
							notranslate[sc.Number] = true;
					}
					format = 2;
				}
				else if (sc.Compare("KERNING"))
				{
					sc.MustGetNumber();
					kerning = sc.Number;
				}
				else
				{
					if (format == 1) goto wrong;
					FTexture **p = &lumplist[*(unsigned char*)sc.String];
					sc.MustGetString();
					FTextureID texid = TexMan.CheckForTexture(sc.String, ETextureType::MiscPatch);
					if (texid.Exists())
					{
						*p = TexMan.GetTexture(texid);
					}
					else if (Wads.GetLumpFile(sc.LumpNum) >= Wads.GetIwadNum())
					{
						// Print a message only if this isn't in zdoom.pk3
						sc.ScriptMessage("%s: Unable to find texture in font definition for %s", sc.String, namebuffer.GetChars());
					}
					format = 2;
				}
			}
			if (format == 1)
			{
				FFont *fnt = new FFont (namebuffer, templatebuf, nullptr, first, count, start, llump, spacewidth, donttranslate);
				fnt->SetCursor(cursor);
				fnt->SetKerning(kerning);
			}
			else if (format == 2)
			{
				for (i = 0; i < 256; i++)
				{
					if (lumplist[i] != nullptr)
					{
						first = i;
						break;
					}
				}
				for (i = 255; i >= 0; i--)
				{
					if (lumplist[i] != nullptr)
					{
						count = i - first + 1;
						break;
					}
				}
				if (count > 0)
				{
					FFont *CreateSpecialFont (const char *name, int first, int count, FTexture **lumplist, const bool *notranslate, int lump, bool donttranslate);
					FFont *fnt = CreateSpecialFont (namebuffer, first, count, &lumplist[first], notranslate, llump, donttranslate);
					fnt->SetCursor(cursor);
					fnt->SetKerning(kerning);
				}
			}
			else goto wrong;
		}
		sc.Close();
	}
	return;

wrong:
	sc.ScriptError ("Invalid combination of properties in font '%s'", namebuffer.GetChars());
#endif
}

//==========================================================================
//
// V_GetColorFromString
//
// Passed a string of the form "#RGB", "#RRGGBB", "R G B", or "RR GG BB",
// returns a number representing that color. If palette is non-NULL, the
// index of the best match in the palette is returned, otherwise the
// RRGGBB value is returned directly.
//
//==========================================================================

int V_GetColor(const char* cstr)
{
	int c[3], i, p;
	char val[3];

	val[2] = '\0';

	// Check for HTML-style #RRGGBB or #RGB color string
	if (cstr[0] == '#')
	{
		size_t len = strlen(cstr);

		if (len == 7)
		{
			// Extract each eight-bit component into c[].
			for (i = 0; i < 3; ++i)
			{
				val[0] = cstr[1 + i * 2];
				val[1] = cstr[2 + i * 2];
				c[i] = ParseHex(val, nullptr);
			}
		}
		else if (len == 4)
		{
			// Extract each four-bit component into c[], expanding to eight bits.
			for (i = 0; i < 3; ++i)
			{
				val[1] = val[0] = cstr[1 + i];
				c[i] = ParseHex(val, nullptr);
			}
		}
		else
		{
			// Bad HTML-style; pretend it's black.
			c[2] = c[1] = c[0] = 0;
		}
	}
	else
	{
		if (strlen(cstr) == 6)
		{
			char* p;
			int color = strtol(cstr, &p, 16);
			if (*p == 0)
			{
				// RRGGBB string
				c[0] = (color & 0xff0000) >> 16;
				c[1] = (color & 0xff00) >> 8;
				c[2] = (color & 0xff);
			}
			else goto normal;
		}
		else
		{
		normal:
			// Treat it as a space-delimited hexadecimal string
			for (i = 0; i < 3; ++i)
			{
				// Skip leading whitespace
				while (*cstr <= ' ' && *cstr != '\0')
				{
					cstr++;
				}
				// Extract a component and convert it to eight-bit
				for (p = 0; *cstr > ' '; ++p, ++cstr)
				{
					if (p < 2)
					{
						val[p] = *cstr;
					}
				}
				if (p == 0)
				{
					c[i] = 0;
				}
				else
				{
					if (p == 1)
					{
						val[1] = val[0];
					}
					c[i] = ParseHex(val, nullptr);
				}
			}
		}
	}
	return MAKERGB(c[0], c[1], c[2]);
}

//==========================================================================
//
// V_InitFontColors
//
// Reads the list of color translation definitions into memory.
//
//==========================================================================

void V_InitFontColors ()
{
	TArray<FName> names;
	int lump, lastlump = 0;
	TranslationParm tparm = { 0, 0, {0}, {0} };	// Silence GCC (for real with -Wextra )
	TArray<TranslationParm> parms;
	TArray<TempParmInfo> parminfo;
	TArray<TempColorInfo> colorinfo;
	int c, parmchoice;
	TempParmInfo info;
	TempColorInfo cinfo;
	PalEntry logcolor;
	unsigned int i, j;
	int k, index;

	info.Index = -1;

	TranslationParms[0].Clear();
	TranslationParms[1].Clear();
	TranslationLookup.Clear();
	TranslationColors.Clear();

	while ((lump = fileSystem.Iterate("engine/textcolors.txt", &lastlump)) != -1)
	{
		FScanner sc(lump);
		while (sc.GetString())
		{
			names.Clear();

			logcolor = DEFAULT_LOG_COLOR;

			// Everything until the '{' is considered a valid name for the
			// color range.
			names.Push (sc.String);
			while (sc.MustGetString(), !sc.Compare ("{"))
			{
				if (names[0] == NAME_Untranslated)
				{
					sc.ScriptError ("The \"untranslated\" color may not have any other names");
				}
				names.Push (sc.String);
			}

			parmchoice = 0;
			info.StartParm[0] = parms.Size();
			info.StartParm[1] = 0;
			info.ParmLen[1] = info.ParmLen[0] = 0;
			tparm.RangeEnd = tparm.RangeStart = -1;

			while (sc.MustGetString(), !sc.Compare ("}"))
			{
				if (sc.Compare ("Console:"))
				{
					if (parmchoice == 1)
					{
						sc.ScriptError ("Each color may only have one set of console ranges");
					}
					parmchoice = 1;
					info.StartParm[1] = parms.Size();
					info.ParmLen[0] = info.StartParm[1] - info.StartParm[0];
					tparm.RangeEnd = tparm.RangeStart = -1;
				}
				else if (sc.Compare ("Flat:"))
				{
					sc.MustGetString();
					logcolor = V_GetColor (sc.String);
				}
				else
				{
					// Get first color
					c = V_GetColor (sc.String);
					tparm.Start[0] = RPART(c);
					tparm.Start[1] = GPART(c);
					tparm.Start[2] = BPART(c);

					// Get second color
					sc.MustGetString();
					c = V_GetColor (sc.String);
					tparm.End[0] = RPART(c);
					tparm.End[1] = GPART(c);
					tparm.End[2] = BPART(c);

					// Check for range specifier
					if (sc.CheckNumber())
					{
						if (tparm.RangeStart == -1 && sc.Number != 0)
						{
							sc.ScriptError ("The first color range must start at position 0");
						}
						if (sc.Number < 0 || sc.Number > 256)
						{
							sc.ScriptError ("The color range must be within positions [0,256]");
						}
						if (sc.Number <= tparm.RangeEnd)
						{
							sc.ScriptError ("The color range must not start before the previous one ends");
						}
						tparm.RangeStart = sc.Number;

						sc.MustGetNumber();
						if (sc.Number < 0 || sc.Number > 256)
						{
							sc.ScriptError ("The color range must be within positions [0,256]");
						}
						if (sc.Number <= tparm.RangeStart)
						{
							sc.ScriptError ("The color range end position must be larger than the start position");
						}
						tparm.RangeEnd = sc.Number;
					}
					else
					{
						tparm.RangeStart = tparm.RangeEnd + 1;
						tparm.RangeEnd = 256;
						if (tparm.RangeStart >= tparm.RangeEnd)
						{
							sc.ScriptError ("The color has too many ranges");
						}
					}
					parms.Push (tparm);
				}
			}
			info.ParmLen[parmchoice] = parms.Size() - info.StartParm[parmchoice];
			if (info.ParmLen[0] == 0)
			{
				if (names[0] != NAME_Untranslated)
				{
					sc.ScriptError ("There must be at least one normal range for a color");
				}
			}
			else
			{
				if (names[0] == NAME_Untranslated)
				{
					sc.ScriptError ("The \"untranslated\" color must be left undefined");
				}
			}
			if (info.ParmLen[1] == 0 && names[0] != NAME_Untranslated)
			{ // If a console translation is unspecified, make it white, since the console
			  // font has no color information stored with it.
				tparm.RangeStart = 0;
				tparm.RangeEnd = 256;
				tparm.Start[2] = tparm.Start[1] = tparm.Start[0] = 0;
				tparm.End[2] = tparm.End[1] = tparm.End[0] = 255;
				info.StartParm[1] = parms.Push (tparm);
				info.ParmLen[1] = 1;
			}
			cinfo.ParmInfo = parminfo.Push (info);
			// Record this color information for each name it goes by
			for (i = 0; i < names.Size(); ++i)
			{
				// Redefine duplicates in-place
				for (j = 0; j < colorinfo.Size(); ++j)
				{
					if (colorinfo[j].Name == names[i])
					{
						colorinfo[j].ParmInfo = cinfo.ParmInfo;
						colorinfo[j].LogColor = logcolor;
						break;
					}
				}
				if (j == colorinfo.Size())
				{
					cinfo.Name = names[i];
					cinfo.LogColor = logcolor;
					colorinfo.Push (cinfo);
				}
			}
		}
	}
	// Make permananent copies of all the color information we found.
	for (i = 0, index = 0; i < colorinfo.Size(); ++i)
	{
		TranslationMap tmap;
		TempParmInfo *pinfo;

		tmap.Name = colorinfo[i].Name;
		pinfo = &parminfo[colorinfo[i].ParmInfo];
		if (pinfo->Index < 0)
		{
			// Write out the set of remappings for this color.
			for (k = 0; k < 2; ++k)
			{
				for (j = 0; j < pinfo->ParmLen[k]; ++j)
				{
					TranslationParms[k].Push (parms[pinfo->StartParm[k] + j]);
				}
			}
			TranslationColors.Push (colorinfo[i].LogColor);
			pinfo->Index = index++;
		}
		tmap.Number = pinfo->Index;
		TranslationLookup.Push (tmap);
	}
	// Leave a terminating marker at the ends of the lists.
	tparm.RangeStart = -1;
	TranslationParms[0].Push (tparm);
	TranslationParms[1].Push (tparm);
	// Sort the translation lookups for fast binary searching.
	qsort (&TranslationLookup[0], TranslationLookup.Size(), sizeof(TranslationLookup[0]), TranslationMapCompare);

	NumTextColors = index;
	assert (NumTextColors >= NUM_TEXT_COLORS);
}

//==========================================================================
//
// TranslationMapCompare
//
//==========================================================================

static int TranslationMapCompare (const void *a, const void *b)
{
	return int(((const TranslationMap *)a)->Name) - int(((const TranslationMap *)b)->Name);
}

//==========================================================================
//
// V_FindFontColor
//
// Returns the color number for a particular named color range.
//
//==========================================================================

EColorRange V_FindFontColor (FName name)
{
	int min = 0, max = TranslationLookup.Size() - 1;

	while (min <= max)
	{
		unsigned int mid = (min + max) / 2;
		const TranslationMap *probe = &TranslationLookup[mid];
		if (probe->Name == name)
		{
			return EColorRange(probe->Number);
		}
		else if (probe->Name < name)
		{
			min = mid + 1;
		}
		else
		{
			max = mid - 1;
		}
	}
	return CR_UNTRANSLATED;
}

//==========================================================================
//
// V_LogColorFromColorRange
//
// Returns the color to use for text in the startup/error log window.
//
//==========================================================================

PalEntry V_LogColorFromColorRange (EColorRange range)
{
	if ((unsigned int)range >= TranslationColors.Size())
	{ // Return default color
		return DEFAULT_LOG_COLOR;
	}
	return TranslationColors[range];
}

//==========================================================================
//
// V_ParseFontColor
//
// Given a pointer to a color identifier (presumably just after a color
// escape character), return the color it identifies and advances
// color_value to just past it.
//
//==========================================================================

EColorRange V_ParseFontColor (const uint8_t *&color_value, int normalcolor, int boldcolor)
{
	const uint8_t *ch = color_value;
	int newcolor = *ch++;

	if (newcolor == '-')			// Normal
	{
		newcolor = normalcolor;
	}
	else if (newcolor == '+')		// Bold
	{
		newcolor = boldcolor;
	}
	else if (newcolor == '[')		// Named
	{
		const uint8_t *namestart = ch;
		while (*ch != ']' && *ch != '\0')
		{
			ch++;
		}
		FName rangename((const char *)namestart, int(ch - namestart), true);
		if (*ch != '\0')
		{
			ch++;
		}
		newcolor = V_FindFontColor (rangename);
	}
	else if (newcolor >= 'A' && newcolor < NUM_TEXT_COLORS + 'A')	// Standard, uppercase
	{
		newcolor -= 'A';
	}
	else if (newcolor >= 'a' && newcolor < NUM_TEXT_COLORS + 'a')	// Standard, lowercase
	{
		newcolor -= 'a';
	}
	else							// Incomplete!
	{
		color_value = ch - (newcolor == '\0');
		return CR_UNDEFINED;
	}
	color_value = ch;
	return EColorRange(newcolor);
}

//==========================================================================
//
// V_InitFonts
//
//==========================================================================

void V_InitFonts()
{
	V_InitCustomFonts();

	FFont *CreateHexLumpFont(const char *fontname, const char* lump);
	FFont *CreateHexLumpFont2(const char *fontname, const char * lump);

	if (fileSystem.FindFile("engine/newconsolefont.hex") < 0)
		I_Error("newconsolefont.hex not found");	// This font is needed - do not start up without it.
	NewConsoleFont = CreateHexLumpFont("NewConsoleFont", "engine/newconsolefont.hex");
	NewSmallFont = CreateHexLumpFont2("NewSmallFont", "engine/newconsolefont.hex");
	CurrentConsoleFont = NewConsoleFont;

	ConFont = V_GetFont("ConsoleFont", "engine/confont.lmp");	// The con font is needed for the slider graphics
	SmallFont = ConFont;	// This is so that it doesn't crash and that it immediately gets seen as a proble. The SmallFont should later be mapped to the small game font.
}

void V_ClearFonts()
{
	while (FFont::FirstFont != nullptr)
	{
		delete FFont::FirstFont;
	}
	FFont::FirstFont = nullptr;
	AlternativeSmallFont = OriginalSmallFont = CurrentConsoleFont = NewSmallFont = NewConsoleFont = SmallFont = SmallFont2 = BigFont = ConFont = IntermissionFont = nullptr;
}

//==========================================================================
//
// CleanseString
//
// Does some mild sanity checking on a string: If it ends with an incomplete
// color escape, the escape is removed.
//
//==========================================================================

char* CleanseString(char* str)
{
	char* escape = strrchr(str, TEXTCOLOR_ESCAPE);
	if (escape != NULL)
	{
		if (escape[1] == '\0')
		{
			*escape = '\0';
		}
		else if (escape[1] == '[')
		{
			char* close = strchr(escape + 2, ']');
			if (close == NULL)
			{
				*escape = '\0';
			}
		}
	}
	return str;
}

int stripaccent(int code)
{
	if (code < 0x8a)
		return code;
	if (code < 0x100)
	{
		if (code == 0x8a)	// Latin capital letter S with caron
			return 'S';
		if (code == 0x8e)	// Latin capital letter Z with caron
			return 'Z';
		if (code == 0x9a)	// Latin small letter S with caron
			return 's';
		if (code == 0x9e)	// Latin small letter Z with caron
			return 'z';
		if (code == 0x9f)	// Latin capital letter Y with diaeresis
			return 'Y';
		if (code == 0xab || code == 0xbb) return '"';	// typographic quotation marks.
		if (code == 0xff)	// Latin small letter Y with diaeresis
			return 'y';
		// Every other accented character has the high two bits set.
		if ((code & 0xC0) == 0)
			return code;
		// Make lowercase characters uppercase so there are half as many tests.
		int acode = code & 0xDF;
		if (acode >= 0xC0 && acode <= 0xC5)		// A with accents
			return 'A' + (code & 0x20);
		if (acode == 0xC7)						// Cedilla
			return 'C' + (acode & 0x20);
		if (acode >= 0xC8 && acode <= 0xCB)		// E with accents
			return 'E' + (code & 0x20);
		if (acode >= 0xCC && acode <= 0xCF)		// I with accents
			return 'I' + (code & 0x20);
		if (acode == 0xD0)						// Eth
			return 'D' + (code & 0x20);
		if (acode == 0xD1)						// N with tilde
			return 'N' + (code & 0x20);
		if ((acode >= 0xD2 && acode <= 0xD6) ||	// O with accents
			acode == 0xD8)						// O with stroke
			return 'O' + (code & 0x20);
		if (acode >= 0xD9 && acode <= 0xDC)		// U with accents
			return 'U' + (code & 0x20);
		if (acode == 0xDD)						// Y with accute
			return 'Y' + (code & 0x20);
		if (acode == 0xDE)						// Thorn
			return 'P' + (code & 0x20);			// well, it sort of looks like a 'P'
	}
	else if (code >= 0x100 && code < 0x180)
	{
		// For the double-accented Hungarian letters it makes more sense to first map them to the very similar looking Umlauts.
		// (And screw the crappy specs that do not allow UTF-8 multibyte character literals here.)
		if (code == 0x150) code = 0xd6;
		else if (code == 0x151) code = 0xf6;
		else if (code == 0x170) code = 0xdc;
		else if (code == 0x171) code = 0xfc;
		else
		{
			static const char accentless[] = "AaAaAaCcCcCcCcDdDdEeEeEeEeEeGgGgGgGgHhHhIiIiIiIiIiIiJjKkkLlLlLlLlLlNnNnNnnNnOoOoOoOoRrRrRrSsSsSsSsTtTtTtUuUuUuUuUuUuWwYyYZzZzZzs";
			return accentless[code - 0x100];
		}
	}
	else if (code >= 0x200 && code < 0x218)
	{
		// 0x200-0x217 are irrelevant but easy to map to other characters more likely to exist.
		static const uint16_t u200map[] = { 0xc4, 0xe4, 0xc2, 0xe2, 0xcb, 0xeb, 0xca, 0xea, 0xcf, 0xef, 0xce, 0xee, 0xd6, 0xf6, 0xd4, 0xe4, 'R', 'r', 'R', 'r', 0xdc, 0xfc, 0xdb, 0xfb };
		return u200map[code - 0x200];
	}
	return getAlternative(code);
}

int getAlternative(int code)
{
	// This is for determining replacements that do not make CanPrint fail.
	switch (code)
	{
	default:
		return code;

	case 0x17f:		// The 'long s' can be safely remapped to the regular variant, not that this gets used in any real text...
		return 's';

	case 0x218:		// Romanian S with comma below may get remapped to S with cedilla.
		return 0x15e;

	case 0x219:
		return 0x15f;

	case 0x21a:		// Romanian T with comma below may get remapped to T with cedilla.
		return 0x162;

	case 0x21b:
		return 0x163;

		// Greek characters with equivalents in either Latin or Cyrillic. This is only suitable for uppercase fonts!
	case 0x391:
		return 'A';

	case 0x392:
		return 'B';

	case 0x393:
		return 0x413;

	case 0x395:
		return 'E';

	case 0x396:
		return 'Z';

	case 0x397:
		return 'H';

	case 0x399:
		return 'I';

	case 0x39a:
		return 'K';

	case 0x39c:
		return 'M';

	case 0x39d:
		return 'N';

	case 0x39f:
		return 'O';

	case 0x3a0:
		return 0x41f;

	case 0x3a1:
		return 'P';

	case 0x3a4:
		return 'T';

	case 0x3a5:
		return 'Y';

	case 0x3a6:
		return 0x424;

	case 0x3a7:
		return 'X';

	case 0x3aa:
		return 0xcf;

	case 0x3ab:
		return 0x178;

	case 0x3bf:
		return 'o';

	case 0x3c2:
		return 0x3c3;	// Lowercase Sigma character in Greek, which changes depending on its positioning in a word; if the font is uppercase only or features a smallcaps style, the second variant of the letter will remain unused

	case 0x3ca:
		return 0xef;

	case 0x3cc:
		return 0xf3;

		// Cyrillic characters with equivalents in the Latin alphabet.
	case 0x400:
		return 0xc8;

	case 0x401:
		return 0xcb;

	case 0x405:
		return 'S';

	case 0x406:
		return 'I';

	case 0x407:
		return 0xcf;

	case 0x408:
		return 'J';

	case 0x450:
		return 0xe8;

	case 0x451:
		return 0xeb;

	case 0x455:
		return 's';

	case 0x456:
		return 'i';

	case 0x457:
		return 0xef;

	case 0x458:
		return 'j';

	}

	// skip the rest of Latin characters because none of them are relevant for modern languages, except Vietnamese which cannot be represented with the tiny bitmap fonts anyway.

	return code;
}