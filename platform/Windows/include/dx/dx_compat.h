
// MinGW-w64 headers are deficient.
// Here, account for CINTERFACE because they don't.
#ifdef REFGUID
# undef REFGUID
#endif
#ifdef REFIID
# undef REFIID
#endif
#ifdef REFCLSID
# undef REFCLSID
#endif
#ifdef REFFMTID
# undef REFFMTID
#endif

// copied from MinGW's <basetyps.h>
#if defined (__cplusplus) && !defined (CINTERFACE)
# define REFGUID const GUID&
# define REFIID const IID&
# define REFCLSID const CLSID&
# define REFFMTID const FMTID&
#else
# define REFGUID const GUID* const
# define REFIID const IID* const
# define REFCLSID const CLSID* const
# define REFFMTID const FMTID* const
#endif
