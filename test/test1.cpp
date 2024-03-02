#include "test1.h"

RSerializeResult foo(Vector3 & v, RSerilalizer & serializer) {
    return serialize(v, serializer);
}

/*
RSerializeResult bar(Matrix44 & m, RSerilalizer & serializer) {
    return serialize(m, serializer);
}
*/

int main() {
    Vector3 v;
    return 0;
}