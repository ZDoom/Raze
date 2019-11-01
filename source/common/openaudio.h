#pragma once

// This really, really needs to be redone in its entirety, the audio lookup code is hideous.
extern FileReader S_OpenAudio(const char *fn, char searchfirst, uint8_t ismusic);
