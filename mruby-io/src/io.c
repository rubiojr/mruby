/*
** io.c - IO class
*/

#include "mruby.h"

#include "mruby/hash.h"
#include "mruby/data.h"
#include "mruby/khash.h"
#include "mruby/array.h"
#include "mruby/class.h"
#include "mruby/string.h"
#include "mruby/variable.h"
#include "mruby/ext/io.h"
#include "error.h"

static int
mrb_io_modestr_to_modenum(mrb_state *mrb, const char *mode)
{
  int flags = 0;
  const char *m = mode;

  switch (*m++) {
    case 'r':
      flags |= O_RDONLY;
      break;
    case 'w':
      flags |= O_WRONLY | O_CREAT | O_TRUNC;
      break;
    case 'a':
      flags |= O_WRONLY | O_CREAT | O_APPEND;
      break;
    default:
      goto error;
  }

  while (*m) {
    switch (*m++) {
      case 'b':
#ifdef O_BINARY
        flags |= O_BINARY;
#endif
        break;
      case '+':
        flags = (flags & ~O_ACCMODE) | O_RDWR;
        break;
      case ':':
        /* PASSTHROUGH */
      default:
        goto error;
    }
  }
  return flags;
error:
  mrb_raisef(mrb, E_ARGUMENT_ERROR, "illegal access mode %s", mode);
  /* mrb_raise execute exit(1) */
  return 0; /* UNREACH */
}

static void
mrb_io_free(mrb_state *mrb, void *ptr)
{
  struct mrb_io *io = (struct mrb_io *)ptr;
  if (io != NULL) {
    fptr_finalize(mrb, io, FALSE);
    mrb_free(mrb, io);
  }
}

static struct mrb_data_type mrb_io_type = { "IO", mrb_io_free };

static struct mrb_io*
mrb_io_alloc(mrb_state *mrb)
{
  struct mrb_io *fptr;

  fptr = (struct mrb_io *)mrb_malloc(mrb, sizeof(struct mrb_io));
  fptr->fd       = -1;
  fptr->mode     = 0;
  fptr->pid      = 0;
  fptr->lineno   = 0;
  fptr->finalize = 0;

  return fptr;
}

static mrb_value
mrb_io_wrap(mrb_state *mrb, struct RClass *c, struct mrb_io *io)
{
  return mrb_obj_value(Data_Wrap_Struct(mrb, c, &mrb_io_type, io));
}

static mrb_value
mrb_io_make(mrb_state *mrb, struct RClass *c)
{
  return mrb_io_wrap(mrb, c, mrb_io_alloc(mrb));
}

static int
io_open(mrb_state *mrb, mrb_value path, mrb_value mode, int perm)
{
  const char *pat;
  int flags;

  pat = mrb_string_value_cstr(mrb, &path);
  flags = mrb_io_modestr_to_modenum(mrb, mrb_string_value_cstr(mrb, &mode));

  return open(pat, flags, perm);
}

static mrb_value
mrb_io_init(mrb_state *mrb, mrb_value io, mrb_value fnum, mrb_value mode)
{
  struct mrb_io *fptr;
  int fd, flags;

  fd = mrb_fixnum(fnum);
  flags = mrb_io_modestr_to_modenum(mrb, mrb_string_value_cstr(mrb, &mode));

  fptr = (struct mrb_io *)mrb_get_datatype(mrb, io, &mrb_io_type);
  if (fptr) {
    mrb_io_free(mrb, fptr);
  }

  mrb_iv_set(mrb, io, mrb_intern(mrb, "@buf"), mrb_str_new2(mrb, ""));
  mrb_iv_set(mrb, io, mrb_intern(mrb, "@pos"), mrb_fixnum_value(0));

  fptr = mrb_io_alloc(mrb);
  fptr->mode = flags;
  fptr->fd   = fd;

  DATA_PTR(io) = fptr;
  DATA_TYPE(io) = &mrb_io_type;
  return io;
}

mrb_value
mrb_io_initialize(mrb_state *mrb, mrb_value io)
{
  mrb_value fnum, mode = mrb_nil_value();

  mrb_get_args(mrb, "i|S", &fnum, &mode);
  if (mrb_nil_p(mode)) {
    mode = mrb_str_new2(mrb, "r");
  }
  return mrb_io_init(mrb, io, fnum, mode);
}

void
fptr_finalize(mrb_state *mrb, struct mrb_io *fptr, int noraise)
{
  int n = 0;

  if (fptr == NULL) {
    return;
  }

  if (fptr->fd > 2) {
    n = close(fptr->fd);
    if (n == 0) {
      fptr->fd = -1;
    }
  }

  if (!noraise && n != 0) {
    mrb_sys_fail(mrb, "fptr_finalize failed.");
  }
}

mrb_value
mrb_io_s_sysopen(mrb_state *mrb, mrb_value klass)
{
  mrb_value path = mrb_nil_value();
  mrb_value mode = mrb_nil_value();
  int fd, perm = -1;

  mrb_get_args(mrb, "S|Sb", &path, &mode, &perm);
  if (mrb_nil_p(mode)) {
    mode = mrb_str_new2(mrb, "r");
  }
  if (perm < 0) {
    perm = 0666;
  }

  fd = io_open(mrb, path, mode, perm);

  return mrb_fixnum_value(fd);
}

mrb_value
mrb_io_sysread(mrb_state *mrb, mrb_value io)
{
  struct mrb_io *fptr;
  mrb_value buf = mrb_nil_value();
  int maxlen, ret;

  mrb_get_args(mrb, "i|S", &maxlen, &buf);
  if (maxlen < 0) {
    return mrb_nil_value();
  }

  if (mrb_nil_p(buf)) {
    buf = mrb_str_new(mrb, "", maxlen);
  }
  if (RSTRING_LEN(buf) != maxlen) {
    buf = mrb_str_resize(mrb, buf, maxlen);
  }

  fptr = (struct mrb_io *)mrb_get_datatype(mrb, io, &mrb_io_type);
  ret = read(fptr->fd, RSTRING_PTR(buf), maxlen);
  switch (ret) {
    case 0: /* EOF */
      if (maxlen == 0) {
        buf = mrb_str_new2(mrb, "");
      } else {
        mrb_raise(mrb, E_EOF_ERROR, "sysread failed: End of File");
        return mrb_nil_value();
      }
      break;
    case -1: /* Error */
      mrb_raise(mrb, E_IO_ERROR, "sysread failed");
      return mrb_nil_value();
      break;
    default:
      if (RSTRING_LEN(buf) != ret) {
        buf = mrb_str_resize(mrb, buf, ret);
      }
      break;
  }

  return buf;
}

mrb_value
mrb_io_sysseek(mrb_state *mrb, mrb_value io)
{
  struct mrb_io *fptr;
  int pos, offset, whence = -1;

  mrb_get_args(mrb, "i|i", &offset, &whence);
  if (whence < 0) {
    whence = 0;
  }

  fptr = (struct mrb_io *)mrb_get_datatype(mrb, io, &mrb_io_type);
  pos = lseek(fptr->fd, offset, whence);
  if (pos < 0) {
    mrb_raise(mrb, E_IO_ERROR, "sysseek faield");
    return mrb_nil_value();
  }

  return mrb_fixnum_value(pos);
}

mrb_value
mrb_io_syswrite(mrb_state *mrb, mrb_value io)
{
  struct mrb_io *fptr;
  mrb_value str, buf;
  int length;

  mrb_get_args(mrb, "S", &str);
  if (mrb_type(str) != MRB_TT_STRING) {
    buf = mrb_funcall(mrb, str, "to_s", 0);
  } else {
    buf = str;
  }

  fptr = (struct mrb_io *)mrb_get_datatype(mrb, io, &mrb_io_type);
  length = write(fptr->fd, RSTRING_PTR(buf), RSTRING_LEN(buf));

  return mrb_fixnum_value(length);
}

mrb_value
mrb_io_sync(mrb_state *mrb, mrb_value io)
{
  struct mrb_io *fptr;
  fptr = (struct mrb_io *)mrb_get_datatype(mrb, io, &mrb_io_type);

  if (fptr->mode & FMODE_SYNC) {
    return mrb_true_value();
  }

  return mrb_false_value();
}

mrb_value
mrb_io_set_sync(mrb_state *mrb, mrb_value io)
{
  mrb_value mode;
  struct mrb_io *fptr;
  fptr = (struct mrb_io *)mrb_get_datatype(mrb, io, &mrb_io_type);

  mrb_get_args(mrb, "o", &mode);
  if (mrb_test(mode)) {
    fptr->mode |= FMODE_SYNC;
  } else {
    fptr->mode &= ~FMODE_SYNC;
  }

  return mode;
}

mrb_value
mrb_io_close(mrb_state *mrb, mrb_value io)
{
  struct mrb_io *fptr;
  fptr = (struct mrb_io *)mrb_get_datatype(mrb, io, &mrb_io_type);
  fptr_finalize(mrb, fptr, FALSE);

  return mrb_nil_value();
}

mrb_value
mrb_io_closed(mrb_state *mrb, mrb_value io)
{
  struct mrb_io *fptr;
  fptr = (struct mrb_io *)mrb_get_datatype(mrb, io, &mrb_io_type);

  if (fptr->fd >= 0) {
    return mrb_false_value();
  }

  return mrb_true_value();
}

void
mrb_init_io(mrb_state *mrb)
{
  struct RClass *io;

  io      = mrb_define_class(mrb, "IO", mrb->object_class);
  MRB_SET_INSTANCE_TT(io, MRB_TT_DATA);

  mrb_include_module(mrb, io, mrb_class_get(mrb, "Enumerable")); /* 15.2.20.3 */

  mrb_define_class_method(mrb, io, "sysopen", mrb_io_s_sysopen, ARGS_ANY());

  mrb_define_method(mrb, io, "initialize", mrb_io_initialize,  ARGS_ANY());    /* 15.2.20.5.21 (x)*/
  mrb_define_method(mrb, io, "sysread",    mrb_io_sysread,     ARGS_ANY());
  mrb_define_method(mrb, io, "sysseek",    mrb_io_sysseek,     ARGS_REQ(1));
  mrb_define_method(mrb, io, "syswrite",   mrb_io_syswrite,    ARGS_REQ(1));
  mrb_define_method(mrb, io, "sync",       mrb_io_sync,        ARGS_NONE());   /* 15.2.20.5.18 */
  mrb_define_method(mrb, io, "sync=",      mrb_io_set_sync,    ARGS_REQ(1));   /* 15.2.20.5.19 */
  mrb_define_method(mrb, io, "close",      mrb_io_close,       ARGS_NONE());   /* 15.2.20.5.1 */
  mrb_define_method(mrb, io, "closed?",    mrb_io_closed,      ARGS_NONE());   /* 15.2.20.5.2 */

  mrb_gv_set(mrb, mrb_intern(mrb, "$/"), mrb_str_new_cstr(mrb, "\n"));
}
