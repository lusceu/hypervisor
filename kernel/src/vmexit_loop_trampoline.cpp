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
#include <tls_t.hpp>
#include <vmexit_loop.hpp>

#include <bsl/exit_code.hpp>

namespace mk
{
    /// <!-- description -->
    ///   @brief Since we cannot create a template function pointer, we need
    ///     a way to call a template function from our ASM entry point.
    ///     Normally the way this works in a normal program is the OS calls
    ///     _start, which then calls main(). The main() function, which is
    ///     an extern C function, similar to this function, can then call a
    ///     template function as needed. So the whole point of this function
    ///     is to simply trampoline from our ASM logic, to a C++ template
    ///     function that is easy to test.
    ///
    /// <!-- inputs/outputs -->
    ///   @param tls the current TLS block
    ///   @return Returns bsl::exit_success on success, bsl::exit_failure
    ///     otherwise
    ///
    extern "C" [[nodiscard]] auto
    vmexit_loop_trampoline(tls_t *const tls) noexcept -> bsl::exit_code
    {
        return vmexit_loop(
            *tls,
            *static_cast<mk_ext_type *>(tls->ext_vmexit),
            g_intrinsic,
            g_vps_pool,
            g_vmexit_log);
    }
}
