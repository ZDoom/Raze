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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Windows-specific hooks for JonoF's Duke3D port.
*/
//-------------------------------------------------------------------------

#ifdef RENDERTYPEWIN

#define WIN32_LEAN_AND_MEAN
#include "compat.h"
#include <windows.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <shellapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "winlayer.h"


int Win_YesNoBox(char *name, char *fmt, ...)
{
    char buf[1000];
    va_list va;
    int r;

    va_start(va,fmt);
    vsprintf(buf,fmt,va);
    va_end(va);

    r = MessageBox((HWND)win_gethwnd(),buf,name,MB_YESNO|MB_TASKMODAL);
    if (r==IDYES) return 'y';
    return 'n';
}

int Win_MsgBox(char *name, char *fmt, ...)
{
    char buf[1000];
    va_list va;

    va_start(va,fmt);
    vsprintf(buf,fmt,va);
    va_end(va);

    MessageBox((HWND)win_gethwnd(),buf,name,MB_OK|MB_TASKMODAL);
    return 'y';
}


// this replaces the Error() function in jmact/util_lib.c
extern void G_Shutdown(void);	// game.c
void Error(char *error, ...)
{
    char buf[1000];
    va_list va;

    G_Shutdown();

    if (error)
    {
        va_start(va, error);
        vsprintf(buf, error, va);
        va_end(va);

        MessageBox((HWND)win_gethwnd(),buf,"Fatal Error",MB_OK|MB_TASKMODAL);
    }

    exit((error != NULL));
}


#endif

#ifdef _WIN32
int G_GetVersionFromWebsite(char *buffer)
{
    int wsainitialized = 0;
    int bytes_sent, i=0, j=0;
    struct sockaddr_in dest_addr;
    struct hostent *h;
    char *host = "eduke32.sourceforge.net";
    char *req = "GET http://eduke32.sourceforge.net/VERSION HTTP/1.0\r\n\r\n";
    char tempbuf[2048],otherbuf[16],ver[16];
    SOCKET mysock;

#ifdef _WIN32
    if (wsainitialized == 0)
    {
        WSADATA ws;

        if (WSAStartup(0x101,&ws) == SOCKET_ERROR)
        {
//            initprintf("update: Winsock error in G_GetVersionFromWebsite() (%d)\n",errno);
            return(0);
        }
        wsainitialized = 1;
    }
#endif

    if ((h=gethostbyname(host)) == NULL)
    {
//        initprintf("update: gethostbyname() error in G_GetVersionFromWebsite() (%d)\n",h_errno);
        return(0);
    }

    dest_addr.sin_addr.s_addr = ((struct in_addr *)(h->h_addr))->s_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(80);

    memset(&(dest_addr.sin_zero), '\0', 8);


    mysock = socket(PF_INET, SOCK_STREAM, 0);

    if (mysock == INVALID_SOCKET)
    {
//        initprintf("update: socket() error in G_GetVersionFromWebsite() (%d)\n",errno);
        return(0);
    }
    initprintf("Connecting to http://%s\n",host);
    if (connect(mysock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) == SOCKET_ERROR)
    {
        //      initprintf("update: connect() error in G_GetVersionFromWebsite() (%d)\n",errno);
        return(0);
    }

    bytes_sent = send(mysock, req, strlen(req), 0);
    if (bytes_sent == SOCKET_ERROR)
    {
        //    initprintf("update: send() error in G_GetVersionFromWebsite() (%d)\n",errno);
        return(0);
    }

    //    initprintf("sent %d bytes\n",bytes_sent);
    recv(mysock, (char *)&tempbuf, sizeof(tempbuf), 0);
    closesocket(mysock);

    memcpy(&otherbuf,&tempbuf,sizeof(otherbuf));

    strtok(otherbuf," ");
    if (atol(strtok(NULL," ")) == 200)
    {
        for (i=0;(unsigned)i<strlen(tempbuf);i++) // HACK: all of this needs to die a fiery death; we just skip to the content
        {
            // instead of actually parsing any of the http headers
            if (i > 4)
                if (tempbuf[i-1] == '\n' && tempbuf[i-2] == '\r' && tempbuf[i-3] == '\n' && tempbuf[i-4] == '\r')
                {
                    while (j < 9)
                    {
                        ver[j] = tempbuf[i];
                        i++, j++;
                    }
                    ver[j] = '\0';
                    break;
                }
        }

        if (j)
        {
            strcpy(buffer,ver);
            return(1);
        }
    }
    return(0);
}
#endif
