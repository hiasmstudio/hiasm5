#ifndef _DLL_H_
#define _DLL_H_

#if defined(BUILDING_DLL) && defined(_WIN32)
# define DLLIMPORT extern "C" __declspec(dllexport)
#elif defined(BUILDING_DLL)
# define DLLIMPORT extern "C" __attribute__((visibility("default")))
#else
#define DLLIMPORT extern "C"
#endif

#endif
