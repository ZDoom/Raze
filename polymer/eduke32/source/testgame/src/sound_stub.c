
#include "compat.h"

void initsb(char dadigistat, char damusistat, int dasamplerate, char danumspeakers, char dabytespersample, char daintspersec, char daquality)
{
    UNREFERENCED_PARAMETER(dadigistat);
    UNREFERENCED_PARAMETER(damusistat);
    UNREFERENCED_PARAMETER(dasamplerate);
    UNREFERENCED_PARAMETER(danumspeakers);
    UNREFERENCED_PARAMETER(dabytespersample);
    UNREFERENCED_PARAMETER(daintspersec);
    UNREFERENCED_PARAMETER(daquality);
}

void uninitsb(void)
{
}

void setears(int daposx, int daposy, int daxvect, int dayvect)
{
    UNREFERENCED_PARAMETER(daposx);
    UNREFERENCED_PARAMETER(daposy);
    UNREFERENCED_PARAMETER(daxvect);
    UNREFERENCED_PARAMETER(dayvect);
}

void wsayfollow(char const *dafilename, int dafreq, int davol, int *daxplc, int *dayplc, char followstat)
{
    UNREFERENCED_PARAMETER(dafilename);
    UNREFERENCED_PARAMETER(dafreq);
    UNREFERENCED_PARAMETER(davol);
    UNREFERENCED_PARAMETER(daxplc);
    UNREFERENCED_PARAMETER(dayplc);
    UNREFERENCED_PARAMETER(followstat);
}

void wsay(char const *dafilename, int dafreq, int volume1, int volume2)
{
    UNREFERENCED_PARAMETER(dafilename);
    UNREFERENCED_PARAMETER(dafreq);
    UNREFERENCED_PARAMETER(volume1);
    UNREFERENCED_PARAMETER(volume2);
}

void loadwaves(void)
{
}

void loadsong(char const *filename)
{
    UNREFERENCED_PARAMETER(filename);
}

void musicon(void)
{
}

void musicoff(void)
{
}

void refreshaudio(void)
{
}
