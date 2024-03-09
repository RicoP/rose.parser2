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

struct Matrix44 {
    float m11,m12,m13,m14;
    float m21,m22,m23,m24;
    float m31,m32,m33,m34;
    float m41,m42,m43,m44;

    Matrix44(RIdentity) {
        m11 = 1; m12 = 0; m13 = 0; m14 = 0;
        m21 = 0; m22 = 1; m23 = 0; m24 = 0;
        m31 = 0; m32 = 0; m33 = 1; m34 = 0;
        m41 = 0; m42 = 0; m43 = 0; m44 = 1;
    }
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

struct Hero {
    Matrix44 transform = identity;
    Vector3 location;
    float life = 0;
};
