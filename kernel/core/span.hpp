#pragma once

#include <cstddef>

/* Non-owning view over a contiguous range (OS_MASTER_SPEC.md §4.3: "prefer
 * non-owning views"). Deliberately not std::span: no <span> without
 * pulling in a hosted STL, and this kernel isn't guaranteed a C++20
 * standard library implementation even where the compiler is C++20.
 *
 * operator[]/subspan do not bounds-check (same contract as std::span) —
 * out-of-range access is caller error, not a reportable runtime condition,
 * and there's no exceptions to report it with anyway (§4.3). Call sites
 * that can't prove bounds by construction should check with KASSERT
 * (kernel/core/assert.h) themselves rather than expecting it here. */
template <typename T>
class Span {
public:
    constexpr Span() : data_(nullptr), size_(0) {}
    constexpr Span(T *data, size_t size) : data_(data), size_(size) {}

    constexpr T *data() const { return data_; }
    constexpr size_t size() const { return size_; }
    constexpr bool empty() const { return size_ == 0; }

    T &operator[](size_t index) const { return data_[index]; }

    T *begin() const { return data_; }
    T *end() const { return data_ + size_; }

    /* Sub-view; caller is responsible for offset/count staying in bounds
     * (see the class comment — no exceptions to report a violation with). */
    constexpr Span<T> subspan(size_t offset, size_t count) const {
        return Span<T>(data_ + offset, count);
    }

private:
    T *data_;
    size_t size_;
};
