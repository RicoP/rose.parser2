#pragma once

#include <rose/string.h>

struct RTypeInfo {
    RString name;
};

template<class T>
inline const RTypeInfo & rose_typeof();

template<>
inline const RTypeInfo & rose_typeof<float>() {
    static const RTypeInfo info { "float" };
    return info;
}

template<class T>
inline const RTypeInfo & rose_typeof();

template<class T, int N>
inline const RTypeInfo & rose_typeof() { return rose_typeof<T>(); }

template<class T, int N>
inline const RTypeInfo & rose_typeof(const T (&)[N]) {
    return rose_typeof<T,N>();
}

template<class T, int N>
inline const RTypeInfo & rose_typeof(T (&)[N]) {
    return rose_typeof<T,N>();
}
