/* sidconfig.h (template) */
#ifndef _sidconfig_h_
#define _sidconfig_h_

/* DLL building support on win32 hosts */
#ifndef SID_EXTERN
#   ifdef DLL_EXPORT      /* defined by libtool (if required) */
#       define SID_EXTERN __declspec(dllexport)
#   endif
#   ifdef SID_DLL_IMPORT  /* define if linking with this dll */
#       define SID_EXTERN __declspec(dllimport)
#   endif
#   ifndef SID_EXTERN     /* static linking or !_WIN32 */
#     if defined(__GNUC__) && (__GNUC__ >= 4)
#       define SID_EXTERN __attribute__ ((visibility("default")))
#     else
#       define SID_EXTERN
#     endif
#   endif
#endif

/* Deprecated attributes */
#if defined(_MSCVER)
#  define SID_DEPRECATED __declspec(deprecated)
#elif defined(__GNUC__)
#  define SID_DEPRECATED __attribute__ ((deprecated))
#else
#  define SID_DEPRECATED
#endif


/* Namespace support */
#define SIDPLAYFP_NAMESPACE __sidplayfp__
#ifdef  SIDPLAYFP_NAMESPACE
#   define SIDPLAYFP_NAMESPACE_START \
    namespace SIDPLAYFP_NAMESPACE    \
    {
#   define SIDPLAYFP_NAMESPACE_STOP  \
    }
#else
#   define SIDPLAYFP_NAMESPACE_START
#   define SIDPLAYFP_NAMESPACE_STOP
#endif

#endif /* _sidconfig_h_ */
