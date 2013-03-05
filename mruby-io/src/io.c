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

static int mrb_io_modestr_to_flags(mrb_state *mrb, const char *modestr);
static int mrb_io_modenum_to_flags(mrb_state *mrb, int modenum);
static int mrb_io_flags_to_modenum(mrb_state *mrb, int flags);

static int
mrb_io_modestr_to_flags(mrb_state *mrb, const char *mode)
{
  int flags = 0;
  const char *m = mode;

  switch (*m++) {
    case 'r':
      flags |= FMODE_READABLE;
      break;
    case 'w':
      flags |= FMODE_WRITABLE | FMODE_CREATE;
      break;
    case 'a':
      flags |= FMODE_WRITABLE | FMODE_APPEND | FMODE_CREATE;
      break;
    default:
      mrb_raisef(mrb, E_ARGUMENT_ERROR, "illegal access mode %s", mode);
  }

  while (*m) {
    switch (*m++) {
      case 'b':
        flags |= FMODE_BINMODE;
        break;
      case '+':
        flags |= FMODE_READWRITE;
        break;
      case ':':
        /* XXX: PASSTHROUGH*/
      default:
        mrb_raisef(mrb, E_ARGUMENT_ERROR, "illegal access mode %s", mode);
    }
  }

  return flags;
}

static int
mrb_io_modenum_to_flags(mrb_state *mrb, int modenum)
{
  int flags = 0;

  switch (modenum & (O_RDONLY|O_WRONLY|O_RDWR)) {
    case O_RDONLY:
      flags = FMODE_READABLE;
      break;
    case O_WRONLY:
      flags = FMODE_WRITABLE;
      break;
    case O_RDWR:
      flags = FMODE_READWRITE;
      break;
  }

  if (modenum & O_APPEND) {
    flags |= FMODE_APPEND;
  }
  if (modenum & O_TRUNC) {
    flags |= FMODE_TRUNC;
  }
  if (modenum & O_CREAT) {
    flags |= FMODE_CREATE;
  }
#ifdef O_BINARY
  if (modenum & O_BINARY) {
    flags |= FMODE_BINMODE;
  }
#endif

  return flags;
}

static int
mrb_io_flags_to_modenum(mrb_state *mrb, int flags)
{
  int modenum = 0;

  switch(flags & (FMODE_READABLE|FMODE_WRITABLE|FMODE_READWRITE)) {
    case FMODE_READABLE:
      modenum = O_RDONLY;
      break;
    case FMODE_WRITABLE:
      modenum = O_WRONLY;
      break;
    case FMODE_READWRITE:
      modenum = O_RDWR;
      break;
  }

  if (flags & FMODE_APPEND) {
    modenum |= O_APPEND;
  }
  if (flags & FMODE_TRUNC) {
    modenum |= O_TRUNC;
  }
  if (flags & FMODE_CREATE) {
    modenum |= O_CREAT;
  }
#ifdef O_BINARY
  if (flags & FMODE_BINMODE) {
    modenum |= O_BINARY
  }
#endif

  return modenum;
}

static int
mrb_proc_exec(const char *pname)
{
  const char *s;
  s = pname;

  while (*s == ' ' || *s == '\t' || *s == '\n')
    s++;

  if (!*s) {
    errno = ENOENT;
    return -1;
  }

  execl("/bin/sh", "sh", "-c", pname, (char *)NULL);
  return -1;
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
io_open(mrb_state *mrb, mrb_value path, int flags, int perm)
{
  const char *pat;
  int modenum;

  pat = mrb_string_value_cstr(mrb, &path);
  modenum = mrb_io_flags_to_modenum(mrb, flags);

  return open(pat, modenum, perm);
}

#ifndef NOFILE
#define NOFILE 64
#endif

mrb_value
mrb_io_popen_init(mrb_state *mrb, mrb_value io, mrb_value cmd, mrb_value mode, mrb_value opt)
{
  struct mrb_io *fptr;
  const char *pname;
  int pid, flags, fd, write_fd;
  int pr[2], pw[2];
  int doexec;

  pname = mrb_string_value_cstr(mrb, &cmd);
  flags = mrb_io_modestr_to_flags(mrb, mrb_string_value_cstr(mrb, &mode));

  doexec = (strcmp("-", pname) != 0);

  if (((flags & FMODE_READABLE) && pipe(pr) == -1)
      || ((flags & FMODE_WRITABLE) && pipe(pw) == -1)) {
    mrb_sys_fail(mrb, "pipe_open failed.");
    return mrb_nil_value();
  }

  if (!doexec) {
    // XXX
    fflush(stdin);
    fflush(stdout);
    fflush(stderr);
  }

retry:
  switch (pid = fork()) {
    case 0: /* child */
      if (flags & FMODE_READABLE) {
        close(pr[0]);
        if (pr[1] != 1) {
          dup2(pr[1], 1);
          close(pr[1]);
        }
      }
      if (flags & FMODE_WRITABLE) {
        close(pw[1]);
        if (pw[0] != 0) {
          dup2(pw[0], 0);
          close(pw[0]);
        }
      }

      if (doexec) {
        for (fd = 3; fd < NOFILE; fd++) {
          close(fd);
        }
        mrb_proc_exec(pname);
        mrb_raisef(mrb, E_IO_ERROR, "command not found: %s", pname);
        _exit(127);
      }
      return mrb_nil_value();
    case -1: /* error */
      if (errno == EAGAIN) {
        goto retry;
      } else {
        int e = errno;
        if (flags & FMODE_READABLE) {
          close(pr[0]);
          close(pr[1]);
        }
        if (flags & FMODE_WRITABLE) {
          close(pw[0]);
          close(pw[1]);
        }

        errno = e;
        mrb_sys_fail(mrb, "pipe_open failed.");
        return mrb_nil_value();
      }
      break;
    default: /* parent */
      if (pid < 0) {
        mrb_sys_fail(mrb, "pipe_open failed.");
        return mrb_nil_value();
      } else {
        if ((flags & FMODE_READABLE) && (flags & FMODE_WRITABLE)) {
          close(pr[1]);
          fd = pr[0];
          close(pw[0]);
          write_fd = pw[1];
        } else if (flags & FMODE_READABLE) {
          close(pr[1]);
          fd = pr[0];
        } else {
          close(pw[0]);
          fd = pw[1];
        }

        mrb_iv_set(mrb, io, mrb_intern(mrb, "@buf"), mrb_str_new2(mrb, ""));
        mrb_iv_set(mrb, io, mrb_intern(mrb, "@pos"), mrb_fixnum_value(0));

        fptr = mrb_io_alloc(mrb);
        fptr->fd   = fd;
        fptr->fd2  = write_fd;
        fptr->pid  = pid;


        DATA_PTR(io)  = fptr;
        DATA_TYPE(io) = &mrb_io_type;
        return io;
      }
  }

  return mrb_nil_value();
}

mrb_value
mrb_io_s_popen(mrb_state *mrb, mrb_value klass)
{
  mrb_value cmd, io;
  mrb_value mode = mrb_str_new2(mrb, "r");
  mrb_value opt  = mrb_hash_new(mrb);

  mrb_get_args(mrb, "S|SH", &cmd, &mode, &opt);
  io = mrb_io_make(mrb, mrb_class_ptr(klass));

  return mrb_io_popen_init(mrb, io, cmd, mode, opt);
}

static mrb_value
mrb_io_init(mrb_state *mrb, mrb_value io, mrb_value fnum, mrb_value mode)
{
  struct mrb_io *fptr;
  int fd, modenum, flags;

  fd = mrb_fixnum(fnum);
  flags   = mrb_io_modestr_to_flags(mrb, mrb_string_value_cstr(mrb, &mode));

  fptr = (struct mrb_io *)mrb_get_datatype(mrb, io, &mrb_io_type);
  if (fptr) {
    mrb_io_free(mrb, fptr);
  }

  mrb_iv_set(mrb, io, mrb_intern(mrb, "@buf"), mrb_str_new2(mrb, ""));
  mrb_iv_set(mrb, io, mrb_intern(mrb, "@pos"), mrb_fixnum_value(0));

  fptr        = mrb_io_alloc(mrb);
  fptr->fd    = fd;
  fptr->flags = flags;

  DATA_PTR(io)  = fptr;
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
  int fd, flags, perm = -1;

  mrb_get_args(mrb, "S|Sb", &path, &mode, &perm);
  if (mrb_nil_p(mode)) {
    mode = mrb_str_new2(mrb, "r");
  }
  if (perm < 0) {
    perm = 0666;
  }

  flags = mrb_io_modestr_to_flags(mrb, mrb_string_value_cstr(mrb, &mode));
  fd = io_open(mrb, path, flags, perm);

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

  if (fptr->flags & FMODE_SYNC) {
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
    fptr->flags |= FMODE_SYNC;
  } else {
    fptr->flags &= ~FMODE_SYNC;
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
  mrb_define_class_method(mrb, io, "popen",   mrb_io_s_popen,   ARGS_ANY());

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
