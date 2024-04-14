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

    const char * p = content;
    while(crnt_line <= line && *p == 0) {
        if(crnt_line == line) {
            char ministring[2] = "X";
            ministring[0] = *p;

            fputs(ministring, stderr);
        }

        ++p;
        if(*p == '\n' || *p == 0) crnt_line++;
    }

    if(crnt_line > line) {
        fputs("\n", stderr);
        for(int i = col; --col;) {
            fputs(" ", stderr);
        }
        fputs("^", stderr);
    }
}

enum sanitize_error {
    sanitize_error_OK = 0,
    sanitize_error_input_error,
    sanitize_error_buffer_overflow,
    sanitize_error_unexpected_end,

    sanitize_error_unknown
};

enum sanitize_state {
    sanitize_state_default = 0,
    sanitize_state_single_line_comment,
    sanitize_state_multi_line_comment,
    sanitize_state_simple_string,
    sanitize_state_multi_line_string_begin,
    sanitize_state_multi_line_string,
};

inline sanitize_error sanitize_string(char * content, int * pline, int * pcol) {
    enum { TEXT_MEMORY_SIZE = 64 };

    if(content == nullptr) return sanitize_error_input_error;

    char previous_text_ringbuffer[TEXT_MEMORY_SIZE];
    int  previous_text_ringbuffer_size = 0;

    char multiline_string_terminator[TEXT_MEMORY_SIZE];
    int  multiline_string_terminator_size = 0;

    auto mmultilinestringend = [&]() {
        if(previous_text_ringbuffer_size < multiline_string_terminator_size) return false;

        for(int iTerminator = 0; iTerminator != multiline_string_terminator_size; ++iTerminator) {
            int iPreviousText = (iTerminator + previous_text_ringbuffer_size + TEXT_MEMORY_SIZE - multiline_string_terminator_size);
            iPreviousText %= TEXT_MEMORY_SIZE;

            if(previous_text_ringbuffer[iPreviousText] != multiline_string_terminator[iTerminator]) return false;
        }

        return true;
    };

    sanitize_state state = sanitize_state_default;
    const char SPACE = ' ';

    char c_prev = 0;
    char c_crnt = 0;
    char terminator = 0;

    int tmp_integer = 0;
    int & line = pline ? *pline : tmp_integer;
    int & col  = pcol  ? *pcol  : tmp_integer;

    line = 1;
    col = 1;

    for(char * p = content; *p; ++p) {
        c_prev = c_crnt;
        c_crnt = *p;

        if(pline && pcol) {
            col++;
            if(c_crnt == '\n') {
                col = 1;
                line++;
            }
        }

        switch(state) {
            default: return sanitize_error_unknown;
            break; case sanitize_state_default: {
                switch(c_crnt) {
                    default: break;
                    break; case '\"': case '\'':
                        terminator = c_crnt;
                        if(c_prev == 'R') {
                            p[-1] = SPACE;
                            multiline_string_terminator[0] = ')';
                            multiline_string_terminator_size = 1;
                            state = sanitize_state_multi_line_string_begin;
                        }
                        else {
                            state = sanitize_state_simple_string;
                        }
                    break; case '/':
                        if(c_prev == '/') {
                            p[-1] = SPACE;
                            p[0] = SPACE;
                            state = sanitize_state_single_line_comment;
                        }
                    break; case '*':
                        if(c_prev == '/') {
                            p[-1] = SPACE;
                            p[0] = SPACE;
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
                if(*p != '\n') *p = SPACE;
                if(c_crnt == '/' && c_prev == '*') {
                    state = sanitize_state_default;
                }
            }
            break; case sanitize_state_simple_string: {
                if(c_prev == '\\') {
                    *p = '?';
                }
                else if(c_crnt == terminator) {
                    state = sanitize_state_default;
                }
            }
            break; case sanitize_state_multi_line_string_begin: {
                if(*p != '\n') *p = 'X';

                if(c_crnt == '(') {
                    state = sanitize_state_multi_line_string;
                }
                else {
                    if(multiline_string_terminator_size == TEXT_MEMORY_SIZE) {
                        return sanitize_error_buffer_overflow;
                    }

                    multiline_string_terminator[multiline_string_terminator_size] = c_crnt;
                    multiline_string_terminator_size++;
                }
            }
            break; case sanitize_state_multi_line_string: {
                if(c_crnt == '\"' && mmultilinestringend()) {
                    state = sanitize_state_default;
                    continue;
                } else {
                    if(multiline_string_terminator_size == TEXT_MEMORY_SIZE) {
                        return sanitize_error_buffer_overflow;
                    }
                }

                previous_text_ringbuffer[previous_text_ringbuffer_size % TEXT_MEMORY_SIZE] = c_crnt;
                previous_text_ringbuffer_size++;

                if(*p != '\n') *p = 'X';
            }
        }
    }

    if(state != sanitize_state_default) return sanitize_error_unexpected_end;

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
        enum { MIN_BUFFER_SIZE = 1024 };

        for(;;) {
            char * buffer = s_language_buffer ? s_language_buffer + s_language_buffer_end : nullptr;
            int buffer_remaining_length = s_language_buffer_length - s_language_buffer_end;

            // It is OK for buffer to be NULL when buffer_remaining_length is 0.
            int len = snprintf(buffer, buffer_remaining_length, "%s : %s;\n", name, expression);

            if(len >= buffer_remaining_length) {
                int minimum_length = s_language_buffer_length + len;
                if(minimum_length < MIN_BUFFER_SIZE) minimum_length = MIN_BUFFER_SIZE;

                // new buffer size grows by the golden ratio (g = 1.618 ~= 13255/8192) to mimic natural growth.
                // we devide by 8192 and cast to a unsigned long long so the compiler can replace the div by a shift.
                {
                    unsigned long long L = s_language_buffer_length;
                    L *= 13255;
                    L /= 8192;
                    s_language_buffer_length = (int)L;
                }
                if(s_language_buffer_length < minimum_length) s_language_buffer_length = minimum_length;

                s_language_buffer = (char *)realloc(s_language_buffer, s_language_buffer_length);
                //printf("New Buffer Length -> %d\n", s_language_buffer_length);

                // try again
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
        { "float"               ,R"___( ('+' | '-')? /\d?.\d+[f]?/ )___" },
        { "integerdec"          ,R"___( /\d+/ )___" },
        { "integerhex"          ,R"___( "0x"/[0-9A-Fa-f]+/ )___" },
        { "integer"             ,R"___( ('+' | '-')? (<integerhex> | <integerdec>) )___" },
        { "number"              ,R"___( <integer> | <float>  )___" },
        { "character"           ,R"___( /'(\\.|[^'])*'/ )___" },
        { "string"              ,R"___( /"(\\.|[^"])*"/ )___" },
        { "assignment"          ,R"___( '=' <lexp> )___" },

        { "factor"              ,R"___( '(' <lexp> ')'
                                      | <number>
                                      | <character>
                                      | <string>
                                      | <ident> '(' <lexp>? (',' <lexp>)* ')'
                                      | <ident>
                                      )___" },

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
                                      | ';'
                                      )___" },

        { "exp"                 ,R"___( <lexp> '>' <lexp>
                                      | <lexp> '<' <lexp>
                                      | <lexp> ">=" <lexp>
                                      | <lexp> "<=" <lexp>
                                      | <lexp> "!=" <lexp>
                                      | <lexp> "==" <lexp>
                                      )___" },

        { "condition"           ,R"___( <exp> | <ident> | <integer> | '!' <condition> )___" },

        { "whatever"            ,R"___( /[^{}]*/ '{' /[^{}]*/ '}' ';'?
                                      | /[^{}]*/ '{' /[^{}]*/ '}' ','?
                                      | /[^{}]*/ '{' <whatever> (',' <whatever>)* ','? '}'
                                      | /[^{}]*/ '{' <whatever> '}' ';'? ','?
                                      | /([^{};]*;)/ <whatever>*
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
                                            )* /$/
                                            )___" },
    };

    mpc_parser_t * parser_array[array_size(config) + 1];
    for(int i = 0; i != array_size(parser_array); ++i) {
        int max = array_size(config);
        parser_array[i] = i < max ? config[i].parser : nullptr;
    }

    //puts(s_language_buffer);
    //return 0;

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
        print_error_line_col(fileContent, line, col);
        return error;
    }

    //puts(fileContent); return 0;

    if (mpc_parse(filename, fileContent, Smallc, &r)) {
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