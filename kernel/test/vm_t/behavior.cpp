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

#include "../../src/vm_t.hpp"
#include "../ext_pool_t_signal_vm_created_failure.hpp"
#include "../ext_pool_t_signal_vm_destroyed_failure.hpp"
#include "../ext_pool_t_success.hpp"
#include "../vp_pool_t_failure.hpp"
#include "../vp_pool_t_success.hpp"

#include <tls_t.hpp>

#include <bsl/ut.hpp>

namespace mk
{
    /// @brief defines the max number of PPs used in testing
    constexpr bsl::safe_uint16 INTEGRATION_MAX_PPS{bsl::to_u16(3)};

    /// @brief defines VMID0
    constexpr bsl::safe_uint16 VMID0{bsl::to_u16(0)};
    /// @brief defines VMID1
    constexpr bsl::safe_uint16 VMID1{bsl::to_u16(1)};

    /// <!-- description -->
    ///   @brief Used to execute the actual checks. We put the checks in this
    ///     function so that we can validate the tests both at compile-time
    ///     and at run-time. If a bsl::ut_check fails, the tests will either
    ///     fail fast at run-time, or will produce a compile-time error.
    ///
    /// <!-- inputs/outputs -->
    ///   @return Always returns bsl::exit_success.
    ///
    [[nodiscard]] constexpr auto
    tests() noexcept -> bsl::exit_code
    {
        bsl::ut_scenario{"initialize invalid id version #1"} = []() {
            bsl::ut_given{} = []() {
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_then{} = [&vm]() {
                    bsl::ut_check(!vm.initialize(bsl::safe_uint16::zero(true)));
                };
            };
        };

        bsl::ut_scenario{"initialize invalid id version #2"} = []() {
            bsl::ut_given{} = []() {
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_then{} = [&vm]() {
                    bsl::ut_check(!vm.initialize(syscall::BF_INVALID_ID));
                };
            };
        };

        bsl::ut_scenario{"initialize success"} = []() {
            bsl::ut_given{} = []() {
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_then{} = [&vm]() {
                    bsl::ut_check(vm.initialize(VMID0));
                };
            };
        };

        bsl::ut_scenario{"initialize more than once failure"} = []() {
            bsl::ut_given{} = []() {
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&vm]() {
                    bsl::ut_required_step(vm.initialize(VMID0));
                    bsl::ut_then{} = [&vm]() {
                        bsl::ut_check(!vm.initialize(VMID0));
                    };
                };
            };
        };

        bsl::ut_scenario{"release of the root VM is ignored"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vp_pool_t_success vp_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID0));
                    bsl::ut_then{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                        bsl::ut_check(vm.release(tls, ext_pool, vp_pool));
                        bsl::ut_check(vm.id());
                    };
                };
            };
        };

        bsl::ut_scenario{"release success"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vp_pool_t_success vp_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_then{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                        bsl::ut_check(vm.release(tls, ext_pool, vp_pool));
                        bsl::ut_check(!vm.id());
                    };
                };
            };
        };

        bsl::ut_scenario{"release success after allocate"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vp_pool_t_success vp_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_then{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                        bsl::ut_check(vm.release(tls, ext_pool, vp_pool));
                        bsl::ut_check(!vm.id());
                        bsl::ut_check(!vm.is_allocated());
                    };
                };
            };
        };

        bsl::ut_scenario{"release of a zombie vm is ignored"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vp_pool_t_success vp_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    vm.zombify();
                    bsl::ut_then{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                        bsl::ut_check(vm.release(tls, ext_pool, vp_pool));
                        bsl::ut_check(vm.id());
                        bsl::ut_check(vm.is_zombie());
                    };
                };
            };
        };

        bsl::ut_scenario{"release of a vm that is still assigned results in a zombie"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vp_pool_t_failure vp_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_then{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                        bsl::ut_check(!vm.release(tls, ext_pool, vp_pool));
                        bsl::ut_check(vm.id());
                        bsl::ut_check(vm.is_zombie());
                    };
                };
            };
        };

        bsl::ut_scenario{"release of a vm that is still active results in a zombie"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vp_pool_t_success vp_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_required_step(vm.set_active(tls));
                    bsl::ut_then{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                        bsl::ut_check(!vm.release(tls, ext_pool, vp_pool));
                        bsl::ut_check(vm.id());
                        bsl::ut_check(vm.is_zombie());
                    };
                };
            };
        };

        bsl::ut_scenario{"release with extension failure results in zombie"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_signal_vm_destroyed_failure ext_pool{};
                vp_pool_t_success vp_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_then{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                        bsl::ut_check(!vm.release(tls, ext_pool, vp_pool));
                        bsl::ut_check(vm.id());
                        bsl::ut_check(vm.is_zombie());
                    };
                };
            };
        };

        bsl::ut_scenario{"allocate without initialize failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_then{} = [&tls, &ext_pool, &vm]() {
                    bsl::ut_check(!vm.allocate(tls, ext_pool));
                };
            };
        };

        bsl::ut_scenario{"allocate zombie failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    vm.zombify();
                    bsl::ut_then{} = [&tls, &ext_pool, &vm]() {
                        bsl::ut_check(!vm.allocate(tls, ext_pool));
                    };
                };
            };
        };

        bsl::ut_scenario{"allocate with extension failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_signal_vm_created_failure ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_then{} = [&tls, &ext_pool, &vm]() {
                        bsl::ut_check(!vm.allocate(tls, ext_pool));
                        bsl::ut_check(tls.state_reversal_required);
                        bsl::ut_check(tls.log_vmid == VMID1);
                    };
                };
            };
        };

        bsl::ut_scenario{"allocate success"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_then{} = [&tls, &ext_pool, &vm]() {
                        bsl::ut_check(vm.allocate(tls, ext_pool));
                        bsl::ut_check(tls.state_reversal_required);
                        bsl::ut_check(tls.log_vmid == VMID1);
                    };
                };
            };
        };

        bsl::ut_scenario{"allocate more than once failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID0));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_then{} = [&tls, &ext_pool, &vm]() {
                        bsl::ut_check(!vm.allocate(tls, ext_pool));
                    };
                };
            };
        };

        bsl::ut_scenario{"deallocate not initialized failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vp_pool_t_success vp_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_then{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                    bsl::ut_check(!vm.deallocate(tls, ext_pool, vp_pool));
                };
            };
        };

        bsl::ut_scenario{"deallocate root vm failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vp_pool_t_success vp_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID0));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_then{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                        bsl::ut_check(!vm.deallocate(tls, ext_pool, vp_pool));
                    };
                };
            };
        };

        bsl::ut_scenario{"deallocate zombie failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vp_pool_t_success vp_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    vm.zombify();
                    bsl::ut_then{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                        bsl::ut_check(!vm.deallocate(tls, ext_pool, vp_pool));
                    };
                };
            };
        };

        bsl::ut_scenario{"deallocate already deallocated failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vp_pool_t_success vp_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_required_step(vm.deallocate(tls, ext_pool, vp_pool));
                    bsl::ut_then{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                        bsl::ut_check(!vm.deallocate(tls, ext_pool, vp_pool));
                    };
                };
            };
        };

        bsl::ut_scenario{"deallocate assigned failure results in zombie"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vp_pool_t_failure vp_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_then{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                        bsl::ut_check(!vm.deallocate(tls, ext_pool, vp_pool));
                        bsl::ut_check(vm.is_zombie());
                        bsl::ut_check(tls.state_reversal_required);
                    };
                };
            };
        };

        bsl::ut_scenario{"deallocate still active failure results in zombie"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vp_pool_t_success vp_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_required_step(vm.set_active(tls));
                    bsl::ut_then{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                        bsl::ut_check(!vm.deallocate(tls, ext_pool, vp_pool));
                        bsl::ut_check(vm.is_zombie());
                        bsl::ut_check(tls.state_reversal_required);
                    };
                };
            };
        };

        bsl::ut_scenario{"deallocate with extension failure results in zombie"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_signal_vm_destroyed_failure ext_pool{};
                vp_pool_t_success vp_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_then{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                        bsl::ut_check(!vm.deallocate(tls, ext_pool, vp_pool));
                        bsl::ut_check(vm.is_zombie());
                        bsl::ut_check(tls.state_reversal_required);
                    };
                };
            };
        };

        bsl::ut_scenario{"deallocate success"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vp_pool_t_success vp_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_then{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                        bsl::ut_check(vm.deallocate(tls, ext_pool, vp_pool));
                        bsl::ut_check(tls.state_reversal_required);
                    };
                };
            };
        };

        bsl::ut_scenario{"zombify without initialize success"} = []() {
            bsl::ut_given{} = []() {
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&vm]() {
                    vm.zombify();
                    bsl::ut_then{} = [&vm]() {
                        bsl::ut_check(!vm.is_zombie());
                    };
                };
            };
        };

        bsl::ut_scenario{"zombify success"} = []() {
            bsl::ut_given{} = []() {
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    vm.zombify();
                    bsl::ut_then{} = [&vm]() {
                        bsl::ut_check(vm.is_zombie());
                    };
                };
            };
        };

        bsl::ut_scenario{"zombify root vm is ignored"} = []() {
            bsl::ut_given{} = []() {
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&vm]() {
                    bsl::ut_required_step(vm.initialize(VMID0));
                    vm.zombify();
                    bsl::ut_then{} = [&vm]() {
                        bsl::ut_check(!vm.is_zombie());
                    };
                };
            };
        };

        bsl::ut_scenario{"zombify more than once is ignored"} = []() {
            bsl::ut_given{} = []() {
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    vm.zombify();
                    vm.zombify();
                    bsl::ut_then{} = [&vm]() {
                        bsl::ut_check(vm.is_zombie());
                    };
                };
            };
        };

        bsl::ut_scenario{"status without initialize"} = []() {
            bsl::ut_given{} = []() {
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_then{} = [&vm]() {
                    bsl::ut_check(vm.is_deallocated());
                    bsl::ut_check(!vm.is_allocated());
                    bsl::ut_check(!vm.is_zombie());
                };
            };
        };

        bsl::ut_scenario{"status after initialize"} = []() {
            bsl::ut_given{} = []() {
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_then{} = [&vm]() {
                        bsl::ut_check(vm.is_deallocated());
                        bsl::ut_check(!vm.is_allocated());
                        bsl::ut_check(!vm.is_zombie());
                    };
                };
            };
        };

        bsl::ut_scenario{"status after allocation"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_then{} = [&vm]() {
                        bsl::ut_check(!vm.is_deallocated());
                        bsl::ut_check(vm.is_allocated());
                        bsl::ut_check(!vm.is_zombie());
                    };
                };
            };
        };

        bsl::ut_scenario{"status after deallocation"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vp_pool_t_success vp_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vp_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_required_step(vm.deallocate(tls, ext_pool, vp_pool));
                    bsl::ut_then{} = [&vm]() {
                        bsl::ut_check(vm.is_deallocated());
                        bsl::ut_check(!vm.is_allocated());
                        bsl::ut_check(!vm.is_zombie());
                    };
                };
            };
        };

        bsl::ut_scenario{"status after zombify"} = []() {
            bsl::ut_given{} = []() {
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    vm.zombify();
                    bsl::ut_then{} = [&vm]() {
                        bsl::ut_check(!vm.is_deallocated());
                        bsl::ut_check(!vm.is_allocated());
                        bsl::ut_check(vm.is_zombie());
                    };
                };
            };
        };

        bsl::ut_scenario{"set_active without initialize failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_then{} = [&tls, &vm]() {
                    bsl::ut_check(!vm.set_active(tls));
                };
            };
        };

        bsl::ut_scenario{"set_active without allocate failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(!vm.set_active(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_active zombie failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    vm.zombify();
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(!vm.set_active(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_active already active"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_required_step(vm.set_active(tls));
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(!vm.set_active(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_active corrupt already active"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_required_step(vm.set_active(tls));
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(!vm.set_active(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_active another vm is active failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = VMID0.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(!vm.set_active(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_active invalid ppid failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    tls.ppid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(!vm.set_active(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_active success"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(vm.set_active(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_inactive without initialize failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_then{} = [&tls, &vm]() {
                    bsl::ut_check(!vm.set_inactive(tls));
                };
            };
        };

        bsl::ut_scenario{"set_inactive without activate failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(!vm.set_inactive(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_inactive zombie is allowed"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_required_step(vm.set_active(tls));
                    vm.zombify();
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(vm.set_inactive(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_inactive already inactive"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_required_step(vm.set_active(tls));
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(!vm.set_inactive(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_inactive active vm is not this vm"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_required_step(vm.set_active(tls));
                    tls.active_vmid = VMID0.get();
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(!vm.set_inactive(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_inactive invalid ppid failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_required_step(vm.set_active(tls));
                    tls.ppid = syscall::BF_INVALID_ID.get();
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(!vm.set_inactive(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_inactive more than once failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_required_step(vm.set_active(tls));
                    bsl::ut_required_step(vm.set_inactive(tls));
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(!vm.set_inactive(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_inactive corrupt active vm"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_required_step(vm.set_active(tls));
                    bsl::ut_required_step(vm.set_inactive(tls));
                    tls.active_vmid = VMID1.get();
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(!vm.set_inactive(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_inactive success"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_required_step(vm.set_active(tls));
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(vm.set_inactive(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"is_active reports true"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_required_step(vm.set_active(tls));
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(vm.is_active(tls) == tls.ppid);
                    };
                };
            };
        };

        bsl::ut_scenario{"is_active reports false"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(!vm.is_active(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"is_active reports false with corrupt online_pps"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = syscall::BF_INVALID_ID.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(!vm.is_active(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"is_active_on_current_pp reports true"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_required_step(vm.set_active(tls));
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(vm.is_active_on_current_pp(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"is_active_on_current_pp reports false"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(!vm.is_active_on_current_pp(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"is_active_on_current_pp reports false with corrupt online_pps"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = syscall::BF_INVALID_ID.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_then{} = [&tls, &vm]() {
                        bsl::ut_check(!vm.is_active_on_current_pp(tls));
                    };
                };
            };
        };

        bsl::ut_scenario{"dump without initialize"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_then{} = [&tls, &vm]() {
                    vm.dump(tls);
                };
            };
        };

        bsl::ut_scenario{"dump with initialize"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_then{} = [&tls, &vm]() {
                        vm.dump(tls);
                    };
                };
            };
        };

        bsl::ut_scenario{"dump with allocate"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_then{} = [&tls, &vm]() {
                        vm.dump(tls);
                    };
                };
            };
        };

        bsl::ut_scenario{"dump with active"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                ext_pool_t_success ext_pool{};
                vm_t<bsl::to_umax(INTEGRATION_MAX_PPS).get()> vm{};
                bsl::ut_when{} = [&tls, &ext_pool, &vm]() {
                    tls.online_pps = INTEGRATION_MAX_PPS.get();
                    tls.active_vmid = syscall::BF_INVALID_ID.get();
                    bsl::ut_required_step(vm.initialize(VMID1));
                    bsl::ut_required_step(vm.allocate(tls, ext_pool));
                    bsl::ut_required_step(vm.set_active(tls));
                    bsl::ut_then{} = [&tls, &vm]() {
                        vm.dump(tls);
                    };
                };
            };
        };

        return bsl::ut_success();
    }
}

/// <!-- description -->
///   @brief Main function for this unit test. If a call to bsl::ut_check() fails
///     the application will fast fail. If all calls to bsl::ut_check() pass, this
///     function will successfully return with bsl::exit_success.
///
/// <!-- inputs/outputs -->
///   @return Always returns bsl::exit_success.
///
[[nodiscard]] auto
main() noexcept -> bsl::exit_code
{
    bsl::enable_color();

    static_assert(mk::tests() == bsl::ut_success());
    return mk::tests();
}
