// LD_PRELOAD shim for the P1.12 sync-failure receipt (spec
// docs/superpowers/specs/2026-11-28-atomic-short-write.md, section 5 case 5).
// Written by Umar for the spec review (scratchpad umar-p112/shim.c, copied
// verbatim below this header because the scratchpad is not durable).
//
//   SHIM_SYNC_EIO=1  fdatasync and fsync fail with EIO
//   SHIM_CHUNK=1     write(2) on fds > 2 stores at most 1000 bytes per call
//
// Build: gcc -shared -fPIC -o fdatasync_eio_shim.so fdatasync_eio_shim.c -ldl
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
static int armed(void){ return getenv("SHIM_ARM")!=0; }
int fdatasync(int fd){ if (getenv("SHIM_SYNC_EIO")) { write(2,"[shim fdatasync EIO]\n",21); errno=EIO; return -1; } int (*r)(int)=dlsym(RTLD_NEXT,"fdatasync"); return r(fd); }
int fsync(int fd){ if (getenv("SHIM_SYNC_EIO")) { errno=EIO; return -1; } int (*r)(int)=dlsym(RTLD_NEXT,"fsync"); return r(fd); }
ssize_t write(int fd,const void*b,size_t n){ ssize_t (*r)(int,const void*,size_t)=dlsym(RTLD_NEXT,"write");
  if (fd>2 && getenv("SHIM_CHUNK") && n>1000) n=1000; return r(fd,b,n); }
