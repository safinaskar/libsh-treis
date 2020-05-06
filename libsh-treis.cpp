// libsh-treis - собрание C++-библиотек (проект из C++-библиотек) с обёртками над C-библиотеками
// Одновременно libsh-treis - это главная библиотека в проекте, которая оборачивает libc в Linux (пространство имён libsh_treis::libc)
// Весь проект предназначен только для Linux
// Цель проекта: бросать исключения в случае ошибок и использовать RAII
// 2018DECTMP

// Сборка этой либы
// - Написан на C++20, проекты на этой либе должны использовать как минимум C++20
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
// - Но иногда сменить тип результата всё-таки можно, именно так сделано у xx_fgetc
// - Убираем <stdarg.h> там, где это возможно. См., например, x_open_2 и x_open_3
// - Мы понимаем ошибку в максимально прямом и естественном смысле, без фантазирования. Мы не называем ошибкой то, что ей не является. Например, если getchar вернул EOF из-за конца файла, то это не ошибка (это следует из здравого смысла). В то же время в некоторых случаях мы упрощаем себе задачу. Например, большинство системных вызовов Linux (а точнее, их обёрток из libc) возвращают -1 в случае ошибки. Такие ошибки мы и вправду считаем ошибками, чтобы упростить себе задачу, в том числе считаем ошибками всякие там EAGAIN, EINTR, EINPROGRESS, EWOULDBLOCK и тому подобные
// - Если кроме естественных ошибок хочется бросать исключения при дополнительных, заводим xx-функцию. Например, xx_getchar бросает исключение при EOF
// - Семантика функций из C headers и соответствующих C++ headers может различаться очень существенно. Например, std::exit из <cstdlib> гарантирует вызов деструкторов для глобальных объектов, а exit из <stdlib.h> - нет. Тем не менее, реализация в Linux хорошая, т. е. std::exit из <cstdlib> просто ссылается на exit из <stdlib.h>. То же для функций из <string.h> и <cstring>. Поэтому всегда используем .h-файлы. Так проще, не надо различать хедеры из стандарта C++ и Linux-специфичные
// - .hpp инклудит только то, что нужно, чтобы сам .hpp работал. .cpp инклудит то, что нужно для реализации. Потом, когда-нибудь, наверное, нужно будет ещё и инклудить то, что нужно для правильного использования, скажем, чтобы было O_CREAT к open (но конкретно в случае open хедеры для O_RDONLY и тому подобных есть)
// - В реализациях функций можно использовать только хедеры, указанные в начале этого файла и указанные непосредственно перед функцией
// - В std::runtime_error и _LIBSH_TREIS_THROW_MESSAGE (в коде либы и в user code) пишите сообщения с большой буквы для совместимости с тем, что выдаёт strerror

// Мелкие замечания
// - Не обрабатываем особо EINTR у close, т. к. я не знаю, у каких ещё функций так надо делать
// - Не используемые вещи отмечены nunu (not used not used). Если я использовал интерфейс где-то, а потом перестал, nunu не ставится
// - В случае ошибок бросаются исключения, и ничего не печатается на экран. Т. к. может быть нужно вызвать какую-нибудь функцию, чтобы проверить, может она выполнить своё действие или нет. И если нет, то сделать что-нибудь другое
// - Деструкторы могут бросать исключения
// - Обёртки вокруг библиотечных функций, являющихся strong exception safe, сами являются strong exception safe. Тем не менее, использование этой либы не гарантирует то, что ваш код будет strong exception safe. Например, деструктор libsh_treis::libc::fd закрывает файл. Но если x_open_3 создал его, то удалён он не будет! Другой пример: открываем файл для записи, пишем данные, потом пишем ещё данные и закрываем. Если при записи второго блока данных произойдёт ошибка, то первая запись не откатится
// - RAII-обёртки вокруг файловых дескрипторов и тому подобного не должны иметь особого состояния. Если разрешить особое состояние, то выловить попытку использования обёртки в особом состоянии можно будет только с помощью статических анализаторов или в runtime'е, что меня не устраивает. Поэтому особого состояния у обёрток не будет. Если нужно перемещать обёртки, возвращать из функций или деструктуировать их до конца scope'а, нужно использовать std::unique_ptr. Функция, создающая пайп, будет возвращать два unique_ptr'а
// - Либа работает только с исключениями, которые сообщают об ошибках. Нет поддержки исключений, которые сообщают о том, как нужно завершить программу. Разрешить таким исключениям появляться где угодно в программе - это неправильно. В частности, нет поддержки исключения, которое говорит, что нужно завершить программу, вернув EXIT_FAILURE, но ничего не выводя на экран
// - Выбрал названия в стиле "x_write", а не "xwrite", потому что иначе обёртки для xcb выглядели бы так: xxcb_ewmh_send_client_message или так: xewmh_send_client_message, а это некрасиво
// - Даже no_raii-функции exception-safe, например, libsh_treis::xcb::no_raii::x_connect делает xcb_disconnect в случае ошибки
// - Вещи, не имеющие особого отношения к сути libsh_treis, но которые могут пригодиться в моём коде, вынесены в libsh_treis::tools. По сути это аналог Google Abseil, который я не стал выносить в отдельную либу, т. к. я всё равно никому не показываю свой код
// - Все функции C, которые фактически принимают span, должны быть обёрнуты с использованием span. Например, read, write, readv, writev, poll, memcpy
// - Нежелательно использовать неуникальные имена методов, такие как get, data и так далее. Например, если бы libsh_treis::libc::fd имел метод get, и у нас был бы unique_ptr<fd> x, то было бы легко перепутать x.get и x->get

// Необёрнутые функции и функции, которые не надо использовать
// - getc не обёрнуто, т. к. работает так же, как и fgetc
// - std::stoi, std::stol, std::stoll, atoi, atol, atoll, strtol, strtoll, sscanf игнорируют whitespace перед числом. Поэтому всех их не рекомендуется использовать (в случае sscanf - не рекомендуется использовать для парсинга целых чисел). Вместо этого предлагается использовать libsh_treis::libc::sto
// - sprintf не обёрнуто, его не надо использовать
// - У нас не будет stdarg-вариантов exec*, т. к. внутри их всё равно придётся реализовать через контейнер. Поэтому будет сразу вариант exec*, который принимает begin и end

// Fast stdio
// - Я попытался написать быструю замену fgetc и fputc. Она должна была обеспечивать производительность ручного решения (с двойным циклом и вызовами read и write) и удобство API fgetc и fputc. Но я упёрся в баг https://bugs.llvm.org/show_bug.cgi?id=44380 . Остаток этого параграфа будет посвящён полезной информации, которую я узнал в процессе поисков (в 2019-м году). Предположения были таковы:
// -- Мы читаем данные не для того, чтобы без изменений записать их обратно. Скорее всего, такой case нужно разбирать отдельно. Поэтому я тестировал отдельно чтение и отдельно запись. Для тестирования чтения я реализовывал "wc -l", для тестирования записи генерировал все символы от 0 по 255 много раз
// -- Нужно решение, которое будет работать с fd любых типов. Поэтому mmap не подходит
// - Обычный stdio работает медленно, даже с учётом unlocked и использования getc/putc вместо fgetc/fputc
// - На 64-разрядной машине часто int32_t всё же быстрее int64_t. Например, для счётчика строк в wc -l лучше использовать int32_t
// - Если результат read положить в int32_t на 64-разрядной машине, то компилятор не может вывести, что этот результат >= -1. Поэтому нужно использовать int64_t либо "подсказать" компилятору правильный интервал для значения. См. https://bugs.llvm.org/show_bug.cgi?id=44360
// - В wc -l при чтении из пайпа оптимальный размер буфера - это размер пайпа, даже если он изменён. Скорость не падает от использования runtime-значения для размера буфера
// - В wc -l при чтении из пайпа буфер должен иметь выравнивание 32 байта. Причина такого значения не известна
// - Не исследовано: vmsplice ("So you could do a very efficient "stdio-like" implementation for logging"), st_blksize, O_DIRECT, pipe (O_DIRECT), самому устанавливать размер пайпа с помощью F_SETPIPE_SZ, __builtin_assume_aligned, big/huge pages, io_uring, async, non-blocking, функции со словом advise в названии

//@ #pragma once
//@ #include <functional>
//@ #include <exception>
//@ #include <stdexcept>
//@ #include <string>

//@ #include <boost/stacktrace.hpp>

//@ #include "gnu-source.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#include "libsh-treis.hpp"

using namespace std::string_literals;

// Вводная часть

// Используется в функциях, которые реализованны в хедере
//@ #define _LIBSH_TREIS_THROW_MESSAGE(m) \
//@   do \
//@     { \
//@       auto str = boost::stacktrace::to_string (boost::stacktrace::stacktrace ()); \
//@       if (!str.empty ()) \
//@         { \
//@           str.pop_back (); \
//@         } \
//@       throw std::runtime_error (__PRETTY_FUNCTION__ + std::string (": ") + (m) + "\n" + str); \
//@     } \
//@   while (false)

//@ #include <locale.h>
#include <string.h>
namespace libsh_treis::libc //@
{ //@
char * //@
x_strerror_l (int errnum, locale_t locale)//@;
{
  char *result = strerror_l (errnum, locale);

  // Не вызываем здесь x_strerror_l, чтобы не было рекурсии
  if (result == nullptr)
    {
      _LIBSH_TREIS_THROW_MESSAGE ("Failed");
    }

  return result;
}
} //@

// В glibc locale_t - это указатель, поэтому, согласно моему code style, следовало бы написать (locale_t)nullptr, а не (locale_t)0. Тем не менее, мне не удалось найти в POSIX подтверждение тому, что locale_t - это указатель, поэтому (locale_t)0
#define THROW_ERRNO \
  do \
    { \
      int saved_errno = errno; \
      auto str = boost::stacktrace::to_string (boost::stacktrace::stacktrace ()); \
      if (!str.empty ()) \
        { \
          str.pop_back (); \
        } \
      throw std::runtime_error (__PRETTY_FUNCTION__ + ": "s + x_strerror_l (saved_errno, (locale_t)0) + "\n" + str); \
    } \
  while (false)

// Добавляем по мере необходимости в функции
#define THROW_ERRNO_MESSAGE(m) \
  do \
    { \
      int saved_errno = errno; \
      auto str = boost::stacktrace::to_string (boost::stacktrace::stacktrace ()); \
      if (!str.empty ()) \
        { \
          str.pop_back (); \
        } \
      throw std::runtime_error (__PRETTY_FUNCTION__ + ": "s + (m) + ": "s + x_strerror_l (saved_errno, (locale_t)0) + "\n" + str); \
    } \
  while (false)

namespace libsh_treis::tools //@
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
      // Имя программы обязательно, иначе нельзя понять, какая именно из программ в пайпе свалилась
      fprintf (stderr, "%s: %s\n", libsh_treis::detail::program_invocation_name_reexported (), ex.what ());
      return false;
    }
  catch (...)
    {
      fprintf (stderr, "%s: unknown exception\n", libsh_treis::detail::program_invocation_name_reexported ());
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

//@ namespace libsh_treis::tools
//@ {
//@ class not_movable
//@ {
//@ public:
//@   not_movable (void) = default;
//@   not_movable (not_movable &&) = delete;
//@   not_movable (const not_movable &) = delete;
//@   not_movable &
//@   operator= (not_movable &&) = delete;
//@   not_movable &
//@   operator= (const not_movable &) = delete;
//@ };
//@ }

// Работает всегда, даже в NDEBUG. Тем не менее, смысл тот же: если assertion не выполняется, значит, в программе баг. Т. е. этот макрос нельзя использовать для проверки того, что реально может произойти
//@ #include <stdio.h>
//@ #include <stdlib.h>
//@ #define LIBSH_TREIS_ASSERT(assertion) \
//@   do \
//@     { \
//@       if (assertion) \
//@         { \
//@           /* Пишем if именно в такой форме, чтобы сработал обычный contextual conversion to bool, а не operator! */ \
//@         } \
//@       else \
//@         { \
//@           fprintf (stderr, "%s", (libsh_treis::detail::program_invocation_name_reexported () + std::string (": ") + __PRETTY_FUNCTION__ + std::string (": assertion \"") + #assertion + std::string ("\" failed\n") + boost::stacktrace::to_string (boost::stacktrace::stacktrace ())).c_str ()); \
//@           exit (EXIT_FAILURE); \
//@         } \
//@     } \
//@   while (false)

//@ #include <span>
//@ #include <cstddef>
//@ namespace libsh_treis::tools
//@ {
//@ template <typename T> std::span<const std::byte, sizeof (T)>
//@ any_as_bytes (const T &x) noexcept
//@ {
//@   return std::as_bytes (std::span<const T, 1> (&x, 1));
//@ }

//@ template <typename T> std::span<std::byte, sizeof (T)>
//@ any_as_writable_bytes (T &x) noexcept
//@ {
//@   return std::as_writable_bytes (std::span<T, 1> (&x, 1));
//@ }
//@ }

// Простые обёртки

//@ #include <sys/types.h> // size_t, ssize_t
//@ #include <span>
//@ #include <cstddef>
#include <unistd.h>
namespace libsh_treis::libc //@
{ //@
ssize_t //@
x_write (int fildes, std::span<const std::byte> buf)//@;
{
  ssize_t result = write (fildes, buf.data (), buf.size ());

  if (result == -1)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

// Возвращаем ssize_t, а не span и не пару span'ов, т. к. я не могу придумать, где можно применить x_read, кроме как в его обёртке read_repeatedly. Поэтому нет нужды изощряться с возвращаемым типом (то же для x_write)
//@ #include <sys/types.h> // size_t, ssize_t
//@ #include <span>
//@ #include <cstddef>
#include <unistd.h>
namespace libsh_treis::libc //@
{ //@
ssize_t //@
x_read (int fildes, std::span<std::byte> buf)//@;
{
  ssize_t result = read (fildes, buf.data (), buf.size ());

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
x_fgetc_nunu (FILE *stream)//@;
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
x_getchar_nunu (void)//@;
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
x_getdelim (char **lineptr, size_t *n, int delimiter, FILE *stream)//@;
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
x_getline (char **lineptr, size_t *n, FILE *stream)//@;
{
  return x_getdelim (lineptr, n, '\n', stream);
}
} //@

// Инклудит хедер для O_RDONLY и тому подобных
// Называем именно x_open_2, а не x_open2, т. к. в ядре Linux существует системный вызов openat2
//@ #include <fcntl.h>
namespace libsh_treis::libc::no_raii //@
{ //@
int //@
x_open_2 (const char *path, int oflag)//@;
{
  LIBSH_TREIS_ASSERT (!((oflag & O_CREAT) == O_CREAT || (oflag & O_TMPFILE) == O_TMPFILE));

  int result = open (path, oflag);

  if (result == -1)
    {
      THROW_ERRNO_MESSAGE (path);
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
x_open_3 (const char *path, int oflag, mode_t mode)//@;
{
  int result = open (path, oflag, mode);

  if (result == -1)
    {
      THROW_ERRNO_MESSAGE (path);
    }

  return result;
}
} //@

#include <unistd.h>
namespace libsh_treis::libc //@
{ //@
void //@
x_close (int fildes)//@;
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
x_vdprintf (int fildes, const char *format, va_list ap)//@;
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
x_vfprintf (FILE *stream, const char *format, va_list ap)//@;
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
x_vprintf (const char *format, va_list ap)//@;
{
  int result = vprintf (format, ap);

  if (result < 0)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

// Результат vsnprintf может быть больше длины s, поэтому возвращать span нельзя
//@ #include <span>
//@ #include <stdarg.h>
#include <stdio.h>
namespace libsh_treis::libc //@
{ //@
int //@
x_vsnprintf (std::span<char> s, const char *format, va_list ap)//@;
{
  int result = vsnprintf (s.data (), s.size (), format, ap);

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
x_vasprintf (char **strp, const char *fmt, va_list ap)//@;
{
  int result = libsh_treis::libc::detail::vasprintf_reexported (strp, fmt, ap);

  if (result < 0)
    {
      _LIBSH_TREIS_THROW_MESSAGE ("Failed");
    }

  return result;
}
} //@

#include <stdarg.h>
namespace libsh_treis::libc //@
{ //@
int //@
x_dprintf (int fildes, const char *format, ...)//@;
{
  va_list ap;
  va_start (ap, format);
  int result = x_vdprintf (fildes, format, ap);
  va_end (ap);
  return result;
}
} //@

//@ #include <stdio.h>
#include <stdarg.h>
namespace libsh_treis::libc //@
{ //@
int //@
x_fprintf (FILE *stream, const char *format, ...)//@;
{
  va_list ap;
  va_start (ap, format);
  int result = x_vfprintf (stream, format, ap);
  va_end (ap);
  return result;
}
} //@

#include <stdarg.h>
namespace libsh_treis::libc //@
{ //@
int //@
x_printf (const char *format, ...)//@;
{
  va_list ap;
  va_start (ap, format);
  int result = x_vprintf (format, ap);
  va_end (ap);
  return result;
}
} //@

//@ #include <span>
#include <stdarg.h>
namespace libsh_treis::libc //@
{ //@
int //@
x_snprintf (std::span<char> s, const char *format, ...)//@;
{
  va_list ap;
  va_start (ap, format);
  int result = x_vsnprintf (s, format, ap);
  va_end (ap);
  return result;
}
} //@

#include <stdarg.h>
namespace libsh_treis::libc::no_raii //@
{ //@
int //@
x_asprintf_nunu (char **strp, const char *fmt, ...)//@;
{
  va_list ap;
  va_start (ap, fmt);
  int result = x_vasprintf (strp, fmt, ap);
  va_end (ap);
  return result;
}
} //@

#include <unistd.h>
namespace libsh_treis::libc //@
{ //@
void //@
x_chdir (const char *path)//@;
{
  if (chdir (path) == -1)
    {
      THROW_ERRNO_MESSAGE (path);
    }
}
} //@

// POSIX 2018 сообщает, что popen "may set errno" в случае ошибки. Предполагаем, что popen всегда ставит errno в случае ошибки (так написано в линуксовом мане)
// #include <stdio.h>
namespace libsh_treis::libc::no_raii //@
{ //@
FILE * //@
x_popen (const char *command, const char *mode)//@;
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
x_pclose (FILE *stream)//@;
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
x_fileno (FILE *stream)//@;
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
x_pipe (void)//@;
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
x_fork (void)//@;
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
x_waitpid (pid_t pid, int *stat_loc, int options)//@;
{
  pid_t result = waitpid (pid, stat_loc, options);

  if (result == (pid_t)-1)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

// Функция обычно используется, чтобы скопировать fd на 0, 1 или 2. Эти fd не имеет смысла оборачивать в RAII-обёртки. Поэтому не-RAII версию x_dup2 помещаем в namespace libsh_treis::libc
#include <unistd.h>
namespace libsh_treis::libc //@
{ //@
void //@
x_dup2 (int fildes, int fildes2)//@;
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
x_execv_nunu (const char *path, const char *const argv[])//@;
{
  execv (path, (char *const *)argv);

  THROW_ERRNO_MESSAGE (path);
}
} //@

#include <unistd.h>
namespace libsh_treis::libc //@
{ //@
[[noreturn]] void //@
x_execvp (const char *file, const char *const argv[])//@;
{
  execvp (file, (char *const *)argv);

  THROW_ERRNO_MESSAGE (file);
}
} //@

//@ #include <time.h>
namespace libsh_treis::libc //@
{ //@
timespec //@
x_clock_gettime (clockid_t clk_id)//@;
{
  timespec result;

  if (clock_gettime (clk_id, &result) == -1)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

//@ #include <pthread.h>
namespace libsh_treis::libc::no_raii //@
{ //@
pthread_t //@
x_pthread_create (const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)//@;
{
  pthread_t result;

  int error = pthread_create (&result, attr, start_routine, arg);

  if (error != 0)
    {
      errno = error;
      THROW_ERRNO;
    }

  return result;
}
} //@

#include <unistd.h>
namespace libsh_treis::libc //@
{ //@
void //@
x_syncfs (int fd)//@;
{
  if (syncfs (fd) == -1)
    {
      THROW_ERRNO;
    }
}
} //@

#include <unistd.h>
namespace libsh_treis::libc //@
{ //@
void //@
x_unlink (const char *pathname)//@;
{
  if (unlink (pathname) == -1)
    {
      THROW_ERRNO_MESSAGE (pathname);
    }
}
} //@

//@ #include <unistd.h>
namespace libsh_treis::libc //@
{ //@
off_t //@
x_lseek (int fd, off_t offset, int whence)//@;
{
  off_t result = lseek (fd, offset, whence);

  if (result == -1)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

#include <stdlib.h>
namespace libsh_treis::libc::no_raii //@
{ //@
int //@
x_mkstemp (char *templ)//@;
{
  int result = mkstemp (templ);

  if (result == -1)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

//@ #include <time.h>
namespace libsh_treis::libc //@
{ //@
void //@
x_clock_nanosleep (clockid_t clock_id, int flags, const timespec *request)//@;
{
  int err = clock_nanosleep (clock_id, flags, request, nullptr);

  if (err != 0)
    {
      errno = err;
      THROW_ERRNO;
    }
}
} //@

//@ #include <dirent.h>
namespace libsh_treis::libc::no_raii //@
{ //@
DIR * //@
x_opendir (const char *dirname)//@;
{
  DIR *result = opendir (dirname);

  if (result == nullptr)
    {
      THROW_ERRNO_MESSAGE (dirname);
    }

  return result;
}
} //@

//@ #include <dirent.h>
namespace libsh_treis::libc //@
{ //@
void //@
x_closedir (DIR *dirp)//@;
{
  if (closedir (dirp) == -1)
    {
      THROW_ERRNO;
    }
}
} //@

// Мы инклудим <dirent.h>, значит, можно использовать DT_REG и прочие
//@ #include <dirent.h>
namespace libsh_treis::libc //@
{ //@
dirent * //@
x_readdir (DIR *dirp)//@;
{
  int saved_errno = errno;

  errno = 0;

  dirent *result = readdir (dirp);

  if (result == nullptr && errno != 0)
    {
      THROW_ERRNO;
    }

  errno = saved_errno;

  return result;
}
} //@

//@ #include <dirent.h>
namespace libsh_treis::libc //@
{ //@
int //@
x_dirfd (DIR *dirp)//@;
{
  int result = dirfd (dirp);

  if (result == -1)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

//@ #include <sys/stat.h>
namespace libsh_treis::libc //@
{ //@
struct stat //@
x_stat (const char *path)//@;
{
  struct stat result;

  if (stat (path, &result) == -1)
    {
      THROW_ERRNO_MESSAGE (path);
    }

  return result;
}
} //@

// Стандартный fread эквивалентен read_repeatedly в том смысле, что он читает всё, пока не упрётся в ошибку или EOF. То есть некий fread_repeatedly уже не нужен
// Всегда передаём 1 в качестве второго аргумента (size) в fread. Чтобы можно было точно понять, сколько именно байт прочитано. Возможно, это будет иметь некие последствия для производительности. Но если вам важна производительность, вы просто не должны использовать fread вовсе, а должны использовать самописную буферизацию
// x_fread аналогичен read_repeatedly, поэтому тоже возвращает std::span<std::byte>
// Сбрасывает err flag
//@ #include <span>
//@ #include <cstddef>
//@ #include <stdio.h>
#include <stddef.h>
namespace libsh_treis::libc //@
{ //@
std::span<std::byte> //@
x_fread (std::span<std::byte> buf, FILE *stream)//@;
{
  clearerr (stream);

  size_t result = fread (buf.data (), 1, buf.size (), stream);

  if (ferror (stream))
    {
      THROW_ERRNO;
    }

  return std::span<std::byte> (buf.data (), result);
}
} //@

//@ #include <time.h>
namespace libsh_treis::libc //@
{ //@
tm //@
x_gmtime_r (time_t t)//@;
{
  tm result;

  if (gmtime_r (&t, &result) == nullptr)
    {
      THROW_ERRNO;
    }

  return result;
}
} //@

// Вы должны позаботиться, чтобы эта функция всегда писала непустую строку
//@ #include <stddef.h>
//@ #include <time.h>
//@ #include <span>
namespace libsh_treis::libc //@
{ //@
size_t //@
x_strftime (std::span<char> s, const char *format, const tm &tm)//@;
{
  size_t result = strftime (s.data (), s.size (), format, &tm);

  if (result == 0)
    {
      _LIBSH_TREIS_THROW_MESSAGE ("Buffer overflow");
    }

  return result;
}
} //@

//@ #include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
namespace libsh_treis::libc //@
{ //@
void //@
x_mkdir (const char *path, mode_t mode)//@;
{
  if (mkdir (path, mode) == -1)
    {
      THROW_ERRNO;
    }
}
} //@

// xx-обёртки

// Сбрасывает err flag перед вызовом getc
//@ #include <stdio.h>
namespace libsh_treis::libc //@
{ //@
char //@
xx_fgetc_nunu (FILE *stream)//@;
{
  int result = x_fgetc_nunu (stream);

  if (result == EOF)
    {
      _LIBSH_TREIS_THROW_MESSAGE ("EOF");
    }

  return (char)(unsigned char)result;
}
} //@

// Сбрасывает err flag перед вызовом getchar
#include <stdio.h>
namespace libsh_treis::libc //@
{ //@
char //@
xx_getchar_nunu (void)//@;
{
  int result = x_getchar_nunu ();

  if (result == EOF)
    {
      _LIBSH_TREIS_THROW_MESSAGE ("EOF");
    }

  return (char)(unsigned char)result;
}
} //@

//@ #include <span>
//@ #include <cstddef>
//@ #include <stdio.h>
namespace libsh_treis::libc //@
{ //@
bool //@
xx_fread (std::span<std::byte> buf, FILE *stream)//@;
{
  auto have_read = x_fread (buf, stream).size ();

  if (have_read == buf.size ())
    {
      return true;
    }

  if (have_read == 0)
    {
      return false;
    }

  _LIBSH_TREIS_THROW_MESSAGE ("Partial data");
}
} //@

//@ #include <span>
//@ #include <cstddef>
//@ #include <stdio.h>
namespace libsh_treis::libc //@
{ //@
void //@
xxx_fread (std::span<std::byte> buf, FILE *stream)//@;
{
  if (!xx_fread (buf, stream))
    {
      _LIBSH_TREIS_THROW_MESSAGE ("EOF");
    }
}
} //@

// Прочие функции

// Такая функция пригодится (т. е. именно с таким API), если, скажем, нужно прочитать первые 1000 байт файла, чтобы узнать, текстовый ли этот файл
// Необходимость повторять вызовы read может быть при чтении с терминала
// Если buf.size () равен нулю, read не вызывается ни разу
// Возвращаем std::span<std::byte>, т. к. именно это нужно в задаче чтения первых 1000 байт файла
//@ #include <span>
//@ #include <cstddef>
#include <sys/types.h>
namespace libsh_treis::libc //@
{ //@
std::span<std::byte> //@
read_repeatedly (int fildes, std::span<std::byte> buf)//@;
{
  auto to_fill = buf;

  while (to_fill.size () > 0)
    {
      ssize_t result_of_x_read = x_read (fildes, to_fill);

      if (result_of_x_read == 0)
        {
          break;
        }

      to_fill = to_fill.subspan (result_of_x_read);
    }

  return buf.first (buf.size () - to_fill.size ());
}
} //@

// Такая функция пригодится, если мы читаем блоки фиксированного размера из файла один за другим
//@ #include <span>
//@ #include <cstddef>
namespace libsh_treis::libc //@
{ //@
bool //@
x_read_repeatedly (int fildes, std::span<std::byte> buf)//@;
{
  auto have_read = read_repeatedly (fildes, buf).size ();

  if (have_read == buf.size ())
    {
      return true;
    }

  if (have_read == 0)
    {
      return false;
    }

  _LIBSH_TREIS_THROW_MESSAGE ("Partial data");
}
} //@

//@ #include <span>
//@ #include <cstddef>
namespace libsh_treis::libc //@
{ //@
void //@
xx_read_repeatedly (int fildes, std::span<std::byte> buf)//@;
{
  if (!x_read_repeatedly (fildes, buf))
    {
      _LIBSH_TREIS_THROW_MESSAGE ("EOF");
    }
}
} //@

// Если buf.size () равно нулю, write не вызывается ни разу
//@ #include <span>
//@ #include <cstddef>
namespace libsh_treis::libc //@
{ //@
void //@
write_repeatedly (int fildes, std::span<const std::byte> buf)//@;
{
  while (buf.size () != 0)
    {
      buf = buf.subspan (x_write (fildes, buf));
    }
}
} //@

//@ #include <sys/types.h>
//@ #include <unistd.h>
//@ namespace libsh_treis::libc
//@ {
//@ struct pipe_result;
//@ class fd: libsh_treis::tools::not_movable
//@ {
//@   int _fd;
//@   int _exceptions;

//@ public:

//@   explicit fd (int f) noexcept : _fd (f), _exceptions (std::uncaught_exceptions ())
//@   {
//@   }

//@   ~fd (void) noexcept (false)
//@   {
//@     if (std::uncaught_exceptions () == _exceptions)
//@       {
//@         x_close (_fd);
//@       }
//@     else
//@       {
//@         close (_fd);
//@       }
//@   }

//@   int
//@   resource (void) const noexcept
//@   {
//@     return _fd;
//@   }
//@ };
//@ }

namespace libsh_treis::libc //@
{ //@
fd //@
x_open_2 (const char *path, int oflag)//@;
{
  return fd (libsh_treis::libc::no_raii::x_open_2 (path, oflag));
}

fd //@
x_open_3 (const char *path, int oflag, mode_t mode)//@;
{
  return fd (libsh_treis::libc::no_raii::x_open_3 (path, oflag, mode));
}

fd //@
x_mkstemp (char *templ)//@;
{
  return fd (libsh_treis::libc::no_raii::x_mkstemp (templ));
}
} //@

// Мне не нравятся функции для парсинга целых чисел в стандартах C и C++, поэтому я пишу свою. А раз уж пишу свою, то в качестве back end'а буду использовать from_chars как самую низкоуровневую и быструю
//@ #include <string_view>
//@ #include <charconv>
//@ #include <system_error>
//@ namespace libsh_treis::libc
//@ {
//@ template <typename T> T
//@ sto (std::string_view s)
//@ {
//@   if (s.size () == 0)
//@     {
//@       _LIBSH_TREIS_THROW_MESSAGE ("Empty string");
//@     }

//@   if (!(s[0] == '-' || ('0' <= s[0] && s[0] <= '9')))
//@     {
//@       _LIBSH_TREIS_THROW_MESSAGE ("Doesn't begin with [-0-9]");
//@     }

//@   T result;

//@   auto [ptr, ec] = std::from_chars (s.cbegin (), s.cend (), result);

//@   if (ec != std::errc ())
//@     {
//@       _LIBSH_TREIS_THROW_MESSAGE ("Not a valid number (std::from_chars returned error)");
//@     }

//@   if (ptr != s.cend ())
//@     {
//@       _LIBSH_TREIS_THROW_MESSAGE ("There is a garbagge after number");
//@     }

//@   return result;
//@ }
//@ }

// Не возвращаем результат [v]asprintf'а, т. к. его можно получить за O(1), вызвав size () у строки. Работает, даже если есть нулевые байты
//@ #include <stdarg.h>
//@ #include <string>
#include <stdlib.h>
namespace libsh_treis::libc //@
{ //@
std::string //@
x_vasprintf (const char *fmt, va_list ap)//@;
{
  char *str;

  int length = libsh_treis::libc::no_raii::x_vasprintf (&str, fmt, ap);

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
x_asprintf (const char *fmt, ...)//@;
{
  va_list ap;
  va_start (ap, fmt);
  std::string result = x_vasprintf (fmt, ap);
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
      _LIBSH_TREIS_THROW_MESSAGE ("Process status is not \"exited\"");
    }

  if (WEXITSTATUS (status) != EXIT_SUCCESS)
    {
      _LIBSH_TREIS_THROW_MESSAGE ("Process exit code is not 0");
    }
}
} //@

// Деструктор всегда делает pclose
//@ #include <stdio.h>
//@ namespace libsh_treis::libc
//@ {
//@ class pipe_stream: libsh_treis::tools::not_movable
//@ {
//@   FILE *_stream;
//@   int _exceptions;

//@ public:

//@   explicit pipe_stream (FILE *stream) noexcept : _stream (stream), _exceptions (std::uncaught_exceptions ())
//@   {
//@   }

//@   ~pipe_stream (void) noexcept (false)
//@   {
//@     if (std::uncaught_exceptions () == _exceptions)
//@       {
//@         process_succeed (x_pclose (_stream));
//@       }
//@     else
//@       {
//@         pclose (_stream);
//@       }
//@   }

//@   FILE *
//@   resource (void) const noexcept
//@   {
//@     return _stream;
//@   }
//@ };
//@ }

namespace libsh_treis::libc //@
{ //@
pipe_stream //@
x_popen (const char *command, const char *mode)//@;
{
  return pipe_stream (libsh_treis::libc::no_raii::x_popen (command, mode));
}
} //@

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
x_pipe (void)//@;
{
  auto result = libsh_treis::libc::no_raii::x_pipe ();

  return { .readable = std::unique_ptr<fd> (new fd (result.readable)), .writable = std::unique_ptr<fd> (new fd (result.writable)) };
}
} //@

// Вызывающая сторона должна сама flush'нуть C stdio и C++ streams перед вызовом этой функции. В том числе flush'нуть C stderr, т. к. он используется моей либой (если туда был вывод без '\n' в конце)
//@ #include <sys/types.h>
#include <stdlib.h>
namespace libsh_treis::libc::no_raii //@
{ //@
pid_t //@
safe_fork (const std::function<void(void)> &func)//@;
{
  pid_t result = libsh_treis::libc::no_raii::x_fork ();

  if (result == 0)
    {
      _Exit (libsh_treis::tools::main_helper (func));
    }

  return result;
}
} //@

//@ #include <sys/types.h>
namespace libsh_treis::libc //@
{ //@
int //@
x_waitpid_status (pid_t pid, int options)//@;
{
  int result;

  x_waitpid (pid, &result, options);

  return result;
}
} //@

// Деструктор всегда делает waitpid
//@ #include <sys/types.h>
//@ #include <sys/wait.h>
//@ namespace libsh_treis::libc
//@ {
//@ class process : libsh_treis::tools::not_movable
//@ {
//@   pid_t _pid;
//@   int _exceptions;

//@ public:

//@   explicit process (pid_t pid) noexcept : _pid (pid), _exceptions (std::uncaught_exceptions ())
//@   {
//@   }

//@   ~process (void) noexcept (false)
//@   {
//@     if (std::uncaught_exceptions () == _exceptions)
//@       {
//@         process_succeed (x_waitpid_status (_pid, 0));
//@       }
//@     else
//@       {
//@         waitpid (_pid, nullptr, 0);
//@       }
//@   }

//@   pid_t
//@   resource (void) const noexcept
//@   {
//@     return _pid;
//@   }
//@ };
//@ }

namespace libsh_treis::libc //@
{ //@
process //@
safe_fork (const std::function<void(void)> &func)//@;
{
  return process (libsh_treis::libc::no_raii::safe_fork (func));
}
} //@

//@ #include <memory>
namespace libsh_treis::libc //@
{ //@
int //@
x_waitpid_raii (std::unique_ptr<process> proc, int options)//@;
{
  LIBSH_TREIS_ASSERT (proc != nullptr);

  process *ptr = proc.release ();

  int result = x_waitpid_status (ptr->resource (), options);

  operator delete (ptr);

  return result;
}
} //@

// x_execv_string и x_execvp_string - для range'а объектов, у которых есть c_str ()

//@ #include <vector>
//@ namespace libsh_treis::libc
//@ {
//@ template <typename Iter> [[noreturn]] void
//@ x_execv_string_nunu (const char *path, Iter b, Iter e)
//@ {
//@   std::vector<const char *> v;
//@   for (; b != e; ++b)
//@     {
//@       v.push_back (b->c_str ());
//@     }
//@   v.push_back (nullptr);
//@   x_execv_nunu (path, v.data ());
//@ }
//@ }

//@ #include <vector>
//@ namespace libsh_treis::libc
//@ {
//@ template <typename Iter> [[noreturn]] void
//@ x_execvp_string (const char *file, Iter b, Iter e)
//@ {
//@   std::vector<const char *> v;
//@   for (; b != e; ++b)
//@     {
//@       v.push_back (b->c_str ());
//@     }
//@   v.push_back (nullptr);
//@   x_execvp (file, v.data ());
//@ }
//@ }

//@ // Выделяет память для массива, инициализируя с помощью default initializing. Независимо от того, что написано в стандарте. Добавил, т. к. не нашёл подобной функции в моей реализации стандартной библиотеки C++. Когда будет везде, надо будет удалить
//@ #include <cstddef>
//@ #include <memory>
//@ #include <type_traits>
//@ namespace libsh_treis::tools
//@ {
//@ template <typename T> std::unique_ptr<T>
//@ make_unique_default_init (std::size_t size)
//@ {
//@   static_assert (std::is_unbounded_array_v<T>);
//@   return std::unique_ptr<T> (new std::remove_extent_t<T>[size]);
//@ }
//@ }

//@ // Owning span
//@ // Предназначен, например, для хранения данных сразу после чтения read'ом или перед записыванием write'ом. От unique_ptr<T[]> отличается отсутствием особого состояния и хранением длины
//@ // От std::vector отличается отсутствием особого состояния (vector можно переместить и оставить исходный объект в пустом состоянии)
//@ // Можно создать ospan нулевой длины. Но идеоматический код не должен использовать такой ospan как маркер отсутствия ospan'а
//@ // ospan нельзя перемещать. Копировать можно, но на данный момент не реализовано
//@ #include <cstddef>
//@ #include <type_traits>
//@ #include <utility>
//@ namespace libsh_treis::tools
//@ {
//@ template <typename T> class ospan: libsh_treis::tools::not_movable
//@ {
//@   static_assert (!std::is_unbounded_array_v<T>);

//@   T *_ptr;
//@   std::size_t _size;

//@   explicit ospan (T *ptr, std::size_t size) noexcept : _ptr (ptr), _size (size)
//@   {
//@   }

//@   template <typename U> friend ospan<U>
//@   make_ospan (std::size_t size);

//@   template <typename U> friend ospan<U>
//@   make_ospan_for_overwrite (std::size_t size);

//@   template <typename U> friend void
//@   swap (ospan<U> &, ospan<U> &) noexcept;

//@ public:
//@   ~ospan (void)
//@   {
//@     delete [] _ptr;
//@   }

//@   const T *
//@   data (void) const noexcept
//@   {
//@     return _ptr;
//@   }

//@   T *
//@   data (void) noexcept
//@   {
//@     return _ptr;
//@   }

//@   std::size_t
//@   size (void) const noexcept
//@   {
//@     return _size;
//@   }

//@   const T &
//@   operator[] (std::size_t i) const noexcept
//@   {
//@     assert (i < _size);
//@     return _ptr[i];
//@   }

//@   T &
//@   operator[] (std::size_t i) noexcept
//@   {
//@     assert (i < _size);
//@     return _ptr[i];
//@   }
//@ };

//@ template <typename T> void
//@ swap (ospan<T> &a, ospan<T> &b) noexcept
//@ {
//@   std::swap (a._ptr,  b._ptr);
//@   std::swap (a._size, b._size);
//@ }

//@ template <typename T> ospan<T>
//@ make_ospan (std::size_t size)
//@ {
//@   return ospan<T> (new T[size](), size);
//@ }

//@ template <typename T> ospan<T>
//@ make_ospan_for_overwrite (std::size_t size)
//@ {
//@   return ospan<T> (new T[size], size);
//@ }
//@ }

// Один из возможных алгоритмов правильного соединения компонентов пути. Используется тот алгоритм, который использует GNU find
//@ #include <string_view>
namespace libsh_treis::tools //@
{ //@
std::string //@
build_path_find (std::string_view up, std::string_view down)//@;
{
  LIBSH_TREIS_ASSERT (!up.empty ());
  LIBSH_TREIS_ASSERT (!down.empty ());
  LIBSH_TREIS_ASSERT (down.front () != '/');

  if (up.back () == '/')
    {
      return std::string (up) + std::string (down);
    }
  else
    {
      return std::string (up) + "/" + std::string (down);
    }
}
} //@

//@ #include <dirent.h>
//@ namespace libsh_treis::libc
//@ {
//@ class directory: libsh_treis::tools::not_movable
//@ {
//@   DIR *_d;
//@   int _exceptions;

//@ public:

//@   explicit directory (DIR *d) noexcept : _d (d), _exceptions (std::uncaught_exceptions ())
//@   {
//@   }

//@   ~directory (void) noexcept (false)
//@   {
//@     if (std::uncaught_exceptions () == _exceptions)
//@       {
//@         x_closedir (_d);
//@       }
//@     else
//@       {
//@         closedir (_d);
//@       }
//@   }

//@   DIR *
//@   resource (void) const noexcept
//@   {
//@     return _d;
//@   }
//@ };
//@ }

namespace libsh_treis::libc //@
{ //@
directory //@
x_opendir (const char *dirname)//@;
{
  return directory (libsh_treis::libc::no_raii::x_opendir (dirname));
}
} //@

//@ #include <span>
//@ #include <cstddef>
#include <string.h>
namespace libsh_treis::libc //@
{ //@
int //@
span_memcmp (std::span<const std::byte> s1, std::span<const std::byte> s2) noexcept//@;
{
  LIBSH_TREIS_ASSERT (s1.size () == s2.size ());
  return memcmp (s1.data (), s2.data (), s1.size ());
}
} //@

//@ #include <span>
//@ #include <cstddef>
#include <string.h>
namespace libsh_treis::libc //@
{ //@
void //@
span_memcpy (std::span<std::byte> dest, std::span<const std::byte> src) noexcept//@;
{
  LIBSH_TREIS_ASSERT (dest.size () == src.size ());
  memcpy (dest.data (), src.data (), dest.size ());
}
} //@
