#pragma once
#include "v_2ddrawer.h"
#include "d_eventbase.h"
#include "s_soundinternal.h"
#include "gamestate.h"
#include "screenjob.h"
#include "gamecontrol.h"

struct CutsceneDef;
struct MapRecord;
struct SummaryInfo;
void PlayLogos(gameaction_t complete_ga, gameaction_t def_ga, bool stopmusic);
void ShowScoreboard(int numplayers, const CompletionFunc& completion_);
void ShowIntermission(MapRecord* fromMap, MapRecord* toMap, SummaryInfo* info, CompletionFunc completion_);
void Local_Job_Init();
