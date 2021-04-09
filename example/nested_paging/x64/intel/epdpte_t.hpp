/// @copyright
/// Copyright (C) 2020 Assured Information Security, Inc.
///
/// @copyright
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// @copyright
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// @copyright
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
/// SOFTWARE.

#ifndef EPDPTE_T_HPP
#define EPDPTE_T_HPP

#include <bsl/convert.hpp>
#include <bsl/cstdint.hpp>
#include <bsl/safe_integral.hpp>

#pragma pack(push, 1)

namespace example
{
    /// @struct example::epdpte_t
    ///
    /// <!-- description -->
    ///   @brief Defines the layout of a nested page-directory-pointer table
    ///     entry (EPDPTE).
    ///
    struct epdpte_t final
    {
        /// @brief defines the "read access" field in the page
        bsl::uint64 r : static_cast<bsl::uint64>(1);
        /// @brief defines the "write access" field in the page
        bsl::uint64 w : static_cast<bsl::uint64>(1);
        /// @brief defines the "execute access" field in the page
        bsl::uint64 e : static_cast<bsl::uint64>(1);
        /// @brief defines the "must be zero" field in the page
        bsl::uint64 mbz1 : static_cast<bsl::uint64>(5);
        /// @brief defines the "accessed" field in the page
        bsl::uint64 a : static_cast<bsl::uint64>(1);
        /// @brief defines the "ignored" field in the page
        bsl::uint64 ignored1 : static_cast<bsl::uint64>(1);
        /// @brief defines the "user execute access" field in the page
        bsl::uint64 e_user : static_cast<bsl::uint64>(1);
        /// @brief defines the "ignored" field in the page
        bsl::uint64 ignored2 : static_cast<bsl::uint64>(1);
        /// @brief defines the "physical address" field in the page
        bsl::uint64 phys : static_cast<bsl::uint64>(40);
        /// @brief defines the "must be zero" field in the page
        bsl::uint64 mbz2 : static_cast<bsl::uint64>(12);
    };
}

#pragma pack(pop)

#endif
