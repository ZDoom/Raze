#pragma once
#include <functional>
#include "dobject.h"
#include "v_2ddrawer.h"
#include "d_eventbase.h"
#include "s_soundinternal.h"
#include "gamestate.h"

using CompletionFunc = std::function<void(bool)>;

void Job_Init();

enum
{
	SJ_BLOCKUI = 1,
};

void EndScreenJob();
void DeleteScreenJob();
bool ScreenJobResponder(event_t* ev);
bool ScreenJobTick();
void ScreenJobDraw();

struct CutsceneDef;
struct MapRecord;
struct SummaryInfo;
bool StartCutscene(const char* s, int flags, const CompletionFunc& completion);
void PlayLogos(gameaction_t complete_ga, gameaction_t def_ga, bool stopmusic);
void ShowScoreboard(int numplayers, const CompletionFunc& completion_);
void ShowIntermission(MapRecord* fromMap, MapRecord* toMap, SummaryInfo* info, CompletionFunc completion_);
