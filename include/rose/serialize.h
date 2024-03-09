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

    bool ok() const { return status == ESerializeStatus::OK; }
};

struct RSerilalizer {
    virtual ~RSerilalizer() {}
    virtual RSerializeResult serializeUtf8(const char *, int bytesLength, const RTypeInfo &) = 0;
    virtual RSerializeResult serialize(float, const RTypeInfo &) = 0;
    virtual RSerializeResult serialize_span(float *, int size, const RTypeInfo &) = 0;
    virtual void array_begin(const RTypeInfo &) = 0;
    virtual void array_end(const RTypeInfo &) = 0;
};

inline RSerializeResult serialize(float f, RSerilalizer & serializer, const RTypeInfo & typeinfo) {
    return serializer.serialize(f, typeinfo);
}

inline RSerializeResult serialize(float * arr, int length, RSerilalizer & serializer, const RTypeInfo & typeinfo) {
    return serializer.serialize_span(arr, length, typeinfo);
}

template<class T>
inline RSerializeResult serialize(T * arr, int length, RSerilalizer & serializer, const RTypeInfo & typeinfo) {
    RSerializeResult result;
    serializer.array_begin(typeinfo);
    for(int i = 0; i < length; ++i) {
        result = serialize(arr[i], serializer, typeinfo);
        if(!result.ok()) break;
    }
    serializer.array_end(typeinfo);
    return result;
}

template<class T, int N>
inline RSerializeResult serialize(T (&arr)[N], RSerilalizer & serializer, const RTypeInfo & typeinfo) {
    return serialize(&arr[0], N, serializer, typeinfo);
}

template<class T>
inline RSerializeResult serialize(T & value, RSerilalizer & serializer) {
    return serialize(value, serializer, rose_typeof_discover<T>());
}
