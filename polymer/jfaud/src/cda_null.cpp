#define JFAUD_INTERNAL
#include "cda_null.hpp"

CDA_Null::CDA_Null(const char *name)
{
	
}

CDA_Null::~CDA_Null()
{
}

bool CDA_Null::IsValid() const
{
	return true;
}

bool CDA_Null::PlayTrack(int n)
{
	return false;
}

bool CDA_Null::Pause()
{
	return false;
}

bool CDA_Null::Resume()
{
	return false;
}

JFAudCDA::State CDA_Null::CheckDisc()
{
	return JFAudCDA::NOT_READY;
}

JFAudCDA::State CDA_Null::GetPlayMode()
{
	return JFAudCDA::NOT_READY;
}

