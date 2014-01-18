#include "mruby.h"
#include "mruby/proc.h"
#include "mruby/array.h"
#include "mruby/string.h"
#include "mruby/compile.h"
#include "mruby/dump.h"
#include "mruby/variable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ENABLE_STDIO
static void
p(mrb_state *mrb, mrb_value obj)
{
  obj = mrb_funcall(mrb, obj, "inspect", 0);
  fwrite(RSTRING_PTR(obj), RSTRING_LEN(obj), 1, stdout);
  putc('\n', stdout);
}
#else
#define p(mrb,obj) mrb_p(mrb,obj)
#endif

int
main(int argc, char **argv)
{
  mrb_state *mrb = mrb_open();
  int n = 0;
  int i;
  mrb_value ARGV, MRUBY_BIN;
  mrbc_context *c;
  mrb_value v;

  if (mrb == NULL) {
    fputs("Invalid mrb_state, exiting mruby\n", stderr);
    return EXIT_FAILURE;
  }

  ARGV = mrb_ary_new_capa(mrb, argc);
  for (i = 1; i < argc; i++) {
    mrb_ary_push(mrb, ARGV, mrb_str_new(mrb, argv[i], strlen(argv[i])));
  }
  mrb_define_global_const(mrb, "ARGV", ARGV);

  MRUBY_BIN = mrb_str_new(mrb, argv[0], strlen(argv[0]));
  mrb_define_global_const(mrb, "MRUBY_BIN", MRUBY_BIN);

  c = mrbc_context_new(mrb);

  // load main
  mrb_sym zero_sym = mrb_intern_lit(mrb, "$0");
  mrbc_filename(mrb, c, "-e");
  mrb_gv_set(mrb, zero_sym, mrb_str_new(mrb, "-e", 2));
  v = mrb_load_string_cxt(mrb, "main", c);

  mrbc_context_free(mrb, c);
  if (mrb->exc) {
    if (!mrb_undef_p(v)) {
      mrb_print_error(mrb);
    }
    n = -1;
  }

  mrb_close(mrb);

  return n == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
