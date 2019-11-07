// libsh-treis - собрание C++-библиотек (проект из C++-библиотек) с обёртками над C-библиотеками
// Одновременно libsh-treis - это главная библиотека в проекте, которая оборачивает libc в Linux (пространство имён libsh-treis::libc)
// Весь проект предназначен только для Linux
// Цель проекта: бросать исключения в случае ошибок и использовать RAII
// 2018DECTMP

// Сборка этой либы
// - Написан на C++17, проекты на этой либе тоже должны использовать как минимум C++17
// - Зависит от boost stacktrace. Причём от той версии, где поддерживается BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE и где есть boost::stacktrace::to_string (const boost::stacktrace::stacktrace &). В частности, 1.71 поддерживается, а 1.67 - нет
// - Желательно указать переменную сборки к make BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE для включения более информативного backtrace'а

// Библиотека следует следующим соглашениям, они рекомендуются и для других проектов:
// - Клонировать нужно с --recurse-submodules
// - Собирать нужно с помощью make
// - make понимает переменные make'а CXX, CPPFLAGS, CXXFLAGS и LDFLAGS и пробрасывает их рекурсивно
// - Final executable собирается со всеми *.o, которые есть в каталоге, и с линковкой всех либ, которые есть во всех libs, которые есть в каталоге
// - Чтобы не сломать Makefile, libs должен быть как минимум один в каждой либе
// - make работает корректно, не смотря на рекурсию
// - Либа должна "touch stamp", если она обновила любой из .o'шников

// Сборка с либой
// - Все файлы, которые линкуются с либой (и саму либу), рекомендуется собирать с -O0 -g, чтобы работал backtrace
// - При сборке с этой либой указывайте в #include правильный относительный путь. Тогда не надо никаких флагов -I

// Runtime
// - Чтобы backtrace работал хорошо, /proc должен быть смонтирован

// Про саму либу
// - В пространстве libsh-treis::libc::no_raii бросаются исключения в случае ошибок, но есть нарушения RAII
// - В пространстве libsh-treis::libc (без no_raii) бросаются исключения в случае ошибок и RAII не нарушается
// - Сигнатура оборачиваемой функции остаётся без изменений, за исключением тривиальных, т. е. если исходная функция при успешном исходе всегда возвращает одно и то же, то нужно возвращать void либо сделать один из out-параметров выходным
// - Другие причины смены сигнатуры недопустимы. В частности, нельзя менять один не-void тип результата на другой. Например, read и write всегда возращают неотрицательное число в случае успеха, поэтому их тип результата можно было бы заменить с ssize_t на size_t. Но я не буду этого делать, т. к. беззнаковые типы - это плохо (см. "ES.107: Don't use unsigned for subscripts, prefer gsl::index" в C++ Core Guidelines)
// - Но иногда сменить тип результата всё-таки можно, именно так сделано у xxfgetc
// - Убираем <stdarg.h> там, где это возможно. См., например, xopen2 и xopen3
// - Мы понимаем ошибку в максимально прямом и естественном смысле, без фантазирования. Мы не называем ошибкой то, что ей не является. Например, если getchar вернул EOF из-за конца файла, то это не ошибка (это следует из здравого смысла). В то же время в некоторых случаях мы упрощаем себе задачу. Например, большинство системных вызовов Linux (а точнее, их обёрток из libc) возвращают -1 в случае ошибки. Такие ошибки мы и вправду считаем ошибками, чтобы упростить себе задачу, в том числе считаем ошибками всякие там EAGAIN, EINTR, EINPROGRESS, EWOULDBLOCK и тому подобные
// - Если кроме естественных ошибок хочется бросать исключения при дополнительных, заводим xx-функцию. Например, xxgetchar бросает исключение при EOF
// - Семантика функций из C headers и соответствующих C++ headers может различаться очень существенно. Например, std::exit из <cstdlib> гарантирует вызов деструкторов для глобальных объектов, а exit из <stdlib.h> - нет. Тем не менее, реализация в Linux хорошая, т. е. std::exit из <cstdlib> просто ссылается на exit из <stdlib.h>. То же для функций из <string.h> и <cstring>. Поэтому всегда используем .h-файлы. Так проще, не надо различать хедеры из стандарта C++ и Linux-специфичные
// - .hpp инклудит только то, что нужно, чтобы сам .hpp работал. .cpp инклудит то, что нужно для реализации. Потом, когда-нибудь, наверное, нужно будет ещё и инклудить то, что нужно для правильного использования, скажем, чтобы было O_CREAT к open (но конкретно в случае open хедеры для O_RDONLY и тому подобных есть)
// - В реализациях функций можно использовать только хедеры, указанные в начале этого файла и указанные непосредственно перед функцией
// - В std::runtime_error и THROW_MESSAGE (в коде либы и в user code) пишите сообщения с большой буквы для совместимости с тем, что выдаёт strerror

// Мелкие замечания
// - Не обрабатываем особо EINTR у close, т. к. я не знаю, у каких ещё функций так надо делать
// - Не используемые вещи отмечены nunu (not used not used)

//@ #pragma once
//@ #include <functional>
//@ #include <exception>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#include <stdexcept>
#include <string>

#include <boost/stacktrace.hpp>

#include "libsh-treis.hpp"

using namespace std::string_literals;

// Вводная часть

#define THROW_MESSAGE(m) \
  do \
    { \
      throw std::runtime_error (__func__ + ": "s + (m) + "\n" + boost::stacktrace::to_string (boost::stacktrace::stacktrace ())); \
    } \
  while (false)

//@ #include <locale.h>
#include <string.h>
namespace libsh_treis::libc //@
{ //@
char * //@
xstrerror_l (int errnum, locale_t locale)//@;
{
  char *result = strerror_l (errnum, locale);

  // Не вызываем здесь xstrerror_l, чтобы не было рекурсии
  if (result == nullptr)
    {
      THROW_MESSAGE ("Failed");
    }

  return result;
}
} //@

// В glibc locale_t - это указатель, поэтому, согласно моему code style, следовало бы написать (locale_t)nullptr, а не (locale_t)0. Тем не менее, мне не удалось найти в POSIX подтверждение тому, что locale_t - это указатель, поэтому (locale_t)0
#define THROW_ERRNO \
  do \
    { \
      int saved_errno = errno; \
      throw std::runtime_error (__func__ + ": "s + xstrerror_l (saved_errno, (locale_t)0) + "\n" + boost::stacktrace::to_string (boost::stacktrace::stacktrace ())); \
    } \
  while (false)

namespace libsh_treis //@
{ //@
bool //@
is_successful (const std::function<void(void)> &func) noexcept//@;
{
  // POSIX гарантирует, что stderr unbuffered или line buffered
  try
    {
      func ();
    }
  catch (const std::exception &ex)
    {
      if (ex.what ()[strlen (ex.what ()) - 1] == '\n')
        {
          fprintf (stderr, "%s", ex.what ());
        }
      else
        {
          fprintf (stderr, "%s\n", ex.what ());
        }

      return false;
    }
  catch (...)
    {
      fprintf (stderr, "Unknown error\n");
      return false;
    }

  return true;
}

int //@
main_helper (const std::function<void(void)> &func) noexcept//@;
{
  if (is_successful (func))
    {
      return EXIT_SUCCESS;
    }
  else
    {
      return EXIT_FAILURE;
    }
}
} //@

// Простые обёртки

//@ #include <sys/types.h> // size_t, ssize_t
#include <unistd.h>
namespace libsh_treis::libc //@
{ //@
ssize_t //@
xwrite_nunu (int fildes, const void *buf, size_t nbyte)//@;
{
  ssize_t result = write (fildes, buf, nbyte);

  if (result == -1)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

//@ #include <sys/types.h> // size_t, ssize_t
#include <unistd.h>
namespace libsh_treis::libc //@
{ //@
ssize_t //@
xread (int fildes, void *buf, size_t nbyte)//@;
{
  ssize_t result = read (fildes, buf, nbyte);

  if (result == -1)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

// fgetc и getc работают одинаково, поэтому реализуем только xfgetc
// Сбрасывает err flag перед вызовом getc
//@ #include <stdio.h>
namespace libsh_treis::libc //@
{ //@
int //@
xfgetc_nunu (FILE *stream)//@;
{
  clearerr (stream);

  int result = getc (stream);

  if (result == EOF && ferror (stream))
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

// Сбрасывает err flag перед вызовом getchar
#include <stdio.h>
namespace libsh_treis::libc //@
{ //@
int //@
xgetchar_nunu (void)//@;
{
  clearerr (stdin);

  int result = getchar ();

  if (result == EOF && ferror (stdin))
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

// Сбрасывает err flag перед вызовом getdelim
//@ #include <sys/types.h> // size_t, ssize_t
//@ #include <stdio.h>
namespace libsh_treis::libc //@
{ //@
ssize_t //@
xgetdelim (char **lineptr, size_t *n, int delimiter, FILE *stream)//@;
{
  clearerr (stdin);

  ssize_t result = getdelim (lineptr, n, delimiter, stream);

  if (result == -1 && ferror (stream))
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

// Сбрасывает err flag перед вызовом функции из libc
//@ #include <sys/types.h> // size_t, ssize_t
//@ #include <stdio.h>
namespace libsh_treis::libc //@
{ //@
ssize_t //@
xgetline (char **lineptr, size_t *n, FILE *stream)//@;
{
  return xgetdelim (lineptr, n, '\n', stream);
}
} //@

// Инклудит хедер для O_RDONLY и тому подобных
//@ #include <fcntl.h>
namespace libsh_treis::libc::no_raii //@
{ //@
int //@
xopen2 (const char *path, int oflag)//@;
{
  int result = open (path, oflag);

  if (result == -1)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

// Инклудит хедер для O_RDONLY и тому подобных
//@ #include <sys/types.h>
//@ #include <fcntl.h>
namespace libsh_treis::libc::no_raii //@
{ //@
int //@
xopen3_nunu (const char *path, int oflag, mode_t mode)//@;
{
  int result = open (path, oflag, mode);

  if (result == -1)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

#include <unistd.h>
namespace libsh_treis::libc //@
{ //@
void //@
xclose (int fildes)//@;
{
  if (close (fildes) == -1)
    {
      THROW_ERRNO;
    }
}
} //@

//@ #include <stdarg.h>
#include <stdio.h>
namespace libsh_treis::libc //@
{ //@
int //@
xvprintf (const char *format, va_list ap)//@;
{
  int result = vprintf (format, ap);

  if (result < 0)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

#include <stdarg.h>
namespace libsh_treis::libc //@
{ //@
int //@
xprintf (const char *format, ...)//@;
{
  va_list ap;
  va_start (ap, format);
  int result = xvprintf (format, ap);
  va_end (ap);
  return result;
}
} //@

// xx-обёртки

// Сбрасывает err flag перед вызовом getc
//@ #include <stdio.h>
namespace libsh_treis::libc //@
{ //@
char //@
xxfgetc_nunu (FILE *stream)//@;
{
  int result = xfgetc_nunu (stream);

  if (result == EOF)
    {
      THROW_MESSAGE ("EOF");
    }

  return (char)(unsigned char)result;
}
} //@

// Сбрасывает err flag перед вызовом getchar
#include <stdio.h>
namespace libsh_treis::libc //@
{ //@
char //@
xxgetchar_nunu (void)//@;
{
  int result = xgetchar_nunu ();

  if (result == EOF)
    {
      THROW_MESSAGE ("EOF");
    }

  return (char)(unsigned char)result;
}
} //@

// Прочие функции

// Такая функция пригодится (т. е. именно с таким API), если, скажем, нужно прочитать первые 1000 байт файла, чтобы узнать, текстовый ли этот файл
// Необходимость повторять вызовы read может быть при чтении с терминала
// Если nbyte равно нулю, read не вызывается ни разу
//@ #include <sys/types.h> // size_t, ssize_t
namespace libsh_treis::libc //@
{ //@
ssize_t //@
read_repeatedly (int fildes, void *buf, size_t nbyte)//@;
{
  ssize_t have_read = 0;

  while (have_read < (ssize_t)nbyte)
    {
      ssize_t result_of_xread = xread (fildes, (char *)buf + have_read, nbyte - have_read);

      if (result_of_xread == 0)
        {
          break;
        }

      have_read += result_of_xread;
    }

  return have_read;
}
} //@

// Такая функция пригодится, если мы читаем блоки фиксированного размера из файла один за другим
//@ #include <sys/types.h> // size_t, ssize_t
namespace libsh_treis::libc //@
{ //@
bool //@
xread_repeatedly_nunu (int fildes, void *buf, size_t nbyte)//@;
{
  ssize_t have_read = read_repeatedly (fildes, buf, nbyte);

  if (have_read == (ssize_t)nbyte)
    {
      return true;
    }

  if (have_read == 0)
    {
      return false;
    }

  THROW_MESSAGE ("Partial data");
}
} //@

//@ #include <sys/types.h> // size_t, ssize_t
namespace libsh_treis::libc //@
{ //@
void //@
xxread_repeatedly_nunu (int fildes, void *buf, size_t nbyte)//@;
{
  if (!xread_repeatedly_nunu (fildes, buf, nbyte))
    {
      THROW_MESSAGE ("EOF");
    }
}
} //@

//@ #include <sys/types.h>
//@ #include <unistd.h>
//@ namespace libsh_treis::libc
//@ {
//@ struct open2_tag
//@ {
//@ };

//@ struct open3_tag_nunu
//@ {
//@ };

//@ class fd
//@ {
//@   int _fd;
//@   int _exceptions;

//@ public:

//@   fd (open2_tag, const char *path, int oflag)
//@   {
//@     _fd = ::libsh_treis::libc::no_raii::xopen2 (path, oflag);
//@     _exceptions = std::uncaught_exceptions ();
//@   }

//@   fd (open3_tag_nunu, const char *path, int oflag, mode_t mode)
//@   {
//@     _fd = ::libsh_treis::libc::no_raii::xopen3_nunu (path, oflag, mode);
//@     _exceptions = std::uncaught_exceptions ();
//@   }

//@   fd (const fd &) = delete;
//@   fd (fd &&) = delete;
//@   fd &operator= (const fd &) = delete;
//@   fd &operator= (fd &&) = delete;

//@   ~fd (void) noexcept (false)
//@   {
//@     if (std::uncaught_exceptions () == _exceptions)
//@       {
//@         xclose (_fd);
//@       }
//@     else
//@       {
//@         close (_fd);
//@       }
//@   }

//@   int
//@   get (void) const noexcept
//@   {
//@     return _fd;
//@   }
//@ };
//@ }

//[todo] del dyo
//[todo] написать пример про strong exc safety после появления fd

//[todo v2]откл boost, потом вкл


//[todo v2]to implement: read, write, getchar, [wish]atoi
//[todo v5] просто реализовать некую базу, в том числе fwrite