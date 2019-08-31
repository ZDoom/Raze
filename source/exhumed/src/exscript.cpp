
// Our replacement for the MACT scripting library as the one Exhumed/Powerslave uses is from an older version. This code is based on that older version

#include "typedefs.h"
#include "exscript.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if 0

// 16 bytes in size?
struct Script
{
    char * _0;
    Script *_4;
    Script * _8;
    Script * _12;
};

int ScriptGrabbed = 0;
Script *currentsection = 0;
int scriptline = 0;

Script *script = 0;
uint8_t *scriptbuffer = 0;
uint8_t *script_p = 0;
uint8_t *scriptend_p = 0;
int tokenready = 0;

char scriptfilename[128];


void FreeScriptSection()
{

}

void FreeScript()
{

}

void Error(const char *fmt, ...)
{
    // TODO
    exit(-1);
}

bool TokenAvailable(int nLine)
{
    uint8_t *pOffs = script_p;
    if (pOffs >= scriptend_p) {
        return false;
    }

    while (1)
    {
        char c = *pOffs;

        if (c > ' ')
        {
            if (c != ';') {
                return true;
            }

            while (1)
            {
                c = *pOffs;

                if (c == 0xA) {
                    break;
                }

                if (pOffs >= scriptend_p) {
                    return 0;
                }

                pOffs++;
            }
        }
        else
        {
            if (c == 0x0A && !nLine)
            {
                Error("Line %i is incomplete\nin file %s\n", scriptline, scriptfilename);
            }

            pOffs++;

            if (pOffs >= scriptend_p) {
                return false;
            }
        }
    }
}

void CheckParseOverflow()
{
    if (script_p >= scriptend_p) {
        Error("End of script reached prematurely\n");
    }
}

void SkipWhiteSpace(int nLine)
{
    while (1)
    {
        CheckParseOverflow();

        char c = *script_p;

        if (c > ' ')
        {
            if (c != ';') {
                return;
            }

            while (1)
            {
                c = *script_p;

                if (c == 0xA || script_p >= scriptend_p) {
                    continue;
                }

                script_p++;
            }
        }
        else
        {
            script_p++;
            if (c != 0xA) {
                continue;
            }

            if (!nLine)
            {
                Error("Line %i is incomplete\nin file %s\n");
            }

            scriptline++;
        }
    }
}

void AddScriptEntry(char *entry, Script *pScript)
{
    Script *eax = 0;

    if (!currentsection) {
        Error("No current section adding %s", entry);
    }

    if (currentsection->_4->_8 != currentsection->_4)
    {
        while (1)
        {
            if (stricmp(entry, currentsection->_4->_8->_0) == 0)
            {
                eax = currentsection->_4->_8;
                break;
            }

            if (currentsection->_4->_8->_8 == currentsection->_4)
            {
                break;
            }
        }
    }

    if (!eax)
    {
        Script *pScript = new Script;

        Script *ecx = currentsection->_4;
        pScript->_8 = currentsection->_4;
        pScript->_12 = ecx->_12;

        ecx = currentsection->_4;
        ecx->_12->_8 = pScript;
        currentsection->_4->_12 = pScript;
        pScript->_0 = entry;
        pScript->_4 = pScript;
    }
    else
    {
        eax = 0;

        if (currentsection->_4->_8 != currentsection->_4)
        {
            while (1)
            {
                if (stricmp(entry, currentsection->_4->_8->_0) == 0)
                {
                    eax = currentsection->_4->_8;
                    break;
                }

                if (currentsection->_4->_8->_8 == currentsection->_4) {
                    break;
                }
            }
        }
        
        eax->_0 = entry;
        eax->_4 = pScript;
    }
}

void AddScriptSection(char *section)
{
    Script *eax = 0;

    if (script->_8 != script)
    {
        while (1)
        {
            if (stricmp(section, script->_8->_0))
            {
                if (script->_8 == script) {
                    eax = 0;
                    break;
                }
            }
            else {
                eax = script->_8;
                break;
            }
        }
    }

    if (!eax)
    {
        Script *pNew = new Script;
        pNew->_8 = script;
        Script *ebx = script->_12;
        pNew->_12 = ebx;
        ebx->_8 = pNew;
        script->_12 = pNew;
        pNew->_0 = section;

        pNew->_4 = new Script;
        pNew->_4->_12 = pNew->_4;
        pNew->_4->_8 = pNew->_4;
    }

    eax = 0;

    if (script->_8 != script)
    {
        while (1)
        {
            if (!stricmp(section, script->_8->_0))
            {
                eax = script->_8;
                break;
            }

            if (script->_8->_8 == script)
            {
                break;
            }
        }
    }

    currentsection = eax;
}

void DecodeToken()
{
    char c = *script_p;

    if (c == '[')
    {
        char *pSection = (char*)script_p;

        while (1)
        {
            c = *script_p;

            if (c <= ' ' || c == '=') {
                break;
            }

            script_p++;
            CheckParseOverflow();
        }

        c = *script_p;

        if (c != '=')
        {
            script_p = '\0';
            SkipWhiteSpace(1);

            c = *script_p;
            if (c != '=')
            {
                Error("No entry separator found for %s\n", pSection);
            }

            script_p = '\0';
            SkipWhiteSpace(1);

            AddScriptEntry(pSection, script_p);

            while (1)
            {

            }
        }
    }
    else
    {
        script_p++;
        char *pSection = (char*)script_p;

        while (1)
        {
            c = *script_p;

            if (c != ']')
            {
                if (c == 0xA) {
                    Error("No matching bracket found for section %s", pSection);
                }

                script_p++;
                CheckParseOverflow();
            }
            else
            {
                script_p = '\0';

                AddScriptSection(pSection);

                while (1)
                {
                    c = *script_p;

                    if (c == 0xA) {
                        return;
                    }

                    if (script_p >= scriptend_p) {
                        return;
                    }

                    script_p++;
                }
            }
        }
    }
}

void LoadScript(char *filename, int nVal)
{
    if (!ScriptGrabbed) {
        FreeScript();
    }

    script = new Script;
    currentsection = 0;
    script->_12 = script;
    script->_8  = script;

    int nScriptSize = 0;
//	LoadFile(filename);
    {
        FILE *fp = fopen(filename, "rb");
        if (!fp) {
            // TODO - do error message
            return;
        }

        fseek(fp, 0, SEEK_END);
        nScriptSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        scriptbuffer = new uint8_t[nScriptSize];
        if (!scriptbuffer) {
            // TODO - do error message
            return;
        }

        fread(scriptbuffer, 1, nScriptSize, fp);
        fclose(fp);
    }

    strcpy(scriptfilename, filename);


    scriptline = 1;
    script_p = scriptbuffer;
    tokenready = 0;
    scriptend_p = scriptbuffer + nScriptSize;

    int edx = 0;

    if (nVal)
    {
        int nLine = 1;

        while (1)
        {
            if (edx) {
                return;
            }

            if (TokenAvailable(nLine))
            {
                SkipWhiteSpace(nLine);
                DecodeToken();
            }
            else
            {
                edx = nLine;
            }
        }
    }
}

#endif
