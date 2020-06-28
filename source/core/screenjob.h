#pragma once
#include <functional>
#include "dobject.h"

using CompletionFunc = std::function<void(bool)>;

class DScreenJob : public DObject
{
	DECLARE_CLASS(DScreenJob, DObject)
	
public:
	virtual int Frame(uint64_t clock, bool skiprequest) = 0;
};

struct JobDesc
{
	DScreenJob* job;
	void (*postAction)();
	bool ignoreifskipped;
};


void RunScreenJob(JobDesc *jobs, int count, CompletionFunc completion, bool clearbefore = true);

struct AnimSound
{
	int framenum;
	int soundnum;
};

DScreenJob *PlayVideo(const char *filename, const AnimSound *ans = nullptr, const int *frameticks = nullptr);
