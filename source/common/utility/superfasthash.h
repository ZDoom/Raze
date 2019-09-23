#pragma once
#include <stdint.h>

uint32_t SuperFastHash (const char *data, size_t len);
uint32_t SuperFastHashI (const char *data, size_t len);
unsigned int MakeKey (const char *s);
unsigned int MakeKey (const char *s, size_t len);
