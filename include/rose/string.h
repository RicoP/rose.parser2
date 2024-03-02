#pragma once

struct RStringFactor {
    // TODO: right now we only support static strings
};

struct RString {
    union {
        const char * utf8string;
        RStringFactor * factory;
    };

    template<int N>
    RString(const char (&string)[N]) : utf8string(string), bytelength (-(N - 1)) {}

    int length() const { return bytelength < 0 ? -bytelength : bytelength; }

private:
    //negative value indicates static string
    int bytelength = 0;
};
