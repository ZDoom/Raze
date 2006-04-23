//placeholder placeholder lol

void    polymer_glinit(void)
{
    bglClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    bglClear(GL_COLOR_BUFFER_BIT);
    bglViewport(0, 0, 1024, 768);
    bglDisable(GL_TEXTURE_2D);
    bglEnable(GL_DEPTH_TEST);
    bglMatrixMode(GL_PROJECTION);
    bglLoadIdentity();
    bglFrustum(-1.0f, 1.0f, -0.75f, 0.75, 1.0f, 100.0f);
    bglMatrixMode(GL_MODELVIEW);
    bglLoadIdentity();
}

void            polymer_drawsector(short sectnum)
{
    sectortype  *sec;
    walltype    *wal;
    
    sec = &sector[sectnum];
    wal = &wall[sec->wallptr];
    OSD_Printf("%i\n", sec->wallnum);
}

void    polymer_drawrooms(void)
{
    OSD_Printf("drawrooms\n");
    polymer_glinit();
    polymer_drawsector(globalcursectnum);
}
