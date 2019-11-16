//@ #pragma once

#define _GNU_SOURCE
#include "gnu-source.hpp"

// Функции

//@ #include <stdarg.h>
#include <stdio.h>
namespace libsh_treis::libc::detail //@
{ //@
int //@
vasprintf_reexported_nunu (char **strp, const char *fmt, va_list ap) noexcept//@;
{
  return vasprintf (strp, fmt, ap);
}
} //@
