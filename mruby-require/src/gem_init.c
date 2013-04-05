#include "mruby.h"
#include "mruby/array.h"
#include "mruby/string.h"
#include "mruby/dump.h"
#include "mruby/compile.h"
#include "mruby/proc.h"

#include "opcode.h"

#include <stdio.h>

mrb_value
mrb_yield_internal(mrb_state *mrb, mrb_value b, int argc, mrb_value *argv, mrb_value self, struct RClass *c);

static void
replace_stop_with_return(mrb_state *mrb, mrb_irep *irep)
{
  if (irep->iseq[irep->ilen - 1] == MKOP_A(OP_STOP, 0)) {
    irep->iseq = mrb_realloc(mrb, irep->iseq, (irep->ilen + 1) * sizeof(mrb_code));
    irep->iseq[irep->ilen - 1] = MKOP_A(OP_LOADNIL, 0);
    irep->iseq[irep->ilen] = MKOP_AB(OP_RETURN, 0, OP_R_NORMAL);
    irep->ilen++;
  }
}

static mrb_value
f_compile(mrb_state *mrb0, const char *code, int len, const char *path)
{
  mrb_state *mrb = mrb_open();
  mrbc_context *c;
  mrb_value irep_size;
  mrb_value result = mrb_nil_value();
  int i, ret, irep_len = mrb->irep_len;
  uint8_t *bin = NULL;
  size_t bin_size = 0;

  c = mrbc_context_new(mrb);
  c->no_exec = 1;
  if (path != NULL) {
    c->filename = path;
  }
  irep_size = mrb_load_nstring_cxt(mrb, code, len, c);

  mrb->irep     += mrb_fixnum(irep_size);
  mrb->irep_len -= irep_len;

  ret = mrb_dump_irep(mrb, 0, 1, &bin, &bin_size);
  if (ret == MRB_DUMP_OK) {
    result = mrb_str_new(mrb0, (char *)bin, bin_size);
  }

  mrb->irep     -= mrb_fixnum(irep_size);
  mrb->irep_len += irep_len;
  mrb_close(mrb);

  return result;
}

static mrb_value
mrb_require_compile(mrb_state *mrb, mrb_value self)
{
  char *path_ptr = NULL;
  mrb_value code, path = mrb_nil_value();
  mrb_value bytecode, array;
  int i;

  mrb_get_args(mrb, "S|S", &code, &path);
  if (!mrb_nil_p(path)) {
    path_ptr = mrb_str_to_cstr(mrb, path);
  }

  return f_compile(mrb, mrb_str_to_cstr(mrb, code), RSTRING_LEN(code), path_ptr);
}

static void
f_load(mrb_state *mrb, mrb_value mrb_str) {
  int ai, n;
  const uint8_t *bytecode;

  printf("rite_binary_header size: %d\n", sizeof(struct rite_binary_header));
  bytecode = (uint8_t *) RSTRING_PTR(mrb_str);
  ai = mrb_gc_arena_save(mrb);
  n = mrb_read_irep(mrb, bytecode);
  mrb_gc_arena_restore(mrb, ai);
  printf("mrb_read_irep returns: %d\n", n);

  if (n >= 0) {
    struct RProc *proc;
    mrb_irep *irep = mrb->irep[n];

    replace_stop_with_return(mrb, irep);
    proc = mrb_proc_new(mrb, irep);
    proc->target_class = mrb->object_class;

    ai = mrb_gc_arena_save(mrb);
    mrb_yield_internal(mrb, mrb_obj_value(proc), 0, NULL, mrb_top_self(mrb), mrb->object_class);
    mrb_gc_arena_restore(mrb, ai);
  } else if (mrb->exc) {
    // fail to load
    longjmp(*(jmp_buf*)mrb->jmp, 1);
  }
}

static mrb_value
mrb_require_load_mrb_str(mrb_state *mrb, mrb_value self)
{
  mrb_value code;

  mrb_get_args(mrb, "S", code);
  f_load(mrb, code);

  return mrb_true_value();
}

void
mrb_mruby_require_gem_init(mrb_state *mrb)
{
  struct RClass *krn;
  krn = mrb->kernel_module;

  mrb_define_method(mrb, krn, "compile",      mrb_require_compile,      ARGS_ANY());
  mrb_define_method(mrb, krn, "load_mrb_str", mrb_require_load_mrb_str, ARGS_REQ(1));
}

void
mrb_mruby_require_gem_final(mrb_state *mrb)
{
}
