class File
  module Constants
    LOCK_SH = 1
    LOCK_EX = 2
    LOCK_NB = 4
    LOCK_UN = 8

    NULL = "/dev/null"

    RDONLY   = 0
    WRONLY   = 1
    RDWR     = 2
    NONBLOCK = 4
    APPEND   = 8

    BINARY   = 0
    SYNC     = 128
    NOFOLLOW = 256
    CREAT    = 512
    TRUNC    = 1024
    EXCL     = 2048

    NOCTTY   = 131072
    DSYNC    = 4194304

    FNM_SYSCASE  = 0
    FNM_NOESCAPE = 1
    FNM_PATHNAME = 2
    FNM_DOTMATCH = 4
    FNM_CASEFOLD = 8
  end
end
