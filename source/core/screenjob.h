#pragma once
#include <functional>
#include "dobject.h"

using CompletionFunc = std::function<void(bool)>;

class DScreenJob : public DObject
{
	DECLARE_CLASS(DScreenJob, DObject)
	//std::function<void(bool skipped)> completion;
	
public:
	virtual int Frame(uint64_t clock, bool skiprequest) = 0;
};


void RunScreenJob(DScreenJob *job, std::function<void(bool skipped)> completion, bool clearbefore = true);
void RunScreenJob(std::function<int(uint64_t, bool)> action,  CompletionFunc completion, bool clearbefore = true);

struct AnimSound
{
	int framenum;
	int soundnum;
};

void PlayVideo(const char *filename, const AnimSound *ans, const int *frameticks, CompletionFunc completion);

// This one is not suitable for ANM and SMK unless a descriptor file format is implemented.
inline void PlayVideo(const char *filename,CompletionFunc completion)
{
	PlayVideo(filename, nullptr, nullptr , completion);
}