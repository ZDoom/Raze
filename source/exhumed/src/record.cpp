//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 sirlemonhead, Nuke.YKT
This file is part of PCExhumed.
PCExhumed is free software; you can redistribute it and/or
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
#include "ns.h"
#include "compat.h"
#include "engine.h"
#include <stdio.h>
#include <string.h>

BEGIN_PS_NS

short record_mode = 0;
int record_limit = -1;
int record_index = 16384;
uint8_t record_buffer[16384];
FILE *record_file;

struct RecordHeader
{
    char signature[4];
    short b;
};

RecordHeader record_head;


uint8_t GetRecord()
{
    if (record_index >= 16384)
    {
        record_index = 0;

        int nRead = fread(record_buffer, 1, 16384, record_file);

        if (nRead < 16384) {
            record_limit = 16384;
        }
        else {
            record_limit = -1;
        }
    }

    if (record_limit > 0)
    {
        if (record_limit <= record_index)
        {
            record_mode = 3;
        }
    }

    return record_buffer[record_index++];
}

void PutRecord(uint8_t record)
{
    uint8_t val_10 = record;

    if (record_index >= 16384)
    {
        record_index = 0;

        fwrite(record_buffer, 16384, 1, record_file);
    }

    record_buffer[record_index++] = val_10;
}

int OpenRecord(const char *filename, short *edx)
{
    record_file = fopen(filename, "rb");
    if (record_file)
    {
        if (!fread(&record_head, sizeof(record_head), 1, record_file)) {
            return 0;
        }

        if (memcmp(record_head.signature, "LOBO", 4) == 0) {
            return 0;
        }

        *edx = record_head.b;

        record_index = 16384;
        record_limit = -1;
        record_mode  = 2;

        return 1;
    }
    else
    {
        record_file = fopen(filename, "wb");
        if (!record_file) {
            return 0;
        }

        strncpy(record_head.signature, "LOBO", 4);
        record_head.b = *edx;

        if (!fwrite(&record_head, sizeof(record_head), 1, record_file)) {
            return 0;
        }

        record_index = 0;
        record_limit = -1;
        record_mode = 1;

        return 1;
    }
}

int ExecRecord(uint8_t *pRecord, int nSize)
{
    if (record_mode == 2)
    {
        for (int i = 0; i < nSize; i++)
        {
            pRecord[i] = GetRecord();
        }
    }
    else if (record_mode == 1)
    {
        for (int i = 0; i < nSize; i++)
        {
            PutRecord(pRecord[i]);
        }
    }

    if (record_mode == 3) {
        return 0;
    }
    else {
        return 1;
    }
}

int CloseRecord()
{
    if (record_mode == 1)
    {
        //loadgame(0); ???

        if (record_index)
        {
            if (!fwrite(record_buffer, record_index, 1, record_file)) {
                return 0;
            }
        }
    }
    else if (record_mode == 2 || record_mode == 3)
    {
        //loadgame(1); ???
    }

    fclose(record_file);
    return 1;
}
END_PS_NS
