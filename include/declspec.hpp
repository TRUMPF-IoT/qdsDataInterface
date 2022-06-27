#pragma once

#ifdef _MSC_VER
    #ifdef DECLSPEC_EXPORT
      #define DECLSPEC __declspec(dllexport)
    #else
      #define DECLSPEC __declspec(dllimport)
    #endif
#else
  #define DECLSPEC
#endif
