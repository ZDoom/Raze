
#ifndef __util_h__
#define __util_h__

inline int Min(int a, int b)
{
	if (a < b)
		return a;
	else
		return b;
}

inline int Max(int a, int b)
{
	if (a < b)
		return b;
	else
		return a;
}

#endif
