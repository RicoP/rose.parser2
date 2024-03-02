#pragma once

#include <rose/meta.h>
#include <rose/serialize.h>

$(foo_t {})
struct Vector3 {
    float x,y,z;
};

inline RSerializeResult serialize(Vector3 & v, RSerilalizer & serializer) {
    float arr[3] = {v.x, v.y, v.z};
    return serialize(arr, serializer);
}

/*
struct Matrix44 {
    float m11,m12,m13,m14;
    float m21,m22,m23,m24;
    float m31,m32,m33,m34;
    float m41,m42,m43,m44;
};

inline RSerializeResult serialize(Matrix44 & mat, RSerilalizer & serializer) {
    float arr[4][4] = {
        {mat.m11,mat.m12,mat.m13,mat.m14},
        {mat.m21,mat.m22,mat.m23,mat.m24},
        {mat.m31,mat.m32,mat.m33,mat.m34},
        {mat.m41,mat.m42,mat.m43,mat.m44}
    };
    return serialize(arr, serializer);
}
*/
