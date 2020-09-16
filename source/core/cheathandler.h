#pragma once

struct cheatseq_t
{
	const char *Sequence;
	const char* ccmd;
	bool (*Handler)(cheatseq_t *);
	uint8_t DontCheck;
	uint8_t Param;
	// This is working data for processing the cheat
	uint8_t CurrentArg;
	uint8_t Args[6];
	const char *Pos;
}; 
struct event_t;

bool Cheat_Responder(event_t* ev);
void SetCheats(cheatseq_t *cht, int count);
void PlaybackCheat(const char* p);
bool SendGenericCheat(cheatseq_t* cheat);

