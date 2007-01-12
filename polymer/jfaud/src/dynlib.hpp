#ifndef __dynlib_hpp__
#define __dynlib_hpp__

class DynamicLibrary {
private:
	void *handle;
public:
	DynamicLibrary(const char *name);
	~DynamicLibrary();

	bool IsOpen(void) const { return handle != (void*)0; }

	void * Get(const char *sym);
};

#endif
