#pragma once


struct UsermapEntry
{
	FString displayname;
	const char* filename;
	const char* container;
	int size;
};

struct UsermapDirectory
{
	FString name;
	UsermapDirectory* parent = nullptr;
	TArray<UsermapDirectory> subdirectories;
	TArray<UsermapEntry> entries;
};

