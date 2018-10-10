//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
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

#ifdef _WIN32

#include "compat.h"

#define NEED_SHELLAPI_H
#define NEED_WINSOCK2_H
#define NEED_WS2TCPIP_H
#include "windows_inc.h"

#include "renderlayer.h"

int32_t G_GetVersionFromWebsite(char *buffer)
{
    static int32_t wsainitialized = 0;
    int32_t i=0, j=0, r=0;
    struct sockaddr_in dest_addr;
    struct hostent *h;
    char const *host = "www.eduke32.com";
    char const *req = "GET http://www.eduke32.com/VERSION HTTP/1.0\r\n\r\n\r\n";
    char *tok;
    char tempbuf[2048],otherbuf[16],ver[16];
    SOCKET mysock;
    WSADATA ws;

#ifdef _WIN32
    if (wsainitialized == 0)
    {
        if (WSAStartup(0x101, &ws) == SOCKET_ERROR)
            return 0;

        wsainitialized = 1;
    }
#endif

    if ((h = gethostbyname(host)) == NULL)
    {
        initprintf("Couldn't resolve %s!\n", host);
        return 0;
    }

    dest_addr.sin_addr.s_addr = ((struct in_addr *)(h->h_addr))->s_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(80);

    memset(&(dest_addr.sin_zero), '\0', 8);

    mysock = socket(PF_INET, SOCK_STREAM, 0);

    if (mysock == INVALID_SOCKET)
    {
        WSACleanup();
        return 0;
    }

    initprintf("Connecting to http://%s\n",host);

    if (connect(mysock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) == SOCKET_ERROR)
        goto done;

    i = send(mysock, req, strlen(req), 0);

    if (i == SOCKET_ERROR)
        goto done;

    i = recv(mysock, (char *)&tempbuf, sizeof(tempbuf), 0);

    if (i < 0)
        goto done;

    Bmemcpy(&otherbuf, &tempbuf, sizeof(otherbuf));

    strtok(otherbuf, " ");

    if ((tok = strtok(NULL, " ")) == NULL)
        goto done;

    if (Batol(tok) == 200)
    {
        for (i = 0; (unsigned)i < strlen(tempbuf); i++)  // HACK: all of this needs to die a fiery death; we just skip to the content
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
            strcpy(buffer, ver);
            r = 1;
            goto done;
        }
    }

done:
    closesocket(mysock);
    WSACleanup();

    return r;
}
#endif
