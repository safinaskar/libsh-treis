// Нет никаких ограничений на типы, которые можно положить в enum_variant, кроме указанных в requires
// LIBSH_TREIS_SWITCH работает так же, как обычный switch или обычный statement: все деструкторы временных объектов вызываются после вычисления выражения, переданного в LIBSH_TREIS_SWITCH. Сам enum_variant, переданный в LIBSH_TREIS_SWITCH запоминается с правильным ссылочным типом, и далее биндинги инициализируются с правильным ссылочным типом. Внутри LIBSH_TREIS_SWITCH и LIBSH_TREIS_CASE можно использовать обычные break, return и т. д.

#pragma once

#include <assert.h>

#include <type_traits>
#include <utility>
#include <tuple>
#include <variant>

#include "libsh-treis.hpp"

namespace libsh_treis::tools
{
template <auto Tag> requires std::is_enum_v<decltype (Tag)> struct in_place_tag_t
{
};

template <auto Tag> inline constexpr in_place_tag_t<Tag> tg;

template <typename Tag, typename... Types>
  requires
    (sizeof... (Types) > 0) &&
    (libsh_treis::tools::Cpp17Destructible<Types> && ...) &&
    ((sizeof (Types) > 0) && ...) &&
    (!std::is_const_v<Types> && ...) &&
    (!std::is_volatile_v<Types> && ...)
class enum_variant
{
  std::variant<Types...> _v;

public:
  typedef Tag tag_t;

  // Конструктор, а не статический метод, чтобы удобно инициализировать поля структуры с помощью designated initializers
  template <Tag Tg, typename... Args> explicit enum_variant (in_place_tag_t<Tg>, Args &&... args) : _v (std::in_place_index<std::size_t (Tg)>, std::forward<Args> (args)...)
  {
  }

  enum_variant (enum_variant &&) = default;
  enum_variant (const enum_variant &) = delete; //WR
  enum_variant &operator= (enum_variant &&) = default;
  enum_variant &operator= (const enum_variant &) = delete; //WR

  ~enum_variant (void) noexcept
  {
  }

  template <Tag Tg, typename... Args> void
  e (Args &&... args)
  {
    _v.template emplace<std::size_t (Tg)> (std::forward<Args> (args)...);
  }

  Tag
  t (void) const noexcept
  {
    return Tag (_v.index ());
  }

  template <Tag Tg> const auto &
  unsafe_get (void) const noexcept
  {
    assert (t () == Tg);
    return std::get<std::size_t (Tg)> (_v);
  }

  template <Tag Tg> auto &
  unsafe_get (void) noexcept
  {
    assert (t () == Tg);
    return std::get<std::size_t (Tg)> (_v);
  }

  template <Tag Tg> auto
  get_if (void) const noexcept
  {
    return std::get_if<std::size_t (Tg)> (&_v);
  }

  template <Tag Tg> auto
  get_if (void) noexcept
  {
    return std::get_if<std::size_t (Tg)> (&_v);
  }

  template <Tag Tg> const auto &
  get_assert (void) const noexcept
  {
    LIBSH_TREIS_ASSERT (t () == Tg);
    return std::get<std::size_t (Tg)> (_v);
  }

  template <Tag Tg> auto &
  get_assert (void) noexcept
  {
    LIBSH_TREIS_ASSERT (t () == Tg);
    return std::get<std::size_t (Tg)> (_v);
  }

  bool
  operator== (const enum_variant &rhs) const
  {
    return _v == rhs._v;
  }

  bool
  operator!= (const enum_variant &rhs) const
  {
    return _v != rhs._v;
  }
};
}

#define LIBSH_TREIS_SWITCH(v) switch (auto &&_libsh_treis_switch = (v); _libsh_treis_switch.t ())

#define LIBSH_TREIS_CASE(tg, ...) \
  if (true) \
    { \
      case ::std::remove_reference_t<decltype (_libsh_treis_switch)>::tag_t::tg: \
      auto &&__VA_ARGS__ = _libsh_treis_switch.template unsafe_get<::std::remove_reference_t<decltype (_libsh_treis_switch)>::tag_t::tg> (); \
      if (true) \

#define LIBSH_TREIS_END_CASE \
      else \
        ; \
      break; \
    } \
  else \
    do \
      ; \
    while (false)

/* Код ниже демонстрирует пример использования */
namespace libsh_treis::tools::detail
{
namespace
{
inline void
example_c42d4084 (void)
{
  namespace tt = libsh_treis::tools;
  enum class foo {a, b, c, d};
  tt::enum_variant<foo, std::monostate, std::monostate, int, std::tuple<int, int>> v (tt::tg<foo::a>);
  LIBSH_TREIS_SWITCH (v)
    {
      case foo::a:
      case foo::b:
        break; // Здесь нужно ставить break, как обычно
      LIBSH_TREIS_CASE (c, x)
        {
          // Здесь можно не ставить break
          x = 0; // Это изменит сам enum_variant
        }
      LIBSH_TREIS_END_CASE;
      LIBSH_TREIS_CASE (d, [x, y])
        {
          x = 0;
        }
      LIBSH_TREIS_END_CASE;
    }
}
}
}
