#include <mpc.c>

#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4047)
#endif 

mpc_err_t *mpca_lang_arr(int flags, const char *language, mpc_parser_t ** array) {
  mpca_grammar_st_t st;
  mpc_input_t *i;
  mpc_err_t *err;

  st.va = &array;
  st.parsers_num = 0;
  st.parsers = NULL;
  st.flags = flags;

  i = mpc_input_new_string("<mpca_lang>", language);
  err = mpca_lang_st(i, &st);
  mpc_input_delete(i);

  free(st.parsers);
  return err;
}

void mpc_cleanup_arr(mpc_parser_t ** array) {
  int i;
  mpc_parser_t * cur;

  for(i = 0; array[i]; ++i) {
    cur = array[i];
    mpc_undefine(cur);
    mpc_delete(cur);
  }
}
