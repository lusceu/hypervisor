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

#include "integration_utils.hpp"

#include <arch_support.hpp>
#include <mk_interface.hpp>

#include <bsl/debug.hpp>
#include <bsl/exit_code.hpp>
#include <bsl/safe_integral.hpp>
#include <bsl/unlikely.hpp>

namespace integration
{
    /// @brief stores the handle the extension will use
    constinit inline syscall::bf_handle_t g_handle{};

    /// <!-- description -->
    ///   @brief Implements the VMExit entry function.
    ///
    /// <!-- inputs/outputs -->
    ///   @param vpsid the ID of the VPS that generated the VMExit
    ///   @param exit_reason the exit reason associated with the VMExit
    ///
    void
    // NOLINTNEXTLINE(bsl-non-safe-integral-types-are-forbidden)
    vmexit_entry(bsl::uint16 const vpsid, bsl::uint64 const exit_reason) noexcept
    {
        bsl::discard(vpsid);
        bsl::discard(exit_reason);

        bsl::error() << "extension purposely dereferencing nullptr. fault expected\n";
        bool *i{};
        // This is intentional as it is what we are testing.
        // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
        *i = true;
    }

    /// <!-- description -->
    ///   @brief Implements the fast fail entry function.
    ///
    /// <!-- inputs/outputs -->
    ///   @param fail_reason the exit reason associated with the fail
    ///
    void
    // NOLINTNEXTLINE(bsl-non-safe-integral-types-are-forbidden)
    fail_entry(syscall::bf_status_t::value_type const fail_reason) noexcept
    {
        bsl::discard(fail_reason);
        syscall::bf_control_op_exit();
    }

    /// <!-- description -->
    ///   @brief Implements the bootstrap entry function.
    ///
    /// <!-- inputs/outputs -->
    ///   @param ppid the physical process to bootstrap
    ///
    void
    // NOLINTNEXTLINE(bsl-non-safe-integral-types-are-forbidden)
    bootstrap_entry(bsl::uint16 const ppid) noexcept
    {
        bsl::errc_type ret{};

        bsl::safe_uint16 vpid{};
        bsl::safe_uint16 vpsid{};

        ret = syscall::bf_vp_op_create_vp(g_handle, syscall::BF_ROOT_VMID, ppid, vpid);
        integration::require_success(ret);

        ret = syscall::bf_vps_op_create_vps(g_handle, vpid, ppid, vpsid);
        integration::require_success(ret);

        ret = syscall::bf_vps_op_init_as_root(g_handle, vpsid);
        integration::require_success(ret);

        ret = init_vps(g_handle, vpsid);
        integration::require_success(ret);

        ret = syscall::bf_vps_op_run(g_handle, syscall::BF_ROOT_VMID, vpid, vpsid);
        integration::require_success(ret);

        bsl::print<bsl::V>() << bsl::here();
        syscall::bf_control_op_exit();
    }

    /// <!-- description -->
    ///   @brief Implements the main entry function for this integration
    ///     test
    ///
    /// <!-- inputs/outputs -->
    ///   @param version the version of the spec implemented by the
    ///     microkernel. This can be used to ensure the extension and the
    ///     microkernel speak the same ABI.
    ///
    extern "C" void
    ext_main_entry(bsl::uint32 const version) noexcept
    {
        bsl::errc_type ret{};

        if (bsl::unlikely(!syscall::bf_is_spec1_supported(version))) {
            bsl::error() << "integration test not supported\n" << bsl::here();
            return syscall::bf_control_op_exit();
        }

        ret = syscall::bf_handle_op_open_handle(syscall::BF_SPEC_ID1_VAL, g_handle);
        integration::require_success(ret);

        ret = syscall::bf_callback_op_register_bootstrap(g_handle, &bootstrap_entry);
        integration::require_success(ret);

        ret = syscall::bf_callback_op_register_vmexit(g_handle, &vmexit_entry);
        integration::require_success(ret);

        ret = syscall::bf_callback_op_register_fail(g_handle, &fail_entry);
        integration::require_success(ret);

        syscall::bf_control_op_wait();
    }
}
