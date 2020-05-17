#pragma once

//
// Chat and cheat routines
//
struct event;

void CT_Init (void);
bool CT_Responder (event_t* ev);
void CT_Drawer (void);
bool Cheat_Responder (event_t* ev);
  