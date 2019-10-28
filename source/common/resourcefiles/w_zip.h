#ifndef __W_ZIP
#define __W_ZIP

#ifdef __GNUC__
// With versions of GCC newer than 4.2, it appears it was determined that the
// cost of an unaligned pointer on PPC was high enough to add padding to the
// end of packed structs.  For whatever reason __packed__ and pragma pack are
// handled differently in this regard. Note that this only needs to be applied
// to types which are used in arrays.
#define FORCE_PACKED __attribute__((__packed__))
#else
#define FORCE_PACKED
#endif


#pragma pack(1)
// FZipCentralInfo
struct FZipEndOfCentralDirectory
{
	uint32_t	Magic;
	uint16_t	DiskNumber;
	uint16_t	FirstDisk;
	uint16_t	NumEntries;
	uint16_t	NumEntriesOnAllDisks;
	uint32_t	DirectorySize;
	uint32_t	DirectoryOffset;
	uint16_t	ZipCommentLength;
} FORCE_PACKED;

// FZipFileInfo
struct FZipCentralDirectoryInfo
{
	uint32_t	Magic;
	uint8_t	VersionMadeBy[2];
	uint8_t	VersionToExtract[2];
	uint16_t	Flags;
	uint16_t	Method;
	uint16_t	ModTime;
	uint16_t	ModDate;
	uint32_t	CRC32;
	uint32_t	CompressedSize;
	uint32_t	UncompressedSize;
	uint16_t	NameLength;
	uint16_t	ExtraLength;
	uint16_t	CommentLength;
	uint16_t	StartingDiskNumber;
	uint16_t	InternalAttributes;
	uint32_t	ExternalAttributes;
	uint32_t	LocalHeaderOffset;
	// file name and other variable length info follows
} FORCE_PACKED;

// FZipLocalHeader
struct FZipLocalFileHeader
{
	uint32_t	Magic;
	uint8_t	VersionToExtract[2];
	uint16_t	Flags;
	uint16_t	Method;
	uint16_t	ModTime;
	uint16_t	ModDate;
	uint32_t	CRC32;
	uint32_t	CompressedSize;
	uint32_t	UncompressedSize;
	uint16_t	NameLength;
	uint16_t	ExtraLength;
	// file name and other variable length info follows
} FORCE_PACKED;


#pragma pack()

#define ZIP_LOCALFILE	MAKE_ID('P','K',3,4)
#define ZIP_CENTRALFILE	MAKE_ID('P','K',1,2)
#define ZIP_ENDOFDIR	MAKE_ID('P','K',5,6)

// File header flags.
#define ZF_ENCRYPTED			0x1

#endif
