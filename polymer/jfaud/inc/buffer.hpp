#ifndef __buffer_hpp__
#define __buffer_hpp__

class Buffer {
public:
	typedef enum { TYPE_PCM, TYPE_MIDI } Type;

private:
protected:
public:
	Buffer() { }
	virtual ~Buffer() { }

	virtual Type GetType() const = 0;
};

#endif
