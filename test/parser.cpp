#include "test1.h"
#include <mpc.h>
#include <algorithm>

RSerializeResult foo(Vector3 & v, RSerilalizer & serializer) {
    return serialize(v, serializer);
}

RSerializeResult bar(Matrix44 & m, RSerilalizer & serializer) {
    return serialize(m, serializer);
}

int  s_language_length = 0;
char s_language[2048];

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
    Vector3 v;
#if 1
    MpcConfig config[] = {
        { "expression", "<product> (('+' | '-') <product>)*" },
        { "product", "<value>   (('*' | '/')   <value>)*" },
        { "value", "/[0-9]+/ | '(' <expression> ')'" },
        { "maths", "/^/ <expression> /$/" }
    };

    mpc_parser_t * parser_array[array_size(config) + 1];
    for(int i = 0; i != array_size(parser_array); ++i) {
        int max = array_size(config);
        parser_array[i] = i < max ? config[i].parser : nullptr;
    }

    puts(s_language);
    
    mpc_parser_t *Maths = std::find_if(
        std::begin(config), 
        std::end(config), 
        [](auto & v) { return 0 == strcmp(v.name, "maths"); }
    )->parser;

    mpca_lang_arr(MPCA_LANG_DEFAULT, s_language, parser_array);
    const char * input = "(4 * 2 * 11 + 2) - 5";
    mpc_result_t r;

    if (mpc_parse("input", input, Maths, &r)) {
        mpc_ast_print((mpc_ast_t *)r.output);
        mpc_ast_delete((mpc_ast_t *)r.output);
    } else {
        mpc_err_print(r.error);
        mpc_err_delete(r.error);
    }

    //mpc_cleanup(4, config[0].parser, config[1].parser, config[2].parser, config[3].parser);
    mpc_cleanup_arr(parser_array);
#else

    mpc_parser_t *Qscript = mpc_new("qscript");
    mpc_parser_t *Comment = mpc_new("comment");
    mpc_parser_t *Resource = mpc_new("resource");
    mpc_parser_t *Rtype = mpc_new("rtype");
    mpc_parser_t *Rname = mpc_new("rname");
    mpc_parser_t *InnerBlock = mpc_new("inner_block");
    mpc_parser_t *Statement = mpc_new("statement");
    mpc_parser_t *Function = mpc_new("function");
    mpc_parser_t *Parameter = mpc_new("parameter");
    mpc_parser_t *Literal = mpc_new("literal");
    mpc_parser_t *Block = mpc_new("block");
    mpc_parser_t *Seperator = mpc_new("seperator");
    mpc_parser_t *Qstring = mpc_new("qstring");
    mpc_parser_t *SimpleStr = mpc_new("simplestr");
    mpc_parser_t *ComplexStr = mpc_new("complexstr");
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Float = mpc_new("float");
    mpc_parser_t *Int = mpc_new("int");

    mpc_err_t *err = mpca_lang(0, R"LANGUAGE(
        qscript        : /^/ (<comment> | <resource>)* /$/ ;
            comment     : '#' /[^\n]*/ ;
            resource       : '[' (<rtype> <rname>) ']' <inner_block> ;
            rtype       : /[*]*/ ;
            rname       : <qstring> ;

            inner_block    : (<comment> | <statement>)* ;
                statement   : <function> '(' (<comment> | <parameter> | <block>)* ')'  <seperator> ;
                function    : <qstring> ;
                parameter   : (<statement> | <literal>) ;
            literal  : (<number> | <qstring>) <seperator> ;
                block       : '{' <inner_block> '}' ;
                seperator   : ',' | "" ;

            qstring        : (<complexstr> | <simplestr>) <qstring>* ;
                simplestr   : /[a-zA-Z0-9_!@#$^&\*_+\-\.=\/<>]+/ ;
                complexstr  : (/"[^"]*"/ | /'[^']*'/) ;

            number         : (<float> | <int>) ;
                float       : /[-+]?[0-9]+\.[0-9]+/ ;
                int         : /[-+]?[0-9]+/ ;

        )LANGUAGE",
        Qscript, Comment, Resource, Rtype, Rname, InnerBlock, Statement, Function,
        Parameter, Literal, Block, Seperator, Qstring, SimpleStr, ComplexStr, Number,
        Float, Int, NULL);

    if(err) {
        mpc_err_print(err);
        return -1;
    }

    const char * input = "[my_func]\n  echo (a b c)\n";
    mpc_result_t r;

    if (mpc_parse("input", input, Qscript, &r)) {
        mpc_ast_print((mpc_ast_t *)r.output);
        mpc_ast_delete((mpc_ast_t *)r.output);
    } else {
        mpc_err_print(r.error);
        mpc_err_delete(r.error);
    }

    mpc_cleanup(18, Qscript, Comment, Resource, Rtype, Rname, InnerBlock,
    Statement, Function, Parameter, Literal, Block, Seperator, Qstring,
    SimpleStr, ComplexStr, Number, Float, Int);
    
#endif

    return 0;
}