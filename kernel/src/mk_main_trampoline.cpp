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

#include <global_resources.hpp>
#include <mk_args_t.hpp>
#include <tls_t.hpp>

#include <bsl/cstdint.hpp>
#include <bsl/cstdio.hpp>
#include <bsl/cstring.hpp>
#include <bsl/exit_code.hpp>
#include <bsl/touch.hpp>

extern "C"
{
    /// @brief stores the TLS blocks used by the microkernel.
    alignas(HYPERVISOR_PAGE_SIZE) constinit bsl::
        array<mk::tls_t, bsl::to_umax(HYPERVISOR_MAX_PPS).get()> g_tls_blocks{};
}

namespace mk
{
    /// <!-- description -->
    ///   @brief Provides the main entry point of the microkernel. This
    ///     function is called by the loader and is responsible for starting
    ///     the hypervisor on a specific core
    ///
    /// <!-- inputs/outputs -->
    ///   @param args the loader provided arguments to the microkernel.
    ///   @param tls the current TLS block
    ///   @return Returns bsl::exit_success on success, bsl::exit_failure
    ///     otherwise
    ///
    [[nodiscard]] extern "C" auto
    mk_main_trampoline(loader::mk_args_t *const args, tls_t *const tls) noexcept -> bsl::exit_code
    {
        return g_mk_main.process(args, *tls);
    }
}
