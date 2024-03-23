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


inline char* read_file_to_memory(const char *path) {
    FILE *file;
    char *buffer;
    long file_size;

    // Open file
    file = fopen(path, "rb"); // Open in binary mode
    if (file == nullptr) {
        perror("Error opening file");
        return nullptr;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    // Allocate memory for the file content + null terminator
    buffer = (char *)malloc(file_size + 1);
    if (buffer == nullptr) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return nullptr;
    }

    // Read file into memory
    if (fread(buffer, 1, file_size, file) != file_size) {
        fprintf(stderr, "Failed to read file\n");
        free(buffer);
        fclose(file);
        return nullptr;
    }

    // Null terminate the string
    buffer[file_size] = '\0';

    fclose(file);
    return buffer;
}

void print_error_line_col(const char * content, int line, int col) {
    if(line < 1) return;
    if(col < 1) return;

    int crnt_line = 1;
    
    for(const char * p = content; *p; ++p) {
        if(*p == '\n') {
            if(crnt_line == line) {
                fputs("\n", stderr);
                for(int i = col; --col;) {
                    fputs(" ", stderr);
                }
                fputs("^", stderr);
            }
            crnt_line++;
            continue;
        }

        if(crnt_line == line) {
            char ministring[2] = "X";
            ministring[0] = *p;

            fputs(ministring, stderr);
        }
    }
}

enum sanitize_error {
    sanitize_error_OK = 0,
    sanitize_error_input_error,

    sanitize_error_unknown
};

enum sanitize_state {
    sanitize_state_default = 0,
    sanitize_state_single_line_comment,
    sanitize_state_multi_line_comment,
    sanitize_state_simple_string,
    //TODO: Deal with C++ multi line strings: R"XXX( ... )XXX"
    sanitize_state_multi_line_string,
};

inline sanitize_error sanitize_string(char * content, int * pline, int * pcol) {
    if(content == nullptr) return sanitize_error_input_error;

    sanitize_state state = sanitize_state_default;
    const char SPACE = ' ';

    char c_prev = 0;
    char c_crnt = 0;
    char terminator = 0;

    if(pline) *pline = 1;
    if(pcol)  *pcol = 1;

    for(char * p = content; *p; ++p) {
        c_prev = c_crnt;
        c_crnt = *p;

        if(pline && pcol) {
            (*pcol)++;
            if(c_crnt == '\n') {
                (*pline)++;
                *pcol = 1;
            }
        }

        switch(state) {
            default: return sanitize_error_unknown;
            break; case sanitize_state_default: {
                switch(c_crnt) {
                    default: break;
                    break; case '\"': case '\'':
                        terminator = c_crnt; 
                        state = sanitize_state_simple_string;
                    break; case '/': 
                        if(c_prev == '/') { 
                            *(p-1) = SPACE;
                            *p = SPACE;
                            state = sanitize_state_single_line_comment;
                        }
                    break; case '*': 
                        if(c_prev == '/') { 
                            *(p-1) = SPACE;
                            *p = SPACE;
                            state = sanitize_state_multi_line_comment;
                        }
                }
            }
            break; case sanitize_state_single_line_comment: {
                if(c_crnt == '\n') {
                    state = sanitize_state_default;
                } else {
                    *p = SPACE;
                }
            }
            break; case sanitize_state_multi_line_comment: {
                *p = SPACE;
                if(c_crnt == '/' && c_prev == '*') {
                    state = sanitize_state_default;
                }
            }
            break; case sanitize_state_simple_string: {
                if(c_crnt == terminator && c_prev != '\\') {
                    state = sanitize_state_default;
                }
            }
        }
    }

    if(state != sanitize_state_default) return sanitize_error_input_error;
    return sanitize_error_OK;
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
        { "ident"               ,R"XXX( /[a-zA-Z_][a-zA-Z0-9_]*/ )XXX" },
        { "type"                ,R"XXX( <ident> )XXX" },
        { "float"               ,R"XXX( /-?\d?.\d+[f]?/ )XXX" },
        { "integer"             ,R"XXX( /-?\d+/ )XXX" },
        { "number"              ,R"XXX( <float> | <integer> )XXX" },
        { "character"           ,R"XXX( /'(\\.|[^'])*'/ )XXX" },
        { "string"              ,R"XXX( /"(\\.|[^"])*"/ )XXX" },
        { "assignment"          ,R"XXX( '=' <lexp> )XXX" },

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
                                      | "while" '(' <condition> ')' <stmt>
                                      | "do" <stmt> "while" '(' <condition> ')' ';'
                                      | "if"    '(' <condition> ')' <stmt>
                                      | <ident> <assignment> ';'
                                      | "print" '(' <lexp>? ')' ';'
                                      | "return" <lexp>? ';'
                                      | <ident> '(' <ident>? (',' <ident>)* ')' ';' 
                                      | ';' )XXX" },

        { "exp"                 ,R"XXX( <lexp> '>' <lexp>
                                      | <lexp> '<' <lexp>
                                      | <lexp> ">=" <lexp>
                                      | <lexp> "<=" <lexp>
                                      | <lexp> "!=" <lexp>
                                      | <lexp> "==" <lexp> )XXX" },

        { "condition"           ,R"XXX( <exp> | <ident> | <integer> | '!' <condition> )XXX" },

        { "whatever"            ,R"XXX( /([^{};]*;)/ <whatever>* 
                                      | /[^{}]*/ ('{' <whatever> '}')
                                      )XXX" },

        { "typevar"             ,R"XXX( <ident> <assignment>? )XXX" }, 
        { "typeident"           ,R"XXX( <type> <typevar> (',' <typevar>)* )XXX" }, 
        { "declaration"         ,R"XXX( <typeident> ';' )XXX" },
        
        { "args"                ,R"XXX( <typeident>? (',' <typeident>)* )XXX" },
        { "body"                ,R"XXX( '{' <stmt>* '}' )XXX" },
        { "function_ident"      ,R"XXX( <ident> )XXX" },
        { "function_prefix"     ,R"XXX( "inline" | "static" | "constexpr" | "consteval" )XXX" },
        { "function"            ,R"XXX( <function_prefix>* <type> <function_ident> '(' <args> ')' "const"? (';' | '{' <whatever>* '}') )XXX" },

        // Records
        { "record_struct"       ,R"XXX( "struct" )XXX" },
        { "record_class"        ,R"XXX( "class" )XXX" },
        { "record_access"       ,R"XXX( "public:" | "private:" | "protected:" )XXX" },
        { "record_decl"         ,R"XXX( <record_struct> | <record_class> )XXX" },
        { "record_name"         ,R"XXX( <ident> )XXX" },
        { "record_body"         ,R"XXX( '{' (<record_access> | <anno_function> | <function> | <declaration>)* '}' )XXX" },
        { "record"              ,R"XXX( <record_decl> <record_name>? <record_body>? <ident>? ';' )XXX" },

        // Annotations
        { "anno_var"            ,R"XXX( <ident> )XXX" },
        { "anno_value"          ,R"XXX( <lexp> )XXX" },
        { "anno_initalizer"     ,R"XXX( '.'? <anno_var> ('=' <anno_value>)? )XXX" },
        { "anno_name"           ,R"XXX( <type> )XXX" },
        { "anno_type"           ,R"XXX( <anno_name> '{' <anno_initalizer>? (',' <anno_initalizer>)* '}' )XXX" },
        { "annotation"          ,R"XXX( '$' '(' (<anno_type>)? ')' )XXX" },

        // Annotation + construct
        { "anno_function"       ,R"XXX( <annotation> <function> )XXX" },
        { "anno_record"         ,R"XXX( <annotation> <record> )XXX" },

        // Small C++
        { "include1"            ,R"XXX( "#include" '"' /(\\.|[^"])*/ '"' )XXX" },
        { "include2"            ,R"XXX( "#include" '<' /(\\.|[^>])*/ '>' )XXX" },
        { "include"             ,R"XXX( <include1> | <include2> )XXX" },
        { "macro"               ,R"XXX( "#" /[^\r\n]*/ )XXX" },
        { "smallcpp"            ,R"XXX( /^/ ( <include>
                                            | <macro> 
                                            | <declaration>

                                            | <anno_record>
                                            | <record> 
                                            
                                            | <anno_function>
                                            | <function>
                                            )* /$/ )XXX" },
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
        [](auto & v) { return 0 == strcmp(v.name, "smallcpp"); }
    )->parser;

    mpca_lang_arr(MPCA_LANG_DEFAULT, s_language, parser_array);
    mpc_result_t r;

    puts("_________________________________________________");

    const char * filename = "test1.in";
    char * fileContent = read_file_to_memory(filename);

    int line,col;
    if(int error = sanitize_string(fileContent, &line, &col)) {
        fprintf(stderr, "File Content of file %s is not correct: code %d (line %d, col %d)", filename, error, line, col);
        return error;
    }

    if (mpc_parse("test1.in", fileContent, Smallc, &r)) {
        mpc_ast_print((mpc_ast_t *)r.output);
        mpc_ast_delete((mpc_ast_t *)r.output);
    } else {
        mpc_err_print(r.error);
        print_error_line_col(fileContent, r.error->state.row + 1, r.error->state.col + 1);
        mpc_err_delete(r.error);
    }

    mpc_cleanup_arr(parser_array);

    free(fileContent);

    return 0;
}