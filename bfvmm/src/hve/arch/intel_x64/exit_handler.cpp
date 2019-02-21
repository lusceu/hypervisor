//
// Copyright (C) 2019 Assured Information Security, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// TIDY_EXCLUSION=-cppcoreguidelines-pro-type-reinterpret-cast
//
// Reason:
//     Although in general this is a good rule, for hypervisor level code that
//     interfaces with the kernel, and raw hardware, this rule is
//     impractical.
//

#include <bfgsl.h>
#include <bfexception.h>

#include <hve/arch/intel_x64/vcpu.h>
#include <hve/arch/intel_x64/exit_handler.h>

namespace bfvmm::intel_x64
{

void
exit_handler::init(gsl::not_null<vcpu *> vcpu)
{ bfignored(vcpu);
handle(vcpu, this);
}

void
exit_handler::fini(gsl::not_null<vcpu *> vcpu) noexcept
{ bfignored(vcpu); }

void
exit_handler::add_handler(
    ::intel_x64::vmcs::value_type reason,
    const handler_delegate_t &d)
{ m_exit_handlers_array.at(reason).push_front(d); }

void
exit_handler::add_exit_handler(
    const handler_delegate_t &d)
{ m_exit_handlers.push_front(d); }

bool
exit_handler::handle(
    vcpu *vcpu, exit_handler *exit_handler)
{
    using namespace ::intel_x64::vmcs;
bfline
    guard_exceptions([&]() {
bfline
        for (const auto &d : exit_handler->m_exit_handlers) {
            d(vcpu);
        }
bfline
        const auto &handlers =
            exit_handler->m_exit_handlers_array.at(
                exit_reason::basic_exit_reason::get()
            );
bfline
        for (const auto &d : handlers) {
bfline
            if (d(vcpu)) {
bfline
                vcpu->run();
            }
        }
    });

    vcpu->halt("unhandled vm exit");

    // Unreachable
    return false;
}

}
