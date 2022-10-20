#pragma once
#include <cstddef>
#include <cstdlib>

#ifdef USE_ONEDPL // oneDPL C++17 PSTL
  #include <oneapi/dpl/algorithm>
  #include <oneapi/dpl/execution>
  #if ONEDPL_USE_DPCPP_BACKEND
    #include <CL/sycl.hpp>
namespace shim {
const auto par_unseq = oneapi::dpl::execution::device_policy<>{oneapi::dpl::execution::make_device_policy(cl::sycl::default_selector{})};
template <typename T> T *alloc_raw(size_t size) { return sycl::malloc_shared<T>(size, par_unseq.queue()); }
template <typename T> void dealloc_raw(T *ptr) { sycl::free(ptr, par_unseq.queue()); }
} // namespace shim
  #else
namespace shim {
static constexpr auto par_unseq = dpl::execution::par_unseq;
}
    #define USE_STD_PTR_ALLOC_DEALLOC
  #endif
#else // Normal C++17 PSTL
  #include <algorithm>
  #include <execution>
namespace shim {
static constexpr auto par_unseq = std::execution::par_unseq;
}
  #define USE_STD_PTR_ALLOC_DEALLOC
#endif
#ifdef USE_STD_PTR_ALLOC_DEALLOC
namespace shim {
template <typename T> T *alloc_raw(size_t size) { return reinterpret_cast<T *>(std::malloc(sizeof(T) * size)); }
template <typename T> void dealloc_raw(T *ptr) { std::free(ptr); }
} // namespace shim
#endif

namespace shim {
// A lightweight counting iterator which will be used by the STL algorithms
// NB: C++ <= 17 doesn't have this built-in, and it's only added later in ranges-v3 (C++2a) which this
// implementation doesn't target
template <typename N> struct ranged {
  class iterator {
    friend class ranged;

  public:
    using difference_type = N;
    using value_type = N;
    using pointer = const N *;
    using reference = N;
    using iterator_category = std::random_access_iterator_tag;
    // XXX This is not part of the iterator spec, it gets picked up by oneDPL if enabled.
    // Without this, the DPL SYCL backend collects the iterator data on the host and copies to the device.
    // This type is unused for any other STL impl.
    using is_passed_directly = std::true_type;
    // clang-format off
    reference operator *() const { return i_; }
    iterator &operator ++() { ++i_; return *this; }
    iterator operator ++(int) { iterator copy(*this); ++i_; return copy; }
    iterator &operator --() { --i_; return *this; }
    iterator operator --(int) { iterator copy(*this); --i_; return copy; }
    iterator &operator +=(N by) { i_+=by; return *this; }
    value_type operator[](const difference_type &i) const { return i_ + i; }
    difference_type operator-(const iterator &it) const { return i_ - it.i_; }
    iterator operator+(const value_type v) const { return iterator(i_ + v); }
    bool operator ==(const iterator &other) const { return i_ == other.i_; }
    bool operator !=(const iterator &other) const { return i_ != other.i_; }
    bool operator < (const iterator &other) const { return i_ < other.i_; }
    // clang-format on
  protected:
    explicit iterator(N start) : i_(start) {}

  private:
    N i_;
  };
  [[nodiscard]] iterator begin() const { return begin_; }
  [[nodiscard]] iterator end() const { return end_; }
  ranged(N begin, N end) : begin_(begin), end_(end) {}

private:
  iterator begin_, end_;
};
} // namespace shim