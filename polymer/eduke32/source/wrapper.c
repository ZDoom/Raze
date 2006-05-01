// this is a wrapper to launch EDuke32 properly from Dukester X
// gcc -o duke3d_w32.exe wrapper.c

#include <windows.h>
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

    for(i=0;i<sizeof(CmdLine);i++)
    {
        if(lpCmdLine[i] == ' ' && lpCmdLine[i+1] == '-' && lpCmdLine[i+2] == 'n' && lpCmdLine[i+3] == 'e' && lpCmdLine[i+4] == 't')
        {
            i += 6;
            j = 0;
            while(!ISWS(lpCmdLine[i]))
            {
                szFileName[j] = lpCmdLine[i];
                j++,i++;
                fprintf(fp,"%d %d\n",j,i);
                if(lpCmdLine[i] == ' ' || lpCmdLine[i] == '\n' || lpCmdLine[i] == '\r')
                    break;
            }
            break;
        }
        else CmdLine[i] = lpCmdLine[i];
    }

    sprintf(sCmdLine,"eduke32.exe %s -rmnet %s",CmdLine,szFileName);
    szCmdLine = sCmdLine;
    fprintf(fp,"EDuke32 wrapper for Dukester X v0.01\
            \nCopyright (c) 2006 EDuke32 team\n\
            \nArgs passed to wrapper: %s\
            \nRancidmeat net filename: %s\
            \nFinal command line: %s\n",lpCmdLine,szFileName,szCmdLine);
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

