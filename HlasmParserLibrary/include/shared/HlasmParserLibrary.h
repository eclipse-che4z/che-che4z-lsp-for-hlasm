#ifndef __HELLO_H__
#define __HELLO_H__

#include <string>

#if defined(_MSC_VER)
//  Microsoft 
#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
//  GCC
#define EXPORT __attribute__((visibility("default")))
#define IMPORT
#else
//  do nothing and hope for the best?
#define EXPORT
#define IMPORT
#pragma warning Unknown dynamic link import/export semantics.
#endif

namespace HlasmParserLibrary {
    class HlasmParserLibrary
    {
    public:
        void static EXPORT parse(std::string &&);
    };
}


#endif