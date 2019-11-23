// libsh-treis - собрание C++-библиотек (проект из C++-библиотек) с обёртками над C-библиотеками
// Одновременно libsh-treis - это главная библиотека в проекте, которая оборачивает libc в Linux (пространство имён libsh_treis::libc)
// Весь проект предназначен только для Linux
// Цель проекта: бросать исключения в случае ошибок и использовать RAII
// 2018DECTMP

// Сборка этой либы
// - Написан на C++20, используется только одна фича из C++20: designated initializers, проекты на этой либе должны использовать как минимум C++17
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
// - В пространстве libsh_treis::libc::no_raii бросаются исключения в случае ошибок, но есть нарушения RAII
// - В пространстве libsh_treis::libc (без no_raii) бросаются исключения в случае ошибок и RAII не нарушается
// - Сигнатура оборачиваемой функции остаётся без изменений, за исключением тривиальных, т. е. если исходная функция при успешном исходе всегда возвращает одно и то же (либо результат можно вывести из аргументов), то нужно возвращать void либо сделать один из out-параметров выходным
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
// - В случае ошибок бросаются исключения, и ничего не печатается на экран. Т. к. может быть нужно вызвать какую-нибудь функцию, чтобы проверить, может она выполнить своё действие или нет. И если нет, то сделать что-нибудь другое
// - Деструкторы могут бросать исключения
// - Обёртки вокруг библиотечных функций, являющихся strong exception safe, сами являются strong exception safe. Тем не менее, использование этой либы не гарантирует то, что ваш код будет strong exception safe. Например, деструктор libsh_treis::libc::fd закрывает файл. Но если xopen3 создал его, то удалён он не будет! Другой пример: открываем файл для записи, пишем данные, потом пишем ещё данные и закрываем. Если при записи второго блока данных произойдёт ошибка, то первая запись не откатится
// - Печать backtrace'а временно отключена, чтобы выяснить, нужна ли она. Когда понадобится - включить. А также убирать '\n' при генерации исключения, а не при ловле
// - RAII-обёртки вокруг файловых дескрипторов и тому подобного не должны иметь особого состояния. Если разрешить особое состояние, то выловить попытку использования обёртки в особом состоянии можно будет только с помощью статических анализаторов или в runtime'е, что меня не устраивает. Поэтому особого состояния у обёрток не будет. Если нужно перемещать обёртки, возвращать из функций или деструктуировать их до конца scope'а, нужно использовать std::unique_ptr. Функция, создающая пайп, будет возвращать два unique_ptr'а

// Необёрнутые функции и функции, которые не надо использовать
// - getc не обёрнуто, т. к. работает так же, как и fgetc
// - std::stoi, std::stol, std::stoll, atoi, atol, atoll, strtol, strtoll, sscanf игнорируют whitespace перед числом. Поэтому всех их не рекомендуется использовать (в случае sscanf - не рекомендуется использовать для парсинга целых чисел). Вместо этого предлагается использовать libsh_treis::libc::stoi
// - sprintf не обёрнуто, его не надо использовать
// - У нас не будет stdarg-вариантов exec*, т. к. внутри их всё равно придётся реализовать через контейнер. Поэтому будет сразу вариант exec*, который принимает begin и end

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
#include "gnu-source.hpp"

using namespace std::string_literals;

// Вводная часть

#define THROW_MESSAGE(m) \
  do \
    { \
      throw std::runtime_error (__func__ + ": "s + (m) /*+ "\n" + boost::stacktrace::to_string (boost::stacktrace::stacktrace ())*/); \
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
      throw std::runtime_error (__func__ + ": "s + xstrerror_l (saved_errno, (locale_t)0) /*+ "\n" + boost::stacktrace::to_string (boost::stacktrace::stacktrace ())*/); \
    } \
  while (false)

namespace libsh_treis //@
{ //@

//@ struct fail_silently
//@ {
//@ };

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
  catch (const fail_silently &)
    {
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
xwrite (int fildes, const void *buf, size_t nbyte)//@;
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
xvdprintf (int fildes, const char *format, va_list ap)//@;
{
  int result = vdprintf (fildes, format, ap);

  if (result < 0)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

//@ #include <stdarg.h>
//@ #include <stdio.h>
namespace libsh_treis::libc //@
{ //@
int //@
xvfprintf (FILE *stream, const char *format, va_list ap)//@;
{
  int result = vfprintf (stream, format, ap);

  if (result < 0)
    {
      THROW_ERRNO;
    }

  return result;
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

//@ #include <sys/types.h> // size_t, ssize_t
//@ #include <stdarg.h>
#include <stdio.h>
namespace libsh_treis::libc //@
{ //@
int //@
xvsnprintf_nunu (char *s, size_t n, const char *format, va_list ap)//@;
{
  int result = vsnprintf (s, n, format, ap);

  if (result < 0)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

//@ #include <stdarg.h>
namespace libsh_treis::libc::no_raii //@
{ //@
int //@
xvasprintf (char **strp, const char *fmt, va_list ap)//@;
{
  int result = libsh_treis::libc::detail::vasprintf_reexported (strp, fmt, ap);

  if (result < 0)
    {
      THROW_MESSAGE ("Failed");
    }

  return result;
}
} //@

#include <stdarg.h>
namespace libsh_treis::libc //@
{ //@
int //@
xdprintf (int fildes, const char *format, ...)//@;
{
  va_list ap;
  va_start (ap, format);
  int result = xvdprintf (fildes, format, ap);
  va_end (ap);
  return result;
}
} //@

//@ #include <stdio.h>
#include <stdarg.h>
namespace libsh_treis::libc //@
{ //@
int //@
xfprintf (FILE *stream, const char *format, ...)//@;
{
  va_list ap;
  va_start (ap, format);
  int result = xvfprintf (stream, format, ap);
  va_end (ap);
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

//@ #include <sys/types.h> // size_t, ssize_t
#include <stdarg.h>
namespace libsh_treis::libc //@
{ //@
int //@
xsnprintf (char *s, size_t n, const char *format, ...)//@;
{
  va_list ap;
  va_start (ap, format);
  int result = xvsnprintf_nunu (s, n, format, ap);
  va_end (ap);
  return result;
}
} //@

#include <stdarg.h>
namespace libsh_treis::libc::no_raii //@
{ //@
int //@
xasprintf_nunu (char **strp, const char *fmt, ...)//@;
{
  va_list ap;
  va_start (ap, fmt);
  int result = xvasprintf (strp, fmt, ap);
  va_end (ap);
  return result;
}
} //@

#include <unistd.h>
namespace libsh_treis::libc //@
{ //@
void //@
xchdir (const char *path)//@;
{
  if (chdir (path) == -1)
    {
      THROW_ERRNO;
    }
}
} //@

// POSIX 2018 сообщает, что popen "may set errno" в случае ошибки. Предполагаем, что popen всегда ставит errno в случае ошибки (так написано в линуксовом мане)
// #include <stdio.h>
namespace libsh_treis::libc::no_raii //@
{ //@
FILE * //@
xpopen (const char *command, const char *mode)//@;
{
  FILE *result = popen (command, mode);

  if (result == nullptr)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

//@ #include <stdio.h>
namespace libsh_treis::libc //@
{ //@
int //@
xpclose (FILE *stream)//@;
{
  int result = pclose (stream);

  if (result == -1)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

//@ #include <stdio.h>
namespace libsh_treis::libc //@
{ //@
int //@
xfileno (FILE *stream)//@;
{
  int result = fileno (stream);

  if (result == -1)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

#include <unistd.h>
namespace libsh_treis::libc::no_raii //@
{ //@
//@ struct pipe_result
//@ {
//@   int readable;
//@   int writable;
//@ };

pipe_result //@
xpipe (void)//@;
{
  int result[2];

  if (pipe (result) == -1)
    {
      THROW_ERRNO;
    }

  return { .readable = result[0], .writable = result[1] };
}
} //@

//@ #include <sys/types.h>
#include <unistd.h>
namespace libsh_treis::libc::no_raii //@
{ //@
pid_t //@
xfork (void)//@;
{
  pid_t result = fork ();

  if (result == (pid_t)-1)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

//@ #include <sys/types.h>
#include <sys/wait.h>
namespace libsh_treis::libc //@
{ //@
pid_t //@
xwaitpid (pid_t pid, int *stat_loc, int options)//@;
{
  pid_t result = waitpid (pid, stat_loc, options);

  if (result == (pid_t)-1)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

// Функция обычно используется, чтобы скопировать fd на 0, 1 или 2. Эти fd не имеет смысла оборачивать в RAII-обёртки. Поэтому не-RAII версию xdup2 помещаем в namespace libsh_treis::libc
#include <unistd.h>
namespace libsh_treis::libc //@
{ //@
void //@
xdup2 (int fildes, int fildes2)//@;
{
  if (dup2 (fildes, fildes2) == -1)
    {
      THROW_ERRNO;
    }
}
} //@

// За одно фиксим проблему с прототипом функций exec*
#include <unistd.h>
namespace libsh_treis::libc //@
{ //@
[[noreturn]] void //@
xexecv_nunu (const char *path, const char *const argv[])//@;
{
  execv (path, (char *const *)argv);

  THROW_ERRNO;
}
} //@

#include <unistd.h>
namespace libsh_treis::libc //@
{ //@
[[noreturn]] void //@
xexecvp (const char *file, const char *const argv[])//@;
{
  execvp (file, (char *const *)argv);

  THROW_ERRNO;
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
xread_repeatedly (int fildes, void *buf, size_t nbyte)//@;
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
xxread_repeatedly (int fildes, void *buf, size_t nbyte)//@;
{
  if (!xread_repeatedly (fildes, buf, nbyte))
    {
      THROW_MESSAGE ("EOF");
    }
}
} //@

// Если nbyte равно нулю, write не вызывается ни разу
//@ #include <sys/types.h> // size_t, ssize_t
namespace libsh_treis::libc //@
{ //@
void //@
write_repeatedly (int fildes, const void *buf, size_t nbyte)//@;
{
  ssize_t written = 0;

  while (written != (ssize_t)nbyte)
    {
      written += xwrite (fildes, (const char *)buf + written, nbyte - written);
    }
}
} //@

//@ #include <sys/types.h>
//@ #include <unistd.h>
//@ namespace libsh_treis::libc
//@ {
//@ struct xopen2_tag
//@ {
//@ };
//@ struct xopen3_tag_nunu
//@ {
//@ };
//@ struct pipe_result;
//@ class fd
//@ {
//@   int _fd;
//@   int _exceptions;

//@   explicit fd (int f) noexcept : _fd (f), _exceptions (std::uncaught_exceptions ())
//@   {
//@   }

//@   friend pipe_result xpipe (void);

//@ public:

//@   fd (xopen2_tag, const char *path, int oflag) : _fd (libsh_treis::libc::no_raii::xopen2 (path, oflag)), _exceptions (std::uncaught_exceptions ())
//@   {
//@   }

//@   fd (xopen3_tag_nunu, const char *path, int oflag, mode_t mode) : _fd (libsh_treis::libc::no_raii::xopen3_nunu (path, oflag, mode)), _exceptions (std::uncaught_exceptions ())
//@   {
//@   }

//@   fd (fd &&) = delete;
//@   fd (const fd &) = delete;
//@   fd &operator= (fd &&) = delete;
//@   fd &operator= (const fd &) = delete;

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

// Мне не нравятся функции для парсинга целых чисел в стандартах C и C++, поэтому я пишу свою. А раз уж пишу свою, то в качестве back end'а буду использовать from_chars как самую низкоуровневую и быструю
//@ #include <string_view>
#include <charconv>
#include <system_error>
namespace libsh_treis::libc //@
{ //@
int //@
stoi (const std::string_view &s)//@;
{
  if (s.size () == 0)
    {
      THROW_MESSAGE ("Empty string");
    }

  if (!(s[0] == '-' || ('0' <= s[0] && s[0] <= '9')))
    {
      THROW_MESSAGE ("Doesn't begin with [-0-9]");
    }

  int result;

  auto [ptr, ec] = std::from_chars (s.cbegin (), s.cend (), result);

  if (ec != std::errc ())
    {
      THROW_MESSAGE ("Not a valid number (std::from_chars returned error)");
    }

  if (ptr != s.cend ())
    {
      THROW_MESSAGE ("There is a garbagge after number");
    }

  return result;
}
} //@

// Не возвращаем результат [v]asprintf'а, т. к. его можно получить за O(1), вызвав size () у строки. Работает, даже если есть нулевые байты
//@ #include <stdarg.h>
//@ #include <string>
#include <stdlib.h>
namespace libsh_treis::libc //@
{ //@
std::string //@
xvasprintf (const char *fmt, va_list ap)//@;
{
  char *str;

  int length = libsh_treis::libc::no_raii::xvasprintf (&str, fmt, ap);

  std::string result (str, length);

  free (str);

  return result;
}
} //@

//@ #include <string>
#include <stdarg.h>
namespace libsh_treis::libc //@
{ //@
std::string //@
xasprintf (const char *fmt, ...)//@;
{
  va_list ap;
  va_start (ap, fmt);
  std::string result = xvasprintf (fmt, ap);
  va_end (ap);
  return result;
}
} //@

#include <stdlib.h>
#include <sys/wait.h>
namespace libsh_treis::libc //@
{ //@
void //@
process_succeed (int status)//@;
{
  if (!WIFEXITED (status))
    {
      THROW_MESSAGE ("Process status is not \"exited\"");
    }

  if (WEXITSTATUS (status) != EXIT_SUCCESS)
    {
      THROW_MESSAGE ("Process exit code is not 0");
    }
}
} //@

// Деструктор всегда делает pclose
//@ #include <stdio.h>
//@ namespace libsh_treis::libc
//@ {
//@ class pipe_stream
//@ {
//@   FILE *_stream;
//@   int _exceptions;

//@ public:

//@   pipe_stream (const char *command, const char *mode) : _stream (libsh_treis::libc::no_raii::xpopen (command, mode)), _exceptions (std::uncaught_exceptions ())
//@   {
//@   }

//@   pipe_stream (pipe_stream &&) = delete;
//@   pipe_stream (const pipe_stream &) = delete;
//@   pipe_stream &operator= (pipe_stream &&) = delete;
//@   pipe_stream &operator= (const pipe_stream &) = delete;

//@   ~pipe_stream (void) noexcept (false)
//@   {
//@     if (std::uncaught_exceptions () == _exceptions)
//@       {
//@         process_succeed (xpclose (_stream));
//@       }
//@     else
//@       {
//@         pclose (_stream);
//@       }
//@   }

//@   FILE *
//@   get (void) const noexcept
//@   {
//@     return _stream;
//@   }
//@ };
//@ }

// Эта функция не является exception-safe
//@ #include <memory>
namespace libsh_treis::libc //@
{ //@
//@ struct pipe_result
//@ {
//@   std::unique_ptr<fd> readable;
//@   std::unique_ptr<fd> writable;
//@ };
pipe_result //@
xpipe (void)//@;
{
  auto result = libsh_treis::libc::no_raii::xpipe ();

  return { .readable = std::unique_ptr<fd> (new fd (result.readable)), .writable = std::unique_ptr<fd> (new fd (result.writable)) };
}
} //@

//@ #include <sys/types.h>
#include <stdlib.h>
// Вызывающая сторона должна сама flush'нуть C stdio и C++ streams перед вызовом этой функции. В том числе flush'нуть C stderr, т. к. он используется моей либой (если туда был вывод без '\n' в конце)
namespace libsh_treis::libc::no_raii //@
{ //@
pid_t //@
safe_fork (const std::function<void(void)> &func)//@;
{
  pid_t result = libsh_treis::libc::no_raii::xfork ();

  if (result == 0)
    {
      _Exit (libsh_treis::main_helper (func));
    }

  return result;
}
} //@

//@ #include <sys/types.h>
namespace libsh_treis::libc //@
{ //@
int //@
xwaitpid_status (pid_t pid, int options)//@;
{
  int result;

  xwaitpid (pid, &result, options);

  return result;
}
} //@

// Деструктор всегда делает waitpid
//@ #include <sys/types.h>
//@ #include <sys/wait.h>
//@ namespace libsh_treis::libc
//@ {
//@ class process
//@ {
//@   pid_t _pid;
//@   int _exceptions;

//@ public:

//@   explicit process (const std::function<void(void)> &func) : _pid (libsh_treis::libc::no_raii::safe_fork (func)), _exceptions (std::uncaught_exceptions ())
//@   {
//@   }

//@   process (process &&) = delete;
//@   process (const process &) = delete;
//@   process &operator= (process &&) = delete;
//@   process &operator= (const process &) = delete;

//@   ~process (void) noexcept (false)
//@   {
//@     if (std::uncaught_exceptions () == _exceptions)
//@       {
//@         process_succeed (xwaitpid_status (_pid, 0));
//@       }
//@     else
//@       {
//@         waitpid (_pid, nullptr, 0);
//@       }
//@   }

//@   pid_t
//@   get (void) const noexcept
//@   {
//@     return _pid;
//@   }
//@ };
//@ }

//@ #include <memory>
namespace libsh_treis::libc //@
{ //@
int //@
xwaitpid_raii (std::unique_ptr<process> proc, int options)//@;
{
  process *ptr = proc.release ();

  int result = xwaitpid_status (ptr->get (), options);

  operator delete (ptr);

  return result;
}
} //@

// xexecv_string и xexecvp_string - для range'а объектов, у которых есть c_str ()

//@ #include <vector>
//@ namespace libsh_treis::libc
//@ {
//@ template <typename Iter> [[noreturn]] void
//@ xexecv_string_nunu (const char *path, Iter b, Iter e)
//@ {
//@   std::vector<const char *> v;
//@   for (; b != e; ++b)
//@     {
//@       v.push_back (b->c_str ());
//@     }
//@   v.push_back (nullptr);
//@   xexecv_nunu (path, v.data ());
//@ }
//@ }

//@ #include <vector>
//@ namespace libsh_treis::libc
//@ {
//@ template <typename Iter> [[noreturn]] void
//@ xexecvp_string (const char *file, Iter b, Iter e)
//@ {
//@   std::vector<const char *> v;
//@   for (; b != e; ++b)
//@     {
//@       v.push_back (b->c_str ());
//@     }
//@   v.push_back (nullptr);
//@   xexecvp (file, v.data ());
//@ }
//@ }
