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

int  s_language_length = 0;
char s_language[10 * 1024];

struct MpcConfig {
    const char * name;
    const char * expression;
    mpc_parser_t * parser = nullptr;

    MpcConfig(const char * name, const char * expression) : name(name), expression(expression) {
        int len = sprintf(s_language + s_language_length, "%s : %s;\n", name, expression);
        parser = mpc_new(name);
        s_language_length += len;
    }
};

extern "C" mpc_err_t *mpca_lang_arr(int flags, const char *language, mpc_parser_t ** array);
extern "C" void mpc_cleanup_arr(mpc_parser_t ** array);

template<class T, int N>
constexpr int array_size(T (&)[N]) { return N; }

int main() {
    MpcConfig config[] = {
        { "ident"     , R"XXX( /[a-zA-Z_][a-zA-Z0-9_]*/ )XXX" },
        { "number"    , R"XXX( /[0-9]+/ )XXX" },
        { "character" , R"XXX( /'.'/ )XXX" },
        { "string"    , R"XXX( /"(\\.|[^"])*"/ )XXX" },

        { "factor"    , R"XXX( '(' <lexp> ')'
                | <number>
                | <character>
                | <string>
                | <ident> '(' <lexp>? (',' <lexp>)* ')'
                | <ident> )XXX" },

        { "term"      , R"XXX( <factor> (('*' | '/' | '%') <factor>)* )XXX" },
        { "lexp"      , R"XXX( <term> (('+' | '-') <term>)* )XXX" },

        { "stmt"      , R"XXX( '{' <stmt>* '}'
                | "while" '(' <exp> ')' <stmt>
                | "if"    '(' <exp> ')' <stmt>
                | <ident> '=' <lexp> ';'
                | "print" '(' <lexp>? ')' ';'
                | "return" <lexp>? ';'
                | <ident> '(' <ident>? (',' <ident>)* ')' ';' )XXX" },

        { "exp"       , R"XXX( <lexp> '>' <lexp>
                | <lexp> '<' <lexp>
                | <lexp> ">=" <lexp>
                | <lexp> "<=" <lexp>
                | <lexp> "!=" <lexp>
                | <lexp> "==" <lexp> )XXX" },

        { "typeident" , R"XXX( ("int" | "char") <ident> )XXX" },
        { "decls"     , R"XXX( (<typeident> ';')* )XXX" },
        { "args"      , R"XXX( <typeident>? (',' <typeident>)* )XXX" },
        { "body"      , R"XXX( '{' <decls> <stmt>* '}' )XXX" },
        { "procedure" , R"XXX( ("int" | "char") <ident> '(' <args> ')' <body> )XXX" },
        { "main"      , R"XXX( "main" '(' ')' <body> )XXX" },
        { "includes"  , R"XXX( ("#include" <string>)* )XXX" },
        { "smallc"    , R"XXX( /^/ <includes> <decls> <procedure>* <main> /$/ )XXX" },
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