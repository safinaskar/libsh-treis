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
