#pragma once

#include <cstddef>

#include <type_traits>

#include "libsh-treis.hpp"

namespace libsh_treis::tools
{
// Двумерный массив с измерениями, известными в runtime. Хранится в динамической памяти
// Для элементов, которые можно копировать memcpy и сравнивать memcmp (а значит, padding быть не должно либо padding должен быть одним и тем же)
// Например, подходит для изображений (если не напутали с padding)
// Тем не менее не пытается придать никакого смысла элементам. Если нужен класс для работы с изображениями, учитывающий pixel format, нужен другой класс
// Класс владеющий. Нет особого состояния
// Сперва хранится первая строчка (row) целиком, потом - вторая и т. д. Сперва указываем количество строк, затем - столбцов. Сперва указываем номер строки, потом - столбца
template <typename T> class array2d: libsh_treis::tools::not_movable
{
  static_assert (!std::is_unbounded_array_v<T>);

  T *_ptr;
  std::ptrdiff_t _rows, _cols;

  explicit array2d (T *ptr, std::ptrdiff_t rows, std::ptrdiff_t cols) noexcept : _ptr (ptr), _rows (rows), _cols (cols)
  {
  }

  template <typename U> friend array2d<U>
  make_array2d_for_overwrite (std::ptrdiff_t rows, std::ptrdiff_t cols);

public:
  ~array2d (void)
  {
    delete [] _ptr;
  }

  const T *
  data (void) const noexcept
  {
    return _ptr;
  }

  T *
  data (void) noexcept
  {
    return _ptr;
  }

  std::span<const T>
  span (void) const noexcept
  {
    return {_ptr, std::size_t (_rows * _cols)};
  }

  std::span<T>
  span (void) noexcept
  {
    return {_ptr, std::size_t (_rows * _cols)};
  }

  std::ptrdiff_t
  rows (void) const noexcept
  {
    return _rows;
  }

  std::ptrdiff_t
  cols (void) const noexcept
  {
    return _cols;
  }
};

template <typename T> array2d<T>
make_array2d_for_overwrite (std::ptrdiff_t rows, std::ptrdiff_t cols)
{
  return array2d<T> (new T[rows * cols], rows, cols);
}

// Не оптимизировано для случая, когда можно соединить несколько memcmp в один
template <typename T> bool
subarray2d_eq (const array2d<T> &a, std::ptrdiff_t arb, std::ptrdiff_t acb, std::ptrdiff_t are, std::ptrdiff_t ace, const array2d<T> &b, std::ptrdiff_t brb, std::ptrdiff_t bcb, std::ptrdiff_t bre, std::ptrdiff_t bce)
{
  LIBSH_TREIS_ASSERT (0 <= arb && arb <= are && are <= a.rows ());
  LIBSH_TREIS_ASSERT (0 <= acb && acb <= ace && ace <= a.cols ());
  LIBSH_TREIS_ASSERT (0 <= brb && brb <= bre && bre <= b.rows ());
  LIBSH_TREIS_ASSERT (0 <= bcb && bcb <= bce && bce <= b.cols ());
  LIBSH_TREIS_ASSERT (are - arb == bre - brb);
  LIBSH_TREIS_ASSERT (ace - acb == bce - bcb);

  for (std::ptrdiff_t i = 0; i != are - arb; ++i)
    {
      if (memcmp (a.data () + (arb + i) * a.cols () + acb, b.data () + (brb + i) * b.cols () + bcb, (ace - acb) * sizeof (T)) != 0)
        {
          return false;
        }
    }

  return true;
}
}
