//-----------------------------------------------------------------------------
//
// Copyright 1993-1996 id Software
// Copyright 1994-1996 Raven Software
// Copyright 1999-2016 Randy Heit
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//
// Alternatively the following applies:
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//-----------------------------------------------------------------------------
//


#include <string.h>
#include <ctype.h>
#include "m_swap.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "v_text.h"
#include "d_gui.h"
#include "g_input.h"
#include "d_event.h"
#include "v_video.h"
#include "utf8.h"
#include "gstrings.h"
#include "vm.h"
#include "c_buttons.h"
#include "v_draw.h"
#include "razemenu.h"
#include "gamestruct.h"
#include "gamecvars.h"
#include "menustate.h"

enum
{
	QUEUESIZE = 128
};


EXTERN_CVAR (Bool, sb_cooperative_enable)
EXTERN_CVAR (Bool, sb_deathmatch_enable)
EXTERN_CVAR (Bool, sb_teamdeathmatch_enable)

int active_con_scale();

// Public data

void CT_Init ();
void CT_Drawer ();
bool CT_Responder (event_t *ev);
void CT_PasteChat(const char *clip);

int chatmodeon;

// Private data

static void CT_ClearChatMessage ();
static void CT_AddChar (int c);
static void CT_BackSpace ();
static void ShoveChatStr (const char *str, uint8_t who);
static bool DoSubstitution (FString &out, const char *in);

static TArray<uint8_t> ChatQueue;

extern FStringCVar* const CombatMacros[];


CVAR (Bool, chat_substitution, false, CVAR_ARCHIVE)

//===========================================================================
//
// CT_Init
//
// 	Initialize chat mode data
//===========================================================================

void CT_Init ()
{
	ChatQueue.Clear();
	chatmodeon = 0;
}

//===========================================================================
//
// CT_Stop
//
//===========================================================================

void CT_Stop ()
{
	chatmodeon = 0;
}

//===========================================================================
//
// CT_Responder
//
//===========================================================================

bool CT_Responder (event_t *ev)
{
	if (chatmodeon && ev->type == EV_GUI_Event)
	{
		if (ev->subtype == EV_GUI_KeyDown || ev->subtype == EV_GUI_KeyRepeat)
		{
			if (ev->data1 == '\r')
			{
				ChatQueue.Push(0);
				ShoveChatStr ((char *)ChatQueue.Data(), chatmodeon - 1);
				ChatQueue.Pop();
				CT_Stop ();
				return true;
			}
			else if (ev->data1 == GK_ESCAPE)
			{
				CT_Stop ();
				return true;
			}
			else if (ev->data1 == '\b')
			{
				CT_BackSpace ();
				return true;
			}
#ifdef __APPLE__
			else if (ev->data1 == 'C' && (ev->data3 & GKM_META))
#else // !__APPLE__
			else if (ev->data1 == 'C' && (ev->data3 & GKM_CTRL))
#endif // __APPLE__
			{
				ChatQueue.Push(0);
				I_PutInClipboard ((char *)ChatQueue.Data());
				ChatQueue.Pop();
				return true;
			}
#ifdef __APPLE__
			else if (ev->data1 == 'V' && (ev->data3 & GKM_META))
#else // !__APPLE__
			else if (ev->data1 == 'V' && (ev->data3 & GKM_CTRL))
#endif // __APPLE__
			{
				CT_PasteChat(I_GetFromClipboard(false));
			}
		}
		else if (ev->subtype == EV_GUI_Char)
		{
			// send a macro
			if (ev->data2 && (ev->data1 >= '0' && ev->data1 <= '9'))
			{
				ShoveChatStr (*CombatMacros[ev->data1 - '0'], chatmodeon - 1);
				CT_Stop ();
			}
			else
			{
				CT_AddChar (ev->data1);
			}
			return true;
		}
#ifdef __unix__
		else if (ev->subtype == EV_GUI_MButtonDown)
		{
			CT_PasteChat(I_GetFromClipboard(true));
		}
#endif
	}

	return false;
}

//===========================================================================
//
// CT_PasteChat
//
//===========================================================================

void CT_PasteChat(const char *clip)
{
	if (clip != nullptr && *clip != '\0')
	{
		auto p = (const uint8_t *)clip;
		// Only paste the first line.
		while (auto chr = GetCharFromString(p))
		{
			if (chr == '\n' || chr == '\r' || chr == '\b')
			{
				break;
			}
			CT_AddChar (chr);
		}
	}
}

//===========================================================================
//
// CT_Drawer
//
//===========================================================================

void CT_Drawer (void)
{
	auto drawer = twod;
	FFont *displayfont = NewConsoleFont;

	if (chatmodeon)
	{
		FStringf prompt("%s ", GStrings("TXT_SAY"));
		int x, scalex, y, promptwidth;


		scalex = 1;
		int scale = active_con_scale(drawer);
		int screen_width = twod->GetWidth() / scale;
		int screen_height= twod->GetHeight() / scale;
		
		y = screen_height - displayfont->GetHeight()-2;
		auto res = gi->GetReservedScreenSpace(hud_size);

		promptwidth = displayfont->StringWidth (prompt) * scalex;
		x = displayfont->GetCharWidth (displayfont->GetCursor()) * scalex * 2 + promptwidth;

		FString printstr = ChatQueue;
		// figure out if the text is wider than the screen
		// if so, only draw the right-most portion of it.
		const uint8_t *textp = (const uint8_t*)printstr.GetChars();
		while(*textp)
		{
			auto textw = displayfont->StringWidth(textp);
			if (x + textw * scalex < screen_width) break;
			GetCharFromString(textp);
		}
		printstr += displayfont->GetCursor();

		twod->AddColorOnlyQuad(0, y, screen_width, screen_height, 0x80000000);
		DrawText(drawer, displayfont, CR_GREEN, 0, y, prompt.GetChars(), 
			DTA_VirtualWidth, screen_width, DTA_VirtualHeight, screen_height, DTA_KeepRatio, true, TAG_DONE);
		DrawText(drawer, displayfont, CR_GREY, promptwidth, y, printstr,
			DTA_VirtualWidth, screen_width, DTA_VirtualHeight, screen_height, DTA_KeepRatio, true, TAG_DONE);
	}
}

//===========================================================================
//
// CT_AddChar
//
//===========================================================================

static void CT_AddChar (int c)
{
	if (ChatQueue.Size() < QUEUESIZE-2)
	{
		int size;
		auto encode = MakeUTF8(c, &size);
		if (*encode)
		{
			for (int i = 0; i < size; i++)
			{
				ChatQueue.Push(encode[i]);
			}
		}
	}
}

//===========================================================================
//
// CT_BackSpace
//
// 	Backs up a space, when the user hits (obviously) backspace
//===========================================================================

static void CT_BackSpace ()
{
	if (ChatQueue.Size())
	{
		int endpos = ChatQueue.Size() - 1;
		while (endpos > 0 && ChatQueue[endpos] >= 0x80 && ChatQueue[endpos] < 0xc0) endpos--;
		ChatQueue.Clamp(endpos);
	}
}

//===========================================================================
//
// CT_ClearChatMessage
//
// 	Clears out the data for the chat message.
//===========================================================================

static void CT_ClearChatMessage ()
{
	ChatQueue.Clear();
}

//===========================================================================
//
// ShoveChatStr
//
// Sends the chat message across the network
//
//===========================================================================

static void ShoveChatStr (const char *str, uint8_t who)
{
	// Don't send empty messages
	if (str == NULL || str[0] == '\0')
		return;

	if (*str == '#')
	{
		C_DoCommand(FStringf("activatecheat \"%s\"", str + 1));
	}
	else
	{
#if 0
		FString substBuff;

		if (str[0] == '/' &&
			(str[1] == 'm' || str[1] == 'M') &&
			(str[2] == 'e' || str[2] == 'E'))
		{ // This is a /me message
			str += 3;
			who |= 2;
		}

		Net_WriteByte(DEM_SAY);
		Net_WriteByte(who);

		if (!chat_substitution || !DoSubstitution(substBuff, str))
		{
			Net_WriteString(MakeUTF8(str));
		}
		else
		{
			Net_WriteString(MakeUTF8(substBuff));
		}
#else
		Printf("%s %s\n", GStrings("TXT_SAY"), str);
#endif
	}
}

//===========================================================================
//
// DoSubstitution
//
// Replace certain special substrings with different values to reflect
// the player's current state.
//
//===========================================================================

static bool DoSubstitution (FString &out, const char *in)
{
#if 0
	const char *a, *b;

	a = in;
	out = "";
	while ( (b = strchr(a, '$')) )
	{
		out.AppendCStrPart(a, b - a);

		a = ++b;
		while (*b && isalpha(*b))
		{
			++b;
		}

		ptrdiff_t ByteLen = b - a;

		// todo: forward to the game modules. Doom used the following tokens:
		// health, weapon, armor, ammocount, ammo
		if (0)
		{
		}
		else
		{
			out += '$';
			out.AppendCStrPart(a, ByteLen);
		}
		a = b;
	}

	// Return false if no substitution was performed
	if (a == in)
	{
		return false;
	}

	out += a;
	return true;
#else
	return false;
#endif
}

CCMD (messagemode)
{
	if (menuactive == MENU_Off)
	{
		chatmodeon = 1;
		C_HideConsole ();
		CT_ClearChatMessage ();
	}
}


CCMD (messagemode2)
{
	if (menuactive == MENU_Off)
	{
		chatmodeon = 2;
		C_HideConsole ();
		CT_ClearChatMessage ();
	}
}

#if 0
CCMD (say)
{
	if (argv.argc() == 1)
	{
		Printf ("Usage: say <message>\n");
	}
	else
	{
		ShoveChatStr (argv[1], 0);
	}
}

CCMD (say_team)
{
	if (argv.argc() == 1)
	{
		Printf ("Usage: say_team <message>\n");
	}
	else
	{
		ShoveChatStr (argv[1], 1);
	}
}
#endif
