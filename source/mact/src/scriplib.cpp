/*
 * scriplib.c
 * MACT library Script file parsing and writing
 *
 * by Jonathon Fowler
 *
 * Since we weren't given the source for MACT386.LIB so I've had to do some
 * creative interpolation here.
 *
 * This all should be rewritten in a much much cleaner fashion.
 *
 */
//-------------------------------------------------------------------------
/*
Duke Nukem Copyright (C) 1996, 2003 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#include "compat.h"

#include "scriplib.h"
#include "file_lib.h"
#include "_scrplib.h"

static script_t *scriptfiles[MAXSCRIPTFILES];


#define SC(s) scriptfiles[s]


int32_t SCRIPT_New(void)
{
    int32_t i;

    for (i=0; i<MAXSCRIPTFILES; i++)
    {
        if (!SC(i))
        {
            SC(i) = (script_t *)Xmalloc(sizeof(script_t));
            memset(SC(i), 0, sizeof(script_t));
            return i;
        }
    }

    return -1;
}

void SCRIPT_Delete(int32_t scripthandle)
{
    ScriptSectionType *s;

    if (scripthandle < 0 || scripthandle >= MAXSCRIPTFILES) return;

    if (!SC(scripthandle)) return;

    if (SCRIPT(scripthandle,apScript))
    {
        while (SCRIPT(scripthandle,apScript)->nextsection != SCRIPT(scripthandle,apScript))
        {
            s = SCRIPT(scripthandle,apScript)->nextsection;
            SCRIPT_FreeSection(SCRIPT(scripthandle,apScript));
            Bfree(SCRIPT(scripthandle,apScript));
            SCRIPT(scripthandle,apScript) = s;
        }

        SCRIPT_FreeSection(SCRIPT(scripthandle, apScript));
        Bfree(SCRIPT(scripthandle,apScript));
    }

    Bfree(SC(scripthandle));
    SC(scripthandle) = 0;
}

void SCRIPT_FreeSection(ScriptSectionType * section)
{
    ScriptEntryType *e;

    if (!section) return;
    if (!section->entries) return;

    while (section->entries->nextentry != section->entries)
    {
        e = section->entries->nextentry;

        Bfree(section->entries->name);
        Bfree(section->entries->value);
        Bfree(section->entries);
        section->entries = e;
    }

    Bfree(section->entries->name);
    Bfree(section->entries->value);
    Bfree(section->entries);
    Bfree(section->name);
}

#define AllocSection(s) \
    { \
        (s) = (ScriptSectionType *)Xmalloc(sizeof(ScriptSectionType)); \
        (s)->name = NULL; \
        (s)->entries = NULL; \
        (s)->lastline = NULL; \
        (s)->nextsection = (s); \
        (s)->prevsection = (s); \
    }
#define AllocEntry(e) \
    { \
        (e) = (ScriptEntryType *)Xmalloc(sizeof(ScriptEntryType)); \
        (e)->name = NULL; \
        (e)->value = NULL; \
        (e)->nextentry = (e); \
        (e)->preventry = (e); \
    }

ScriptSectionType * SCRIPT_SectionExists(int32_t scripthandle, const char * sectionname)
{
    ScriptSectionType *s, *ls=NULL;

    if (scripthandle < 0 || scripthandle >= MAXSCRIPTFILES) return NULL;
    if (!sectionname) return NULL;
    if (!SC(scripthandle)) return NULL;
    if (!SCRIPT(scripthandle,apScript)) return NULL;

    for (s = SCRIPT(scripthandle,apScript); ls != s; ls=s,s=s->nextsection)
        if (!Bstrcasecmp(s->name, sectionname)) return s;

    return NULL;
}

ScriptSectionType * SCRIPT_AddSection(int32_t scripthandle, const char * sectionname)
{
    ScriptSectionType *s,*s2;

    if (scripthandle < 0 || scripthandle >= MAXSCRIPTFILES) return NULL;
    if (!sectionname) return NULL;
    if (!SC(scripthandle)) return NULL;

    s = SCRIPT_SectionExists(scripthandle, sectionname);
    if (s) return s;

    AllocSection(s);
    s->name = Bstrdup(sectionname);
    if (!SCRIPT(scripthandle,apScript))
    {
        SCRIPT(scripthandle,apScript) = s;
    }
    else
    {
        s2 = SCRIPT(scripthandle,apScript);
        while (s2->nextsection != s2) s2=s2->nextsection;
        s2->nextsection = s;
        s->prevsection = s2;
    }

    return s;
}

ScriptEntryType * SCRIPT_EntryExists(ScriptSectionType * section, const char * entryname)
{
    ScriptEntryType *e,*le=NULL;

    if (!section) return NULL;
    if (!entryname) return NULL;
    if (!section->entries) return NULL;

    for (e = section->entries; le != e; le=e,e=e->nextentry)
        if (!Bstrcasecmp(e->name, entryname)) return e;

    return NULL;
}

void SCRIPT_AddEntry(int32_t scripthandle, const char * sectionname, const char * entryname, const char * entryvalue)
{
    ScriptSectionType *s;
    ScriptEntryType *e,*e2;

    if (scripthandle < 0 || scripthandle >= MAXSCRIPTFILES) return;
    if (!sectionname || !entryname || !entryvalue) return;
    if (!SC(scripthandle)) return;

//	s = SCRIPT_SectionExists(scripthandle, sectionname);
//	if (!s) {
    s = SCRIPT_AddSection(scripthandle, sectionname);
    if (!s) return;
//	}

    e = SCRIPT_EntryExists(s, entryname);
    if (!e)
    {
        AllocEntry(e);
        e->name = Bstrdup(entryname);
        if (!s->entries)
        {
            s->entries = e;
        }
        else
        {
            e2 = s->entries;
            while (e2->nextentry != e2) e2=e2->nextentry;
            e2->nextentry = e;
            e->preventry = e2;
        }
    }

    Bfree(e->value);
    e->value = Bstrdup(entryvalue);
}


int32_t SCRIPT_ParseBuffer(int32_t scripthandle, char *data, int32_t length)
{
    char *fence = data + length;
    char *dp, *sp, ch=0, lastch=0;
    char const *currentsection = "";
    char *currententry = NULL;
    char *currentvalue = NULL;
    enum
    {
        ParsingIdle,
        ParsingSectionBegin,
        ParsingSectionName,
        ParsingEntry,
        ParsingValueBegin,
        ParsingValue
    };
    enum
    {
        ExpectingSection = 1,
        ExpectingEntry = 2,
        ExpectingAssignment = 4,
        ExpectingValue = 8,
        ExpectingComment = 16
    };
    int32_t state;
    int32_t expect;
    int32_t linenum=1;
    int32_t rv = 0;
#define SETRV(v) if (v>rv||rv==0) rv=v

    if (!data) return 1;
    if (length < 0) return 1;

    dp = sp = data;
    state = ParsingIdle;
    expect = ExpectingSection | ExpectingEntry;

#define EATLINE(p) while (length > 0 && *p != '\n' && *p != '\r') { p++; length--; }
#define LETTER() { lastch = ch; ch = *(sp++); length--; }

    while (length > 0)
    {
        switch (state)
        {
        case ParsingIdle:
            LETTER();
            switch (ch)
            {
                // whitespace
            case ' ':
            case '\t': continue;
            case '\n': if (lastch == '\r') continue; linenum++; continue;
            case '\r': linenum++; continue;

            case ';':
                /*case '#':*/
                EATLINE(sp);
                continue;

            case '[': if (!(expect & ExpectingSection))
                {
                    // Unexpected section start
                    printf("Unexpected start of section on line %d.\n", linenum);
                    SETRV(-1);
                    EATLINE(sp);
                    continue;
                }
                else
                {
                    state = ParsingSectionBegin;
                    continue;
                }

            default:  if (isalpha(ch))
                {
                    if (!(expect & ExpectingEntry))
                    {
                        // Unexpected name start
                        printf("Unexpected entry LabelText on line %d.\n", linenum);
                        SETRV(-1);
                        EATLINE(sp);
                        continue;
                    }
                    else
                    {
                        currententry = dp = sp-1;
                        state = ParsingEntry;
                        continue;
                    }
                }
                else
                {
                    // Unexpected character
                    printf("Illegal character (ASCII %d) on line %d.\n", ch, linenum);
                    SETRV(-1);
                    EATLINE(sp);
                    continue;
                }
            }

        case ParsingSectionBegin:
            currentsection = dp = sp;
            state = ParsingSectionName;
            fallthrough__;
        case ParsingSectionName:
            LETTER();
            switch (ch)
            {
            case '\n':
            case '\r':	// Unexpected newline
                printf("Unexpected newline on line %d.\n", linenum);
                SETRV(-1);
                state = ParsingIdle;
                linenum++;
                continue;

            case ']':
                *(dp) = 0;	// Add new section
                expect = ExpectingSection | ExpectingEntry;
                state = ParsingIdle;
                EATLINE(sp);
                continue;

            default:
                dp++;
                continue;
            }

        case ParsingEntry:
            LETTER();
            switch (ch)
            {
            case ';':
                /*case '#':*/
                // unexpected comment
                EATLINE(sp);
                printf("Unexpected comment on line %d.\n", linenum);
                SETRV(-1);
                fallthrough__;
            case '\n':
            case '\r':
                // Unexpected newline
                printf("Unexpected newline on line %d.\n", linenum);
                SETRV(-1);
                expect = ExpectingSection | ExpectingEntry;
                state = ParsingIdle;
                linenum++;
                continue;

            case '=':
                // Entry name finished, now for the value
                while (*dp == ' ' || *dp == '\t') dp--;
                *(++dp) = 0;
                state = ParsingValueBegin;
                continue;

            default:
                dp++;
                continue;
            }

        case ParsingValueBegin:
            currentvalue = dp = sp;
            state = ParsingValue;
            fallthrough__;
        case ParsingValue:
            LETTER();
            switch (ch)
            {
            case '\n':
            case '\r':
                // value complete, add it using parsed name
                while (*dp == ' ' || *dp == '\t') dp--;
                *(dp) = 0;
                while (*currentvalue == ' ' || *currentvalue == '\t') currentvalue++;
                state = ParsingIdle;
                linenum++;

                SCRIPT_AddSection(scripthandle,currentsection);
                SCRIPT_AddEntry(scripthandle,currentsection,currententry,currentvalue);
                continue;

            default:
                dp++;
                continue;
            }

        default: length=0;
            continue;
        }
    }

    if (sp > fence) printf("Stepped outside the fence!\n");

    return rv;
}


//---

int32_t SCRIPT_Init(char const * name)
{
    int32_t h = SCRIPT_New();

    if (h >= 0) Bstrncpy(SCRIPT(h,scriptfilename), name, 127);

    return h;
}

void SCRIPT_Free(int32_t scripthandle)
{
    SCRIPT_Delete(scripthandle);
}

int32_t SCRIPT_Load(char const * filename)
{
    int32_t s,h,l;
    char *b;

    h = SafeOpenRead(filename, filetype_binary);
    l = SafeFileLength(h)+1;
    b = (char *)Xmalloc(l);
    SafeRead(h,b,l-1);
    b[l-1] = '\n';	// JBF 20040111: evil nasty hack to trick my evil nasty parser
    SafeClose(h);

    s = SCRIPT_Init(filename);
    if (s<0)
    {
        Bfree(b);
        return -1;
    }

    SCRIPT_ParseBuffer(s,b,l);

    Bfree(b);

    return s;
}

void SCRIPT_Save(int32_t scripthandle, char const * filename)
{
    char const *section, *entry, *value;
    int32_t sec, ent, numsect, nument;
    FILE *fp;


    if (!filename) return;
    if (!SC(scripthandle)) return;

    fp = fopen(filename, "w");
    if (!fp) return;

    numsect = SCRIPT_NumberSections(scripthandle);
    for (sec=0; sec<numsect; sec++)
    {
        section = SCRIPT_Section(scripthandle, sec);
        if (sec>0) fprintf(fp, "\n");
        if (section[0] != 0)
            fprintf(fp, "[%s]\n", section);

        nument = SCRIPT_NumberEntries(scripthandle,section);
        for (ent=0; ent<nument; ent++)
        {
            entry = SCRIPT_Entry(scripthandle,section,ent);
            value = SCRIPT_GetRaw(scripthandle,section,entry);

            fprintf(fp, "%s = %s\n", entry, value);
        }
    }

    fclose(fp);
}

int32_t SCRIPT_NumberSections(int32_t scripthandle)
{
    int32_t c=0;
    ScriptSectionType *s,*ls=NULL;

    if (!SC(scripthandle)) return 0;
    if (!SCRIPT(scripthandle,apScript)) return 0;

    for (s = SCRIPT(scripthandle,apScript); ls != s; ls=s,s=s->nextsection) c++;

    return c;
}

char const * SCRIPT_Section(int32_t scripthandle, int32_t which)
{
    ScriptSectionType *s,*ls=NULL;

    if (!SC(scripthandle)) return "";
    if (!SCRIPT(scripthandle,apScript)) return "";

    for (s = SCRIPT(scripthandle,apScript); which>0 && ls != s; ls=s, s=s->nextsection, which--) ;

    return s->name;
}

int32_t SCRIPT_NumberEntries(int32_t scripthandle, char const * sectionname)
{
    ScriptSectionType *s;
    ScriptEntryType *e,*le=NULL;
    int32_t c=0;

    if (!SC(scripthandle)) return 0;
    if (!SCRIPT(scripthandle,apScript)) return 0;

    s = SCRIPT_SectionExists(scripthandle, sectionname);
    if (!s) return 0;

    for (e = s->entries; le != e; le=e,e=e->nextentry) c++;
    return c;
}

char const * SCRIPT_Entry(int32_t scripthandle, char const * sectionname, int32_t which)
{
    ScriptSectionType *s;
    ScriptEntryType *e,*le=NULL;

    if (!SC(scripthandle)) return 0;
    if (!SCRIPT(scripthandle,apScript)) return 0;

    s = SCRIPT_SectionExists(scripthandle, sectionname);
    if (!s) return "";

    for (e = s->entries; which>0 && le != e; le=e, e=e->nextentry, which--) ;
    return e->name;
}

char const * SCRIPT_GetRaw(int32_t scripthandle, char const * sectionname, char const * entryname)
{
    ScriptSectionType *s;
    ScriptEntryType *e;

    if (!SC(scripthandle)) return 0;
    if (!SCRIPT(scripthandle,apScript)) return 0;

    s = SCRIPT_SectionExists(scripthandle, sectionname);
    e = SCRIPT_EntryExists(s, entryname);

    if (!e) return "";
    return e->value;
}

static char * SCRIPT_ParseString(char ** dest, char * p)
{
    int32_t c = 0;
    char ch;

    if (!(*dest))
    {
        // find length
        char *q = p;

        if (*q == '\"')
        {
            // quoted string
            q++;
            while ((ch = *(q++)) && ch != '\"')
            {
                if (ch == '\\')
                {
                    ch = *(q++);

                    if (!ch)
                        break;
                }

                c++;
            }
        }
        else
        {
            while ((ch = *(q++)) && ch != ' ' && ch != '\t')
                c++;
        }

        // allocate
        *dest = (char*)Xcalloc(c+1,sizeof(char));
        c = 0;
    }

    if (*p == '\"')
    {
        // quoted string
        p++;
        while ((ch = *(p++)))
        {
            switch (ch)
            {
            case '\\':
                ch = *(p++);
                switch (ch)
                {
                case 0:   return p;
                case 'n': (*dest)[c++] = '\n'; break;
                case 'r': (*dest)[c++] = '\r'; break;
                case 't': (*dest)[c++] = '\t'; break;
                default:  (*dest)[c++] = ch; break;
                }
                break;
            case '\"':
                (*dest)[c] = 0;
                return p;
            default:
                (*dest)[c++] = ch;
                break;
            }
        }
        if (ch == 0) return p;
    }
    else
    {
        while ((ch = *(p++)))
        {
            if (ch == ' ' || ch == '\t') { (*dest)[c] = 0; break; }
            else (*dest)[c++] = ch;
        }
    }

    return p;
}

int32_t SCRIPT_GetStringPtr(int32_t scripthandle, char const * sectionname, char const * entryname, char ** dest)
{
    ScriptSectionType *s;
    ScriptEntryType *e;
    char *p;

    if (!SC(scripthandle)) return 1;
    if (!SCRIPT(scripthandle,apScript)) return 1;

    s = SCRIPT_SectionExists(scripthandle, sectionname);
    e = SCRIPT_EntryExists(s, entryname);

    //dest[0] = 0;
    if (!e) return 1;

    p = e->value;

    SCRIPT_ParseString(dest, p);

    return 0;
}

int32_t SCRIPT_GetString(int32_t scripthandle, char const * sectionname, char const * entryname, char * dest)
{
    return SCRIPT_GetStringPtr(scripthandle, sectionname, entryname, &dest);
}

int32_t SCRIPT_GetDoubleString(int32_t scripthandle, const char * sectionname, const char * entryname, char * dest1, char * dest2)
{
    ScriptSectionType *s;
    ScriptEntryType *e;
    char *p;

    if (!SC(scripthandle)) return 1;
    if (!SCRIPT(scripthandle,apScript)) return 1;

    s = SCRIPT_SectionExists(scripthandle, sectionname);
    e = SCRIPT_EntryExists(s, entryname);

    //dest1[0] = 0;
    //dest2[0] = 0;
    if (!e) return 1;

    p = e->value;

    p = SCRIPT_ParseString(&dest1, p);

    if (*(p-1) != '\"')
        return 0;

    while (*p == ' ' || *p == '\t') p++;
    if (*p == 0) return 0;

    SCRIPT_ParseString(&dest2, p);

    return 0;
}

int32_t SCRIPT_GetNumber(int32_t scripthandle, const char * sectionname, const char * entryname, int32_t * number)
{
    ScriptSectionType *s;
    ScriptEntryType *e;
    char *p;

    if (!SC(scripthandle)) return 1;
    if (!SCRIPT(scripthandle,apScript)) return 1;

    s = SCRIPT_SectionExists(scripthandle, sectionname);
    e = SCRIPT_EntryExists(s, entryname);

    if (!e) return 1;// *number = 0;
    else
    {
        if (e->value[0] == '0' && e->value[1] == 'x')
        {
            // hex
            *number = strtol(e->value+2, &p, 16);
            if (p == e->value+2 || (*p != 0 && *p != ' ' && *p != '\t')) return 1;
        }
        else
        {
            // decimal
            *number = strtol(e->value, &p, 10);
            if (p == e->value || (*p != 0 && *p != ' ' && *p != '\t')) return 1;
        }
    }

    return 0;
}

int32_t SCRIPT_GetBoolean(int32_t scripthandle, char const * sectionname, char const * entryname, int32_t * boole)
{
    ScriptSectionType *s;
    ScriptEntryType *e;

    if (!SC(scripthandle)) return 1;
    if (!SCRIPT(scripthandle,apScript)) return 1;

    s = SCRIPT_SectionExists(scripthandle, sectionname);
    e = SCRIPT_EntryExists(s, entryname);

    if (!e) return 1;// *boole = 0;
    else
    {
        if (!Bstrncasecmp(e->value, "true", 4)) *boole = 1;
        else if (!Bstrncasecmp(e->value, "false", 5)) *boole = 0;
        else if (e->value[0] == '1' && (e->value[1] == ' ' || e->value[1] == '\t' || e->value[1] == 0)) *boole = 1;
        else if (e->value[0] == '0' && (e->value[1] == ' ' || e->value[1] == '\t' || e->value[1] == 0)) *boole = 0;
    }

    return 0;
}

void SCRIPT_PutSection(int32_t scripthandle, char const * sectionname)
{
    SCRIPT_AddSection(scripthandle, sectionname);
}
void SCRIPT_PutRaw(int32_t scripthandle, char const *sectionname, char const *entryname, char const *raw)
{
    SCRIPT_AddEntry(scripthandle, sectionname, entryname, raw);
}

void SCRIPT_PutString(int32_t scripthandle, char const *sectionname, char const *entryname, const char *string)
{
    const char *q;
    char *raw,*p;
    int32_t len = 3;
    if (!string) string = "";

    for (q=string; *q; q++)
    {
        if (*q == '\r' || *q == '\n' || *q == '\t' || *q == '\\' || *q == '"') len+=2;
        else if (*q >= ' ') len++;
    }
    p = raw = (char *)Xmalloc(len);
    *(p++) = '"';
    for (q=string; *q; q++)
    {
        if (*q == '\r') { *(p++) = '\\'; *(p++) = 'r'; }
        else if (*q == '\n') { *(p++) = '\\'; *(p++) = 'n'; }
        else if (*q == '\t') { *(p++) = '\\'; *(p++) = 't'; }
        else if (*q == '\\' || *q == '"') { *(p++) = '\\'; *(p++) = *q; }
        else if (*q >= ' ') *(p++) = *q;
    }
    *(p++) = '"';
    *p=0;

    SCRIPT_AddEntry(scripthandle, sectionname, entryname, raw);
    Bfree(raw);
}

void SCRIPT_PutDoubleString
(
    int32_t scripthandle,
    const char * sectionname,
    const char * entryname,
    const char * string1,
    const char * string2
)
{
    const char *q;
    char *raw,*p;
    int32_t len = 6;
    if (!string1) string1 = "";
    if (!string2) string2 = "";

    for (q=string1; *q; q++)
    {
        if (*q == '\r' || *q == '\n' || *q == '\t' || *q == '\\' || *q == '"') len+=2;
        else if (*q >= ' ') len++;
    }
    for (q=string2; *q; q++)
    {
        if (*q == '\r' || *q == '\n' || *q == '\t' || *q == '\\' || *q == '"') len+=2;
        else if (*q >= ' ') len++;
    }
    p = raw = (char *)Xmalloc(len);
    *(p++) = '"';
    for (q=string1; *q; q++)
    {
        if (*q == '\r') { *(p++) = '\\'; *(p++) = 'r'; }
        else if (*q == '\n') { *(p++) = '\\'; *(p++) = 'n'; }
        else if (*q == '\t') { *(p++) = '\\'; *(p++) = 't'; }
        else if (*q == '\\' || *q == '"') { *(p++) = '\\'; *(p++) = *q; }
        else if (*q >= ' ') *(p++) = *q;
    }
    *(p++) = '"';
    *(p++) = ' ';
    *(p++) = '"';
    for (q=string2; *q; q++)
    {
        if (*q == '\r') { *(p++) = '\\'; *(p++) = 'r'; }
        else if (*q == '\n') { *(p++) = '\\'; *(p++) = 'n'; }
        else if (*q == '\t') { *(p++) = '\\'; *(p++) = 't'; }
        else if (*q == '\\' || *q == '"') { *(p++) = '\\'; *(p++) = *q; }
        else if (*q >= ' ') *(p++) = *q;
    }
    *(p++) = '"';
    *p=0;

    SCRIPT_AddEntry(scripthandle, sectionname, entryname, raw);
    Bfree(raw);
}

void SCRIPT_PutNumber
(
    int32_t scripthandle,
    const char * sectionname,
    const char * entryname,
    int32_t number,
    int32_t hexadecimal,
    int32_t defaultvalue
)
{
    char raw[64];

    UNREFERENCED_PARAMETER(defaultvalue);
    if (hexadecimal) Bsprintf(raw, "0x%X", number);
    else Bsprintf(raw, "%d", number);

    SCRIPT_AddEntry(scripthandle, sectionname, entryname, raw);
}

void SCRIPT_PutBoolean
(
    int32_t scripthandle,
    char * sectionname,
    char * entryname,
    int32_t boole
)
{
    char raw[2] = "0";

    if (boole) raw[0] = '1';

    SCRIPT_AddEntry(scripthandle, sectionname, entryname, raw);
}

void SCRIPT_PutDouble
(
    int32_t scripthandle,
    char * sectionname,
    char * entryname,
    double number,
    int32_t defaultvalue
)
{
    char raw[64];

    UNREFERENCED_PARAMETER(defaultvalue);
    Bsprintf(raw, "%g", number);

    SCRIPT_AddEntry(scripthandle, sectionname, entryname, raw);
}

