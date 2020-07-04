#pragma once

struct cheatseq_t
{
	const char *Sequence;
	const char *Pos;
	bool (*Handler)(cheatseq_t *);
	uint8_t DontCheck;
	// This is working data for processing the cheat
	uint8_t CurrentArg;
	uint8_t Args[6];
}; 
struct event_t;

bool Cheat_Responder(event_t* ev);
void SetCheats(cheatseq_t *cht, int count);

