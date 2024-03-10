#include "test1.h"
#include <mpc.h>
#include <algorithm>

RSerializeResult foo(Vector3 & v, RSerilalizer & serializer) {
    return serialize(v, serializer);
}

RSerializeResult bar(Matrix44 & m, RSerilalizer & serializer) {
    return serialize(m, serializer);
}

void baz() {
    Vector3 v;
}

extern "C" mpc_err_t *mpca_lang_arr(int flags, const char *language, mpc_parser_t ** array);
extern "C" void mpc_cleanup_arr(mpc_parser_t ** array);

template<class T, int N>
constexpr int array_size(T (&)[N]) { return N; }

int  s_language_length = 0;
char s_language[10 * 1024];

struct MpcConfig {
    const char * name = nullptr;
    const char * expression = nullptr;
    mpc_parser_t * parser = nullptr;

    MpcConfig(const char * name, const char * expression) : name(name), expression(expression) {
        char * buffer = s_language + s_language_length;
        int buffer_length = array_size(s_language) - s_language_length;
        int len = snprintf(buffer, buffer_length, "%s : %s;\n", name, expression);

        if(len >= buffer_length) {
            puts("buffer not big enough!");
            exit(1);
        }

        parser = mpc_new(name);
        s_language_length += len;
    }
};

int main() {
    MpcConfig config[] = {
        { "linecomment"         ,R"XXX( "//" /[^\r\n]*/ )XXX" },
        //{ "multilinecomment"    ,R"XXX( "/*" /[ \w\r\n]*?/ "*/" )XXX" },
        { "comment"             ,R"XXX( <linecomment> )XXX" },
        { "ident"               ,R"XXX( /[a-zA-Z_][a-zA-Z0-9_]*/ )XXX" },
        { "type"                ,R"XXX( <ident> )XXX" },
        { "float"               ,R"XXX( /-?\d?.\d+[f]?/ )XXX" },
        { "integer"             ,R"XXX( /-?\d+/ )XXX" },
        { "number"              ,R"XXX( <float> | <integer> )XXX" },
        { "character"           ,R"XXX( /'.'/ )XXX" },
        { "string"              ,R"XXX( /"(\\.|[^"])*"/ )XXX" },

        { "factor"              ,R"XXX( '(' <lexp> ')'
                                      | <number>
                                      | <character>
                                      | <string>
                                      | <ident> '(' <lexp>? (',' <lexp>)* ')'
                                      | <ident> )XXX" },

        { "term"                ,R"XXX( <factor> (('*' | '/' | '%') <factor>)* )XXX" },
        { "lexp"                ,R"XXX( <term> (('+' | '-') <term>)* )XXX" },

        { "stmt"                ,R"XXX( '{' <stmt>* '}'
                                      | <declaration>
                                      | "while" '(' <exp> ')' <stmt>
                                      | "do" <stmt> "while" '(' <condition> ')' ';'
                                      | "if"    '(' <exp> ')' <stmt>
                                      | <ident> '=' <lexp> ';'
                                      | "print" '(' <lexp>? ')' ';'
                                      | "return" <lexp>? ';'
                                      | <ident> '(' <ident>? (',' <ident>)* ')' ';' )XXX" },

        { "exp"                 ,R"XXX( <lexp> '>' <lexp>
                                      | <lexp> '<' <lexp>
                                      | <lexp> ">=" <lexp>
                                      | <lexp> "<=" <lexp>
                                      | <lexp> "!=" <lexp>
                                      | <lexp> "==" <lexp> )XXX" },

        { "condition"           ,R"XXX( <exp> | <ident> | <integer> )XXX" },

        { "typeident"           ,R"XXX( <type> <ident> )XXX" },
        { "declaration"         ,R"XXX( <typeident> ('=' <number>)? ';' )XXX" },
        { "args"                ,R"XXX( <typeident>? (',' <typeident>)* )XXX" },
        { "body"                ,R"XXX( '{' <stmt>* '}' )XXX" },
        { "function_ident"      ,R"XXX( <ident> )XXX" },
        { "function"            ,R"XXX( <type> <function_ident> '(' <args> ')' (<body> | ';') )XXX" },
        { "include1"            ,R"XXX( "#include" <string> )XXX" },
        { "include2"            ,R"XXX( "#include" '<' /(\\.|[^">])*/ '>' )XXX" },
        { "include"             ,R"XXX( <include1> | <include2> )XXX" },
        { "smallc"              ,R"XXX( /^/ (<include> | <comment> | <declaration> | <function>)* /$/ )XXX" },
    };

    mpc_parser_t * parser_array[array_size(config) + 1];
    for(int i = 0; i != array_size(parser_array); ++i) {
        int max = array_size(config);
        parser_array[i] = i < max ? config[i].parser : nullptr;
    }

    //puts(s_language);

    mpc_parser_t *Smallc = std::find_if(
        std::begin(config),
        std::end(config),
        [](auto & v) { return 0 == strcmp(v.name, "smallc"); }
    )->parser;

    mpca_lang_arr(MPCA_LANG_DEFAULT, s_language, parser_array);
    mpc_result_t r;

    FILE * inputFile = fopen("test1.in", "rb");
    if (mpc_parse_file("test1.in", inputFile, Smallc, &r)) {
        mpc_ast_print((mpc_ast_t *)r.output);
        mpc_ast_delete((mpc_ast_t *)r.output);
    } else {
        mpc_err_print(r.error);
        mpc_err_delete(r.error);
    }

    mpc_cleanup_arr(parser_array);

    fclose(inputFile);

    return 0;
}