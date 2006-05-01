// this is a wrapper to launch EDuke32 properly from Dukester X
// gcc -o duke3d_w32.exe wrapper.c

#include <windows.h>
#include <string.h>
#include <stdio.h>

#define ISWS(x) ((x == ' ') || (x == '\t') || (x == '\r') || (x == '\n'))

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
    int i,j;
    LPTSTR szCmdLine;

    char CmdLine[1024];
    char sCmdLine[1024];
    char szFileName[255];

    FILE * fp=fopen("wrapper.log","w");
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&szFileName,sizeof(szFileName));
    strcpy(sCmdLine,lpCmdLine);
    for(i=0;i<(signed)strlen(sCmdLine);i++)
    {
        if(sCmdLine[i] == '-' && sCmdLine[i+1] == 'n' && sCmdLine[i+2] == 'e' && sCmdLine[i+3] == 't')
        {
            j = 0,i += 5;
            while(!sCmdLine[i] != ' ' && i<(signed)strlen(sCmdLine))
            {
                szFileName[j] = sCmdLine[i];
                if(sCmdLine[i+1] == ' ')
                    break;
                j++,i++;
            }
            break;
        }
        else CmdLine[i] = sCmdLine[i];
    }

    if(szFileName[0])
        sprintf(sCmdLine,"eduke32.exe %s -rmnet %s",CmdLine,szFileName);
    else sprintf(sCmdLine,"eduke32.exe %s",CmdLine);

    szCmdLine = sCmdLine;

    fprintf(fp,"EDuke32 wrapper for Dukester X v0.01\n");
    fprintf(fp,"Copyright (c) 2006 EDuke32 team\n\n");
    fprintf(fp,"Args passed to wrapper: %s\n",lpCmdLine);
    fprintf(fp,"Final command line: %s",szCmdLine);

    fclose(fp);

    ZeroMemory(&si,sizeof(si));
    ZeroMemory(&pi,sizeof(pi));
    si.cb = sizeof(si);

    if (!CreateProcess(NULL,szCmdLine,NULL,NULL,0,0,NULL,NULL,&si,&pi)) {
        MessageBox(0,"Failed to start eduke32.exe.", "Failure starting game", MB_OK|MB_ICONSTOP);
        return 1;
    }
    return 0;
}

