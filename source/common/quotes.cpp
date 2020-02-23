/*
**
** quotes.cpp
** Duke-Nukem-style quote buffer
**
**---------------------------------------------------------------------------
** Copyright 2019 Christoph Oelckers
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
** This is actually a VERY inefficient way to manage strings 
** but needs to be preserved because the CON VM depends on it.
*/ 

#include "quotemgr.h"
#include "savegamehelp.h"
#include "serializer.h"
#include "printf.h"


void Quotes::MakeStringLabel(FString &quote)
{
	// Only prepend a quote if the string is localizable.
	if (quote.Len() > 0 && quote[0] != '$' && GStrings[quote]) quote.Insert(0, "$");
}

void Quotes::InitializeQuote(int num, const char *text, bool fromscript)
{
	quotes[num] = text;
	if (fromscript)	// means this is the initial setup from the source data.
	{
		MakeStringLabel(quotes[num]);
	}
}

void Quotes::InitializeExQuote(int num, const char *text, bool fromscript)
{
	exquotes[num] = text;
	if (fromscript)	// means this is the initial setup from the source data.
	{
		MakeStringLabel(quotes[num]);
	}
}

void Quotes::AppendQuote(int dst, int src, int len)
{
	// This needs to apply the localization because the combined string is not localizable anymore.
	if (quotes[dst][0] == '$') quotes[dst] = GStrings.localize(quotes[dst]);
	if (len < 0) quotes[dst] << GStrings.localize(quotes[src]);
	else quotes[dst] += FString(GStrings.localize(quotes[src]), len);
}

void Quotes::FormatQuote(int dst, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	quotes[dst].VFormat(fmt, ap);
}

void Quotes::Substitute(int dst, const char* text, const char* replc)
{
	if (quotes[dst][0] == '$') quotes[dst] = GStrings.localize(quotes[dst]);
	quotes[dst].Substitute(text, replc);
}


void Quotes::Serialize(FSerializer &arc)
{
	// This only saves the regular quotes. The ExQuotes array is immutable once initialized.
	if (arc.BeginObject("quotes"))
	{
		for (int i = 0; i < MAXQUOTES; i++)
		{
			char buf[10];
			mysnprintf(buf, 10, "%d", i);
			FString nulstr;
			arc(buf, quotes[i], nulstr);
		}
		arc.EndObject();
	}
}

Quotes quoteMgr;
