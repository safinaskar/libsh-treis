//@ #pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "gnu-source.hpp"

// Функции

//@ #include <stdarg.h>
#include <stdio.h>
namespace libsh_treis::libc::detail //@
{ //@
int //@
vasprintf_reexported (char **strp, const char *fmt, va_list ap) noexcept//@;
{
  return vasprintf (strp, fmt, ap);
}
} //@

#include <errno.h>
namespace libsh_treis::detail //@
{ //@
char * //@
program_invocation_name_reexported (void) noexcept//@;
{
  return program_invocation_name;
}
} //@

// У функции syscall (man 2 syscall) нет варианта vsyscall, поэтому обернуть её не получится. Поэтому единственный оставшийся метод таков:
// gnu-source.hpp:
// extern const ... syscall_reexported;
// gnu-source.cpp:
// const ... syscall_reexported = &syscall;
// Но тогда возникнут проблемы с initialization order. Т. е. все сисвызовы, которые мы создадим, будут с пометкой "не вызывать во время инициализации"
// Поэтому единственный метод таков: определить все сисвызовы здесь, в gnu-source.cpp
// bp значит backport

//@ #include <fcntl.h> // For AT_FDCWD
//@ #include <linux/fs.h> // For RENAME_EXCHANGE, ...
#include <unistd.h>
#include <sys/syscall.h>
namespace libsh_treis::libc::bp //@
{ //@
int //@
bp_renameat2 (int olddirfd, const char *oldpath, int newdirfd, const char *newpath, unsigned int flags)//@;
{
  return syscall (SYS_renameat2, olddirfd, oldpath, newdirfd, newpath, flags);
}
} //@
