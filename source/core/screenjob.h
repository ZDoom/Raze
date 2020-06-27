#pragma once
#include <functional>
#include "dobject.h"


class DScreenJob : public DObject
{
	DECLARE_CLASS(DScreenJob, DObject)
	//std::function<void(bool skipped)> completion;
	
public:
	virtual int Frame(int clock, bool skiprequest) = 0;
};


void RunScreenJob(DScreenJob *job, std::function<void(bool skipped)> completion, bool clearbefore = true);

struct AnimSound
{
	int framenum;
	int soundnum;
};

void PlayVideo(const char *filename, AnimSound *ans, int frameticks, std::function<void(bool skipped)> completion);

// This one is not suitable for ANM and SMK unless a descriptor file format is implemented.
inline void PlayVideo(const char *filename, std::function<void(bool skipped)> completion)
{
	PlayVideo(filename, nullptr, -1, completion);
}