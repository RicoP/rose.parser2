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

// Base template for the class; it simply forwards to the function template.
template<typename T>
struct rose_typeof_array_impl {
    static inline const RTypeInfo & get() {
        return rose_typeof<T>();
    }
};

// Specialization for single-dimensional array types.
template<typename T, int N>
struct rose_typeof_array_impl<T[N]> {
    static inline const RTypeInfo & get() {
        return rose_typeof<T>(); // Deduce to base type
    }
};

// Specialization for two-dimensional array types.
template<typename T, int N, int M>
struct rose_typeof_array_impl<T[N][M]> {
    static inline const RTypeInfo & get() {
        return rose_typeof<T>(); // Deduce to base type
    }
};

template<class T>
inline const RTypeInfo & rose_typeof_discover() {
    return rose_typeof_array_impl<T>::get();
}
