#pragma once

#include <rose/type.h>

enum class ESerializeStatus : int {
    OK = 0
};

struct RSerializeResult {
    ESerializeStatus status = ESerializeStatus::OK;
    int line = -1;

    static RSerializeResult ok(int line) {
        RSerializeResult result { ESerializeStatus::OK, line };
        return result;
    }
};

struct RSerilalizer {
    virtual ~RSerilalizer() {}
    virtual RSerializeResult serialize_float(float arr, const RTypeInfo &) = 0;
    virtual RSerializeResult serialize_span(float * arr, int size, const RTypeInfo &) = 0;
};

inline RSerializeResult serialize(float * arr, int length, RSerilalizer & serializer, const RTypeInfo & typeinfo) {
    return serializer.serialize_span(arr, length, typeinfo);
}

template<class T, int N>
inline RSerializeResult serialize(T (&arr)[N], RSerilalizer & serializer, const RTypeInfo & typeinfo) {
    return serialize(arr, serializer, typeinfo);
}

template<class T, int N>
inline RSerializeResult serialize(T (&arr)[N], RSerilalizer & serializer) {
    return serialize(arr, serializer, rose_typeof(arr));
}

template<class T>
inline RSerializeResult serialize(T & value, RSerilalizer & serializer) {
    return serialize(value, serializer, rose_typeof(value));
}
