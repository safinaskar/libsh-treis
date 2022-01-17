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
namespace libsh_treis::libc::detail //@
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

// gcc 10.2.1 ругается на строчку "extern constinit long (* const syscall_reexported) (long, ...);" в хедере. Говорит, функция не может вернуть constinit. Т. е. gcc считает, что constinit относится к return value, а не к function pointer. Reported: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=104066

//@ typedef long (*syscall_reexported_t) (long, ...);
//@ extern
constinit const syscall_reexported_t syscall_reexported//@;
= &syscall;
} //@
