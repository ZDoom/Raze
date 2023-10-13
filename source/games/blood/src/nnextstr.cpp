//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT
Copyright (C) NoOne

*****************************************************************
NoOne: A very basic string parser. Update or replace eventually.
*****************************************************************

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#ifdef NOONE_EXTENSIONS
#include "common_game.h"
#include "nnextstr.h"
#include "nnexts.h"

struct NAMED_TYPE
{
    unsigned int id;
    const char* text;
};

static const NAMED_TYPE gBoolNames[] =
{
    { false, "0" },
    { true, "1" },
    { false, "No" },
    { true, "Yes" },
    { false, "False" },
    { true, "True" },
};

const char* enumStrGetChar(int nOffs, char* out, const char* str, char expcr)
{
    int j = ClipLow(nOffs, 0);
    int i = 0;

    out[0] = '\0';
    
    if (j > 0)
    {
        // search for start
        while (str[i] && j > 0)
        {
            if (str[i++] == expcr)
                j--;
        }
    }

    while (str[i] && str[i] != expcr)
        out[j++] = str[i++];
    
    
    out[j] = '\0';
    
    return (out[0]) ? out : NULL;
}

int enumStr(int nOffs, const char* str, char* key, char* val)
{
    if (!str)
        return 0;
    
    const char* pStr;
    char buffer1[256], buffer2[256], string[256];
    int t;

    if (isarray(str))
    {
        t = Bstrlen(str);
        Bstrcpy(string, str);
        string[t] = '\0';

        pStr = &string[(string[0] == '(')];
        if (string[t - 1] == ')')
            string[t - 1] = '\0';

        removeSpaces(string);

        if (enumStrGetChar(nOffs, buffer1, pStr, ','))
        {
            if (key)
            {
                if (enumStrGetChar(0, buffer2, buffer1, '='))
                    Bsprintf(key, "%s", buffer2);
                else
                    key[0] = '\0';
            }

            if (val)
            {
                if (enumStrGetChar(1, buffer2, buffer1, '='))
                {
                    Bsprintf(val, "%s", buffer2);
                    t = ClipLow(Bstrlen(val), 1);
                    if (val[0] == '(' && val[t - 1] != ')')
                    {
                        char tval[256];

                        nOffs++;
                        while ( 1 )
                        {
                            if ((nOffs = enumStr(nOffs, str, tval)) != 0)
                            {
                                t = Bstrlen(tval); Bstrcat(val, ","); Bstrcat(val, tval);
                                if (tval[t - 1] != ')')
                                    continue;
                            }
                            else
                            {
                                ThrowError("End of array is not found in \"%s\"", str);
                            }

                            return nOffs;
                        }
                    }

                }
                else
                    val[0] = '\0';
            }

            return ++nOffs;
        }
    }

    return 0;
}

int enumStr(int nOffs, const char* str, char* val)
{
    if (!str)
        return 0;
    
    const char* pStr;
    char string[256];
    int t;

    t = Bstrlen(str);
    Bstrcpy(string, str);
    string[t] = '\0';

    pStr = &string[(string[0] == '(')];
    if (string[t - 1] == ')')
        string[t - 1] = '\0';

    removeSpaces(string);
    if (enumStrGetChar(nOffs, val, pStr, ','))
        return ++nOffs;

    return 0;
}

void removeSpaces(char* str)
{
    if (str)
    {
        int t = Bstrlen(str);
        for (int i = t - 1; i >= 0; i--)
        {
            if (!isspace(str[i]))
                continue;

            for (int j = i; j < t; j++) { str[j] = str[j + 1]; }
        }
    }
}

int btoi(const char* str)
{
    if (str)
    {
        int i;
        const NAMED_TYPE* pEntry = gBoolNames;
        for (i = 0; i < LENGTH(gBoolNames); i++)
        {
            if (Bstrcasecmp(str, pEntry->text) == 0)
                return (bool)pEntry->id;

            pEntry++;
        }
    }

    return -1;
}
char isbool(const char* str) { return (str && btoi(str) != -1); }
char isarray(const char* str, int* nLen)
{
    if (nLen)
        *nLen = 0;

    if (str)
    {
        int l = Bstrlen(str);
        if (l && str[0] == '(' && str[l - 1] == ')')
        {
            if (nLen)
            {
                *nLen = *nLen + 1;
                const char* pStr = str;
                while ((pStr = Bstrchr(pStr, ',')) != NULL)
                    pStr++, * nLen = *nLen + 1;
            }

            return true;
        }
    }

    return false;
}

char isperc(const char* str)
{
    if (str)
    {
        int l = Bstrlen(str);
        if (--l > 0 && str[l] == '%')
        {
            while (--l > 0)
            {
                if (!isdigit(str[l]))
                    return false;
            }

            if (isdigit(str[l]) || str[l] == '-' || str[l] == '+')
                return true;
        }
    }

    return false;
}

char isfix(const char* str, char flags)
{
    if (str)
    {
        int l = Bstrlen(str);
        if (l > 0)
        {
            if (!isdigit(str[0]))
            {
                switch (str[0])
                {
                    case '-':
                        if (!(flags & 0x01)) return false;
                        break;
                    case '+':
                        if (!(flags & 0x02)) return false;
                        break;
                    default:
                        return false;

                }
            }

            while (--l > 0)
            {
                if (!isdigit(str[l]))
                    return false;
            }

            return true;
        }
    }

    return false;

}


char isufix(const char* str)
{
    return isfix(str, 0);
}

char isempty(const char* str)
{
    return (!str || str[0] == '\0');
}
#endif