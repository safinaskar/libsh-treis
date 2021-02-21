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

// У функции syscall (man 2 syscall) нет варианта vsyscall, поэтому обернуть её не получится. Поэтому единственный оставшийся метод: использовать указатель на функцию
//@ #include <sys/syscall.h>
#include <unistd.h>
namespace libsh_treis::libc::detail //@
{ //@
//@ extern
constinit long (* const syscall_reexported) (long, ...)//@;
= &syscall;
} //@
