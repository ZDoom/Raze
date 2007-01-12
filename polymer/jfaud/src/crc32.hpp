#ifndef __crc32_hpp__
#define __crc32_hpp__

class CRC32
{
private:
	uint32_t sum;
protected:
public:
	CRC32();
	~CRC32();

	void Reset();
	void ProcessBlock(const void *block, unsigned length);
	uint32_t Value() const;
};

#endif
