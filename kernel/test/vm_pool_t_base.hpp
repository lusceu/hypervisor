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

#ifndef VM_POOL_T_BASE_HPP
#define VM_POOL_T_BASE_HPP

#include <bsl/discard.hpp>
#include <bsl/safe_integral.hpp>

namespace mk
{
    /// @class mk::vm_pool_t_base
    ///
    /// <!-- description -->
    ///   @brief Provides the base vm_pool_t for testing.
    ///
    template<typename derived>
    class vm_pool_t_base
    {
    public:
        /// <!-- description -->
        ///   @brief Returns true if the requested vm_t is deallocated, false
        ///     if the provided VMID is invalid, or if the vm_t is not
        ///     deallocated.
        ///
        /// <!-- inputs/outputs -->
        ///   @param vmid the ID of the vm_t to query
        ///   @return Returns true if the requested vm_t is deallocated, false
        ///     if the provided VMID is invalid, or if the vm_t is not
        ///     deallocated.
        ///
        [[nodiscard]] static constexpr auto
        is_deallocated(bsl::safe_uint16 const &vmid) noexcept -> bool
        {
            bsl::discard(vmid);
            return false;
        }

        /// <!-- description -->
        ///   @brief Returns true if the requested vm_t is allocated, false
        ///     if the provided VMID is invalid, or if the vm_t is not
        ///     allocated.
        ///
        /// <!-- inputs/outputs -->
        ///   @param vmid the ID of the vm_t to query
        ///   @return Returns true if the requested vm_t is allocated, false
        ///     if the provided VMID is invalid, or if the vm_t is not
        ///     allocated.
        ///
        [[nodiscard]] static constexpr auto
        is_allocated(bsl::safe_uint16 const &vmid) noexcept -> bool
        {
            bsl::discard(vmid);
            return true;
        }

        /// <!-- description -->
        ///   @brief Returns true if the requested vm_t is a zombie, false
        ///     if the provided VMID is invalid, or if the vm_t is not
        ///     a zombie.
        ///
        /// <!-- inputs/outputs -->
        ///   @param vmid the ID of the vm_t to query
        ///   @return Returns true if the requested vm_t is a zombie, false
        ///     if the provided VMID is invalid, or if the vm_t is not
        ///     a zombie.
        ///
        [[nodiscard]] static constexpr auto
        is_zombie(bsl::safe_uint16 const &vmid) noexcept -> bool
        {
            bsl::discard(vmid);
            return false;
        }
    };
}

#endif
