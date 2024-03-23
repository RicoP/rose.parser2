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
    //TODO: Deal with C++ multi line strings: R"___( ... )___"
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

char * s_language_buffer = nullptr;
int    s_language_buffer_length = 0;
int    s_language_buffer_end = 0;

struct MpcConfig {
    const char * name = nullptr;
    const char * expression = nullptr;
    mpc_parser_t * parser = nullptr;

    MpcConfig(const char * name, const char * expression) : name(name), expression(expression) {
        enum { MIN_BUFFER_SIZE = 4 * 1024 };

        for(;;) {
            char * buffer = s_language_buffer ? s_language_buffer + s_language_buffer_end : nullptr;
            int buffer_remaining_length = s_language_buffer_length - s_language_buffer_end;

            // It is OK for buffer to be NULL when buffer_remaining_length is 0.
            int len = snprintf(buffer, buffer_remaining_length, "%s : %s;\n", name, expression);

            if(len >= buffer_remaining_length) {
                int minimum_length = s_language_buffer_length + len;
                if(minimum_length < MIN_BUFFER_SIZE) minimum_length = MIN_BUFFER_SIZE;

                int new_buffer_length = s_language_buffer_length;
                // new buffer size grows by the golden ratio to mimic the Fibonacci sequence...
                new_buffer_length *= 1618;
                new_buffer_length /= 1000;

                if(new_buffer_length < minimum_length) new_buffer_length = minimum_length;

                s_language_buffer_length = new_buffer_length;
                s_language_buffer = (char *)realloc(s_language_buffer, s_language_buffer_length);
                continue;
            }

            s_language_buffer_end += len;
            break;
        }

        parser = mpc_new(name);
    }
};

int main() {
    MpcConfig config[] = {
        { "ident"               ,R"___( /[a-zA-Z_][a-zA-Z0-9_]*/ )___" },
        { "namespace_global"    ,R"___( "::" )___" },
        { "namespace"           ,R"___( <ident> "::" )___" },
        { "type"                ,R"___( <namespace_global>? <namespace>* <ident> )___" },
        { "float"               ,R"___( /-?\d?.\d+[f]?/ )___" },
        { "integer"             ,R"___( /-?\d+/ )___" },
        { "number"              ,R"___( <float> | <integer> )___" },
        { "character"           ,R"___( /'(\\.|[^'])*'/ )___" },
        { "string"              ,R"___( /"(\\.|[^"])*"/ )___" },
        { "assignment"          ,R"___( '=' <lexp> )___" },

        { "factor"              ,R"___( '(' <lexp> ')'
                                      | <number>
                                      | <character>
                                      | <string>
                                      | <ident> '(' <lexp>? (',' <lexp>)* ')'
                                      | <ident> )___" },

        { "term"                ,R"___( <factor> (('*' | '/' | '%') <factor>)* )___" },
        { "lexp"                ,R"___( <term> (('+' | '-') <term>)* )___" },

        { "stmt"                ,R"___( '{' <stmt>* '}'
                                      | <declaration>
                                      | "while" '(' <condition> ')' <stmt>
                                      | "do" <stmt> "while" '(' <condition> ')' ';'
                                      | "if"    '(' <condition> ')' <stmt>
                                      | <ident> <assignment> ';'
                                      | "print" '(' <lexp>? ')' ';'
                                      | "return" <lexp>? ';'
                                      | <ident> '(' <ident>? (',' <ident>)* ')' ';' 
                                      | ';' )___" },

        { "exp"                 ,R"___( <lexp> '>' <lexp>
                                      | <lexp> '<' <lexp>
                                      | <lexp> ">=" <lexp>
                                      | <lexp> "<=" <lexp>
                                      | <lexp> "!=" <lexp>
                                      | <lexp> "==" <lexp> )___" },

        { "condition"           ,R"___( <exp> | <ident> | <integer> | '!' <condition> )___" },

        { "whatever"            ,R"___( /([^{};]*;)/ <whatever>* 
                                      | /[^{}]*/ ('{' <whatever> '}')
                                      )___" },

        { "typevar"             ,R"___( <ident> <assignment>? )___" }, 
        { "typeident"           ,R"___( <type> <typevar> (',' <typevar>)* )___" }, 
        { "declaration"         ,R"___( <typeident> ';' )___" },
        
        { "args"                ,R"___( <typeident>? (',' <typeident>)* )___" },
        { "body"                ,R"___( '{' <stmt>* '}' )___" },
        { "function_ident"      ,R"___( <ident> )___" },
        { "function_prefix"     ,R"___( "inline" | "static" | "constexpr" | "consteval" )___" },
        { "function"            ,R"___( <function_prefix>* <type> <function_ident> '(' <args> ')' "const"? (';' | '{' <whatever>* '}') )___" },

        // Records
        { "record_struct"       ,R"___( "struct" )___" },
        { "record_class"        ,R"___( "class" )___" },
        { "record_access"       ,R"___( "public" | "private" | "protected" )___" },
        { "record_decl"         ,R"___( <record_struct> | <record_class> )___" },
        { "record_name"         ,R"___( <ident> )___" },
        { "record_body"         ,R"___( '{' ( (<record_access>':') | <anno_function> | <function> | <declaration>)* '}' )___" },
        { "record"              ,R"___( <record_decl> <record_name>? <record_body>? <ident>? ';' )___" },

        // Annotations
        { "anno_var"            ,R"___( <ident> )___" },
        { "anno_value"          ,R"___( <lexp> )___" },
        { "anno_initalizer"     ,R"___( '.'? <anno_var> ('=' <anno_value>)? )___" },
        { "anno_name"           ,R"___( <type> )___" },
        { "anno_type"           ,R"___( <anno_name> '{' <anno_initalizer>? (',' <anno_initalizer>)* '}' )___" },
        { "annotation"          ,R"___( '$' '(' (<anno_type>)? ')' )___" },

        // Annotation + construct
        { "anno_function"       ,R"___( <annotation> <function> )___" },
        { "anno_record"         ,R"___( <annotation> <record> )___" },

        // Small C++
        { "include1"            ,R"___( "#include" '"' /(\\.|[^"])*/ '"' )___" },
        { "include2"            ,R"___( "#include" '<' /(\\.|[^>])*/ '>' )___" },
        { "include"             ,R"___( <include1> | <include2> )___" },
        { "macro"               ,R"___( "#" /[^\r\n]*/ )___" },
        { "smallcpp"            ,R"___( /^/ ( <include>
                                            | <macro> 
                                            | <declaration>

                                            | <anno_record>
                                            | <record> 
                                            
                                            | <anno_function>
                                            | <function>
                                            )* /$/ )___" },
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

    mpca_lang_arr(MPCA_LANG_DEFAULT, s_language_buffer, parser_array);
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
    free(s_language_buffer);

    return 0;
}