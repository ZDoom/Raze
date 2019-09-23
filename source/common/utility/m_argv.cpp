/*
** m_argv.cpp
** Manages command line arguments
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include <string.h>
#include "m_argv.h"

//===========================================================================
//
// FArgs Default Constructor
//
//===========================================================================

FArgs::FArgs()
{
}

//===========================================================================
//
// FArgs Copy Constructor
//
//===========================================================================

FArgs::FArgs(const FArgs &other)
: Argv(other.Argv)
{
}

//===========================================================================
//
// FArgs Argv Constructor
//
//===========================================================================

FArgs::FArgs(int argc, char **argv)
{
	SetArgs(argc, argv);
}

//===========================================================================
//
// FArgs String Argv Constructor
//
//===========================================================================

FArgs::FArgs(int argc, const std::string *argv)
{
	AppendArgs(argc, argv);
}



//===========================================================================
//
// FArgs Copy Operator
//
//===========================================================================

FArgs &FArgs::operator=(const FArgs &other)
{
	Argv = other.Argv;
	return *this;
}

//===========================================================================
//
// FArgs :: SetArgs
//
//===========================================================================

void FArgs::SetArgs(int argc, char **argv)
{
	Argv.resize(argc);
	for (int i = 0; i < argc; ++i)
	{
		Argv[i] = argv[i];
	}
}

//===========================================================================
//
// FArgs :: FlushArgs
//
//===========================================================================

void FArgs::FlushArgs()
{
	Argv.clear();
}

//===========================================================================
//
// FArgs :: CheckParm
//
// Checks for the given parameter in the program's command line arguments.
// Returns the argument number (1 to argc-1) or 0 if not present
//
//===========================================================================

int FArgs::CheckParm(const char *check, int start) const
{
	for (unsigned i = start; i < Argv.size(); ++i)
	{
		if (0 == stricmp(check, Argv[i].c_str()))
		{
			return i;
		}
	}
	return 0;
}

//===========================================================================
//
// FArgs :: CheckParmList
//
// Returns the number of arguments after the parameter (if found) and also
// returns a pointer to the first argument.
//
//===========================================================================

int FArgs::CheckParmList(const char *check, const std::string **strings, int start) const
{
	unsigned int i, parmat = CheckParm(check, start);

	if (parmat == 0)
	{
		if (strings != nullptr)
		{
			*strings = nullptr;
		}
		return 0;
	}
	for (i = ++parmat; i < Argv.size(); ++i)
	{
		if (Argv[i][0] == '-' || Argv[i][1] == '+')
		{
			break;
		}
	}
	if (strings != nullptr)
	{
		*strings = &Argv[parmat];
	}
	return i - parmat;
}

//===========================================================================
//
// FArgs :: CheckValue
//
// Like CheckParm, but it also checks that the parameter has a value after
// it and returns that or nullptr if not present.
//
//===========================================================================

const char *FArgs::CheckValue(const char *check) const
{
	int i = CheckParm(check);

	if (i > 0 && i < (int)Argv.size() - 1)
	{
		i++;
		return Argv[i][0] != '+' && Argv[i][0] != '-' ? Argv[i].c_str() : nullptr;
	}
	else
	{
		return nullptr;
	}
}

//===========================================================================
//
// FArgs :: TakeValue
//
// Like CheckValue, except it also removes the parameter and its argument
// (if present) from argv.
//
//===========================================================================

std::string FArgs::TakeValue(const char *check)
{
	int i = CheckParm(check);
	std::string out;

	if (i > 0 && i < (int)Argv.size())
	{
		if (i < (int)Argv.size() - 1 && Argv[i+1][0] != '+' && Argv[i+1][0] != '-')
		{
			out = Argv[i+1];
			Argv.erase(Argv.begin() + i, Argv.begin() + (i+2));	// Delete the parm and its value.
		}
		else
		{
			Argv.erase(Argv.begin() + i);		// Just delete the parm, since it has no value.
		}
	}
	return out;
}

//===========================================================================
//
// FArgs :: RemoveArg
//
//===========================================================================

void FArgs::RemoveArgs(const char *check)
{
	int i = CheckParm(check);

	if (i > 0 && i < (int)Argv.size() - 1)
	{
		do 
		{
			RemoveArg(i);
		}
		while (Argv[i][0] != '+' && Argv[i][0] != '-' && i < (int)Argv.size() - 1);
	}
}

//===========================================================================
//
// FArgs :: GetArg
//
// Gets the argument at a particular position.
//
//===========================================================================

const char *FArgs::GetArg(int arg) const
{
	return ((unsigned)arg < Argv.size()) ? Argv[arg].c_str() : nullptr;
}

//===========================================================================
//
// FArgs :: GetArgList
//
// Returns a pointer to the std::string at a particular position.
//
//===========================================================================

const std::string *FArgs::GetArgList(int arg) const
{
	return ((unsigned)arg < Argv.size()) ? &Argv[arg] : nullptr;
}

//===========================================================================
//
// FArgs :: NumArgs
//
//===========================================================================

int FArgs::NumArgs() const
{
	return (int)Argv.size();
}

//===========================================================================
//
// FArgs :: AppendArg
//
// Adds another argument to argv. Invalidates any previous results from
// GetArgList().
//
//===========================================================================

void FArgs::AppendArg(std::string arg)
{
	Argv.push_back(arg);
}

//===========================================================================
//
// FArgs :: AppendArgs
//
// Adds an array of std::strings to argv.
//
//===========================================================================

void FArgs::AppendArgs(int argc, const std::string *argv)
{
	if (argv != nullptr && argc > 0)
	{
		for (int i = 0; i < argc; ++i)
		{
			Argv.push_back(argv[i]);
		}
	}
}

//===========================================================================
//
// FArgs :: RemoveArg
//
// Removes a single argument from argv.
//
//===========================================================================

void FArgs::RemoveArg(int argindex)
{
	Argv.erase(Argv.begin() + argindex);
}

//===========================================================================
//
// FArgs :: CollectFiles
//
// Takes all arguments after any instance of -param and any arguments before
// all switches that end in .extension and combines them into a single
// -switch block at the end of the arguments. If extension is nullptr, then
// every parameter before the first switch is added after this -param.
//
//===========================================================================

void FArgs::CollectFiles(const char *param, const char *extension)
{
	std::vector<std::string> work;
	unsigned int i;
	size_t extlen = extension == nullptr ? 0 : strlen(extension);

	// Step 1: Find suitable arguments before the first switch.
	i = 1;
	while (i < Argv.size() && Argv[i][0] != '-' && Argv[i][0] != '+')
	{
		bool useit;

		if (extlen > 0)
		{ // Argument's extension must match.
			size_t len = Argv[i].length();
			useit = (len >= extlen && stricmp(&Argv[i][len - extlen], extension) == 0);
		}
		else
		{ // Anything will do so long as it's before the first switch.
			useit = true;
		}
		if (useit)
		{
			work.push_back(Argv[i]);
			RemoveArg(i);
		}
		else
		{
			i++;
		}
	}

	// Step 2: Find each occurence of -param and add its arguments to work.
	while ((i = CheckParm(param, i)) > 0)
	{
		RemoveArg(i);
		while (i < Argv.size() && Argv[i][0] != '-' && Argv[i][0] != '+')
		{
			work.push_back(Argv[i]);
			RemoveArg(i);
		}
	}

	// Optional: Replace short path names with long path names
#if 0 //def _WIN32
	for (i = 0; i < work.Size(); ++i)
	{
		work[i] = I_GetLongPathName(work[i]);
	}
#endif

	// Step 3: Add work back to Argv, as long as it's non-empty.
	if (work.size() > 0)
	{
		Argv.push_back(param);
		AppendArgs(work.size(), &work[0]);
	}
}

//===========================================================================
//
// FArgs :: GatherFiles
//
// Returns all the arguments after the first instance of -param. If you want
// to combine more than one or get switchless stuff included, you need to
// call CollectFiles first.
//
//===========================================================================

FArgs *FArgs::GatherFiles(const char *param) const
{
	const std::string *files;
	int filecount;

	filecount = CheckParmList(param, &files);
	return new FArgs(filecount, files);
}
