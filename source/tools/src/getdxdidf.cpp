// compile with:
// gcc -o getdxdidf.exe src\getdxdidf.c -Ic:\mingw32\dx6\include -Lc:\mingw32\dx6\lib -ldxguid -ldinput -mwindows

#include "compat.h"

#define NEED_DINPUT_H
#include "windows_inc.h"

char const * WhatGUID(const GUID *guid)
{
    if (guid == &GUID_XAxis) return "&GUID_XAxis";
    if (guid == &GUID_YAxis) return "&GUID_YAxis";
    if (guid == &GUID_ZAxis) return "&GUID_ZAxis";
    if (guid == &GUID_RxAxis) return "&GUID_RxAxis";
    if (guid == &GUID_RyAxis) return "&GUID_RyAxis";
    if (guid == &GUID_RzAxis) return "&GUID_RzAxis";
    if (guid == &GUID_Slider) return "&GUID_Slider";

    if (guid == &GUID_Button) return "&GUID_Button";
    if (guid == &GUID_Key) return "&GUID_Key";

    if (guid == &GUID_POV) return "&GUID_POV";

    if (guid == &GUID_Unknown) return "&GUID_Unknown";

    return "NULL";
}



int WINAPI WinMain(HINSTANCE hInstance ATTRIBUTE((unused)), HINSTANCE hPrevInstance ATTRIBUTE((unused)), LPSTR lpCmdLine ATTRIBUTE((unused)), int nCmdShow ATTRIBUTE((unused)))
{
    FILE *fp;
    DWORD i;

    fp = fopen("didf.txt", "w");
    if (!fp) return -1;
    setvbuf(fp, NULL, _IONBF, 0);


    fprintf(fp,
        "// Keyboard\n"
        "\n"
        "static DIOBJECTDATAFORMAT c_dfDIKeyboard_odf[] = {\n"
        );

    for (i=0; i<c_dfDIKeyboard.dwNumObjs; i++) {
        fprintf(fp,
            "\t{ %s, %lu, 0x%08lu, 0x%08lu },\n",
                WhatGUID(c_dfDIKeyboard.rgodf[i].pguid),
                c_dfDIKeyboard.rgodf[i].dwOfs,
                c_dfDIKeyboard.rgodf[i].dwType,
                c_dfDIKeyboard.rgodf[i].dwFlags
            );
    }
    fprintf(fp,
        "};\n"
        "\n"
        "const DIDATAFORMAT c_dfDIKeyboard = { %lu, %lu, 0x%08lu, %lu, %lu, c_dfDIKeyboard_odf };\n\n",
        c_dfDIKeyboard.dwSize,
            c_dfDIKeyboard.dwObjSize,
            c_dfDIKeyboard.dwFlags,
            c_dfDIKeyboard.dwDataSize,
            c_dfDIKeyboard.dwNumObjs
        );



    fprintf(fp,
        "// Mouse\n"
        "\n"
        "static DIOBJECTDATAFORMAT c_dfDIMouse_odf[] = {\n"
        );

    for (i=0; i<c_dfDIMouse.dwNumObjs; i++) {
        fprintf(fp,
            "\t{ %s, %lu, 0x%08lu, 0x%08lu },\n",
                WhatGUID(c_dfDIMouse.rgodf[i].pguid),
                c_dfDIMouse.rgodf[i].dwOfs,
                c_dfDIMouse.rgodf[i].dwType,
                c_dfDIMouse.rgodf[i].dwFlags
            );
    }
    fprintf(fp,
        "};\n"
        "\n"
        "const DIDATAFORMAT c_dfDIMouse = { %lu, %lu, 0x%08lu, %lu, %lu, c_dfDIMouse_odf };\n\n",
        c_dfDIMouse.dwSize,
            c_dfDIMouse.dwObjSize,
            c_dfDIMouse.dwFlags,
            c_dfDIMouse.dwDataSize,
            c_dfDIMouse.dwNumObjs
        );



    fprintf(fp,
        "// Joystick\n"
        "\n"
        "static DIOBJECTDATAFORMAT c_dfDIJoystick_odf[] = {\n"
        );

    for (i=0; i<c_dfDIJoystick.dwNumObjs; i++) {
        fprintf(fp,
            "\t{ %s, %lu, 0x%08lu, 0x%08lu },\n",
                WhatGUID(c_dfDIJoystick.rgodf[i].pguid),
                c_dfDIJoystick.rgodf[i].dwOfs,
                c_dfDIJoystick.rgodf[i].dwType,
                c_dfDIJoystick.rgodf[i].dwFlags
            );
    }
    fprintf(fp,
        "};\n"
        "\n"
        "const DIDATAFORMAT c_dfDIJoystick = { %lu, %lu, 0x%08lu, %lu, %lu, c_dfDIJoystick_odf };\n\n",
        c_dfDIJoystick.dwSize,
            c_dfDIJoystick.dwObjSize,
            c_dfDIJoystick.dwFlags,
            c_dfDIJoystick.dwDataSize,
            c_dfDIJoystick.dwNumObjs
        );


    fclose(fp);

    return 0;
}
