#pragma once


struct FUsermapEntry
{
	FString displayname;
	FString container;
	FString filename;
	FString info;
	int size;
};

struct FUsermapDirectory
{
	FString dirname;
	FUsermapDirectory* parent = nullptr;
	TArray<FUsermapDirectory> subdirectories;
	TArray<FUsermapEntry> entries;
};

