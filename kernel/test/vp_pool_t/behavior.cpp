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

#include "../../src/vp_pool_t.hpp"
#include "../vp_t_allocate_failure.hpp"
#include "../vp_t_deallocate_failure.hpp"
#include "../vp_t_initialize_and_release_failure.hpp"
#include "../vp_t_initialize_failure.hpp"
#include "../vp_t_migrate_failure.hpp"
#include "../vp_t_release_failure.hpp"
#include "../vp_t_set_active_failure.hpp"
#include "../vp_t_set_inactive_failure.hpp"
#include "../vp_t_success.hpp"

#include <tls_t.hpp>

#include <bsl/ut.hpp>

namespace mk
{
    /// @brief defines the max number of VMs used in testing
    constexpr bsl::safe_uintmax INTEGRATION_MAX_VPS{bsl::to_umax(3)};

    /// @brief defines VMID0
    constexpr bsl::safe_uint16 VMID0{bsl::to_u16(0)};
    /// @brief defines VMID1
    constexpr bsl::safe_uint16 VMID1{bsl::to_u16(1)};

    /// @brief defines PPID0
    constexpr bsl::safe_uint16 PPID0{bsl::to_u16(0)};

    /// @brief defines VPID0
    constexpr bsl::safe_uint16 VPID0{bsl::to_u16(0)};
    /// @brief defines VPID1
    constexpr bsl::safe_uint16 VPID1{bsl::to_u16(1)};
    /// @brief defines VPID2
    constexpr bsl::safe_uint16 VPID2{bsl::to_u16(2)};

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
        bsl::ut_scenario{"initialize vp_t reports success"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_then{} = [&pool]() {
                    bsl::ut_check(pool.initialize(bsl::dontcare, bsl::dontcare));
                };
            };
        };

        bsl::ut_scenario{"initialize vp_t reports failure"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_initialize_failure, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_then{} = [&pool]() {
                    bsl::ut_check(!pool.initialize(bsl::dontcare, bsl::dontcare));
                };
            };
        };

        bsl::ut_scenario{"initialize vp_t and release report failure"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_initialize_and_release_failure, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_then{} = [&pool]() {
                    bsl::ut_check(!pool.initialize(bsl::dontcare, bsl::dontcare));
                };
            };
        };

        bsl::ut_scenario{"release without initialize"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_then{} = [&pool]() {
                    bsl::ut_check(pool.release(bsl::dontcare, bsl::dontcare));
                };
            };
        };

        bsl::ut_scenario{"release with initialize"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(pool.release(bsl::dontcare, bsl::dontcare));
                    };
                };
            };
        };

        bsl::ut_scenario{"release with initialize and vp_t reports failure"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_release_failure, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.release(bsl::dontcare, bsl::dontcare));
                    };
                };
            };
        };

        bsl::ut_scenario{"allocate all vps"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&tls, &pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    bsl::ut_then{} = [&tls, &pool]() {
                        bsl::ut_check(pool.allocate(tls, bsl::dontcare, VMID0, PPID0) == VPID0);
                        bsl::ut_check(pool.allocate(tls, bsl::dontcare, VMID0, PPID0) == VPID1);
                        bsl::ut_check(pool.allocate(tls, bsl::dontcare, VMID0, PPID0) == VPID2);
                        bsl::ut_check(!pool.allocate(tls, bsl::dontcare, VMID0, PPID0));
                    };
                };
            };
        };

        bsl::ut_scenario{"allocate vp_t reports failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                vp_pool_t<vp_t_allocate_failure, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&tls, &pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    bsl::ut_then{} = [&tls, &pool]() {
                        bsl::ut_check(!pool.allocate(tls, bsl::dontcare, VMID0, PPID0));
                    };
                };
            };
        };

        bsl::ut_scenario{"deallocate invalid id"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&tls, &pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    bsl::ut_then{} = [&tls, &pool]() {
                        bsl::ut_check(!pool.deallocate(tls, bsl::dontcare, syscall::BF_INVALID_ID));
                    };
                };
            };
        };

        bsl::ut_scenario{"deallocate vp_t reports failure"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                vp_pool_t<vp_t_deallocate_failure, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&tls, &pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    auto const vpid{pool.allocate(tls, bsl::dontcare, VMID0, PPID0)};
                    bsl::ut_then{} = [&tls, &pool, &vpid]() {
                        bsl::ut_check(!pool.deallocate(tls, bsl::dontcare, vpid));
                    };
                };
            };
        };

        bsl::ut_scenario{"deallocate success"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&tls, &pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    auto const vpid{pool.allocate(tls, bsl::dontcare, VMID0, PPID0)};
                    bsl::ut_then{} = [&tls, &pool, &vpid]() {
                        bsl::ut_check(pool.deallocate(tls, bsl::dontcare, vpid));
                    };
                };
            };
        };

        bsl::ut_scenario{"zombify invalid id"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_then{} = [&pool]() {
                    bsl::ut_check(!pool.zombify(syscall::BF_INVALID_ID));
                };
            };
        };

        bsl::ut_scenario{"zombify success"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_then{} = [&pool]() {
                    bsl::ut_check(pool.zombify(VPID1));
                };
            };
        };

        bsl::ut_scenario{"status invalid id"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.is_allocated(syscall::BF_INVALID_ID));
                        bsl::ut_check(!pool.is_deallocated(syscall::BF_INVALID_ID));
                        bsl::ut_check(!pool.is_zombie(syscall::BF_INVALID_ID));
                    };
                };
            };
        };

        bsl::ut_scenario{"status after initialize"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.is_allocated(VPID0));
                        bsl::ut_check(pool.is_deallocated(VPID0));
                        bsl::ut_check(!pool.is_zombie(VPID0));
                    };
                };
            };
        };

        bsl::ut_scenario{"status after allocate"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&tls, &pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    auto const vpid{pool.allocate(tls, bsl::dontcare, VMID0, PPID0)};
                    bsl::ut_then{} = [&pool, &vpid]() {
                        bsl::ut_check(pool.is_allocated(vpid));
                        bsl::ut_check(!pool.is_deallocated(vpid));
                        bsl::ut_check(!pool.is_zombie(vpid));
                    };
                };
            };
        };

        bsl::ut_scenario{"status after deallocate"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&tls, &pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    auto const vpid{pool.allocate(tls, bsl::dontcare, VMID0, PPID0)};
                    bsl::ut_required_step(pool.deallocate(bsl::dontcare, bsl::dontcare, vpid));
                    bsl::ut_then{} = [&pool, &vpid]() {
                        bsl::ut_check(!pool.is_allocated(vpid));
                        bsl::ut_check(pool.is_deallocated(vpid));
                        bsl::ut_check(!pool.is_zombie(vpid));
                    };
                };
            };
        };

        bsl::ut_scenario{"status after zombify"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    bsl::ut_required_step(pool.zombify(VPID1));
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.is_allocated(VPID1));
                        bsl::ut_check(!pool.is_deallocated(VPID1));
                        bsl::ut_check(pool.is_zombie(VPID1));
                    };
                };
            };
        };

        bsl::ut_scenario{"is_assigned_to_ vm invalid id"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.is_assigned_to_vm(syscall::BF_INVALID_ID));
                    };
                };
            };
        };

        bsl::ut_scenario{"is_assigned_to_vm id with error code"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.is_assigned_to_vm(bsl::safe_uint16::zero(true)));
                    };
                };
            };
        };

        bsl::ut_scenario{"is_assigned_to_vm without initialize"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.is_assigned_to_vm(VMID0));
                    };
                };
            };
        };

        bsl::ut_scenario{"is_assigned_to_vm nothing assigned"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.is_assigned_to_vm(VMID0));
                    };
                };
            };
        };

        bsl::ut_scenario{"is_assigned_to_vm assigned"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&tls, &pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    bsl::ut_required_step(pool.allocate(tls, bsl::dontcare, VMID0, PPID0));
                    bsl::ut_required_step(pool.allocate(tls, bsl::dontcare, VMID0, PPID0));
                    bsl::ut_required_step(pool.deallocate(tls, bsl::dontcare, VPID0));
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(pool.is_assigned_to_vm(VMID0) == VPID1);
                    };
                };
            };
        };

        bsl::ut_scenario{"is_assigned_to_vm assigned but wrong query"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&tls, &pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    bsl::ut_required_step(pool.allocate(tls, bsl::dontcare, VMID0, PPID0));
                    bsl::ut_required_step(pool.allocate(tls, bsl::dontcare, VMID0, PPID0));
                    bsl::ut_required_step(pool.deallocate(tls, bsl::dontcare, VPID0));
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.is_assigned_to_vm(VMID1));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_active invalid id"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.set_active(bsl::dontcare, syscall::BF_INVALID_ID));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_active vp_t reports failure"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_set_active_failure, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.set_active(bsl::dontcare, VPID0));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_active success"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(pool.set_active(bsl::dontcare, VPID0));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_inactive invalid id"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.set_inactive(bsl::dontcare, syscall::BF_INVALID_ID));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_inactive vp_t reports failure"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_set_inactive_failure, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.set_inactive(bsl::dontcare, VPID0));
                    };
                };
            };
        };

        bsl::ut_scenario{"set_inactive success"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(pool.set_inactive(bsl::dontcare, VPID0));
                    };
                };
            };
        };

        bsl::ut_scenario{"is_active invalid id"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.is_active(bsl::dontcare, syscall::BF_INVALID_ID));
                    };
                };
            };
        };

        bsl::ut_scenario{"is_active success"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.is_active(bsl::dontcare, VPID0));
                        bsl::ut_check(pool.set_active(bsl::dontcare, VPID0));
                        bsl::ut_check(pool.is_active(bsl::dontcare, VPID0));
                        bsl::ut_check(pool.set_inactive(bsl::dontcare, VPID0));
                        bsl::ut_check(!pool.is_active(bsl::dontcare, VPID0));
                    };
                };
            };
        };

        bsl::ut_scenario{"is_active_on_current_pp invalid id"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(
                            !pool.is_active_on_current_pp(bsl::dontcare, syscall::BF_INVALID_ID));
                    };
                };
            };
        };

        bsl::ut_scenario{"is_active_on_current_pp success"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.is_active_on_current_pp(bsl::dontcare, VPID0));
                        bsl::ut_check(pool.set_active(bsl::dontcare, VPID0));
                        bsl::ut_check(pool.is_active_on_current_pp(bsl::dontcare, VPID0));
                        bsl::ut_check(pool.set_inactive(bsl::dontcare, VPID0));
                        bsl::ut_check(!pool.is_active_on_current_pp(bsl::dontcare, VPID0));
                    };
                };
            };
        };

        bsl::ut_scenario{"migrate invalid id"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.migrate(bsl::dontcare, PPID0, syscall::BF_INVALID_ID));
                    };
                };
            };
        };

        bsl::ut_scenario{"migrate vp_t reports failure"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_migrate_failure, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.migrate(bsl::dontcare, PPID0, VPID0));
                    };
                };
            };
        };

        bsl::ut_scenario{"migrate success"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(pool.migrate(bsl::dontcare, PPID0, VPID0));
                    };
                };
            };
        };

        bsl::ut_scenario{"assigned_vm invalid id"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.assigned_vm(syscall::BF_INVALID_ID));
                    };
                };
            };
        };

        bsl::ut_scenario{"assigned_vm unassigned"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.assigned_vm(VPID0));
                    };
                };
            };
        };

        bsl::ut_scenario{"assigned_vm success"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&tls, &pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    bsl::ut_required_step(pool.allocate(tls, bsl::dontcare, VMID0, PPID0));
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(pool.assigned_vm(VPID0) == VMID0);
                    };
                };
            };
        };

        bsl::ut_scenario{"assigned_pp invalid id"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.assigned_pp(syscall::BF_INVALID_ID));
                    };
                };
            };
        };

        bsl::ut_scenario{"assigned_pp unassigned"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(!pool.assigned_pp(VPID0));
                    };
                };
            };
        };

        bsl::ut_scenario{"assigned_pp success"} = []() {
            bsl::ut_given{} = []() {
                tls_t tls{};
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&tls, &pool]() {
                    bsl::ut_required_step(pool.initialize(bsl::dontcare, bsl::dontcare));
                    bsl::ut_required_step(pool.allocate(tls, bsl::dontcare, VMID0, PPID0));
                    bsl::ut_then{} = [&pool]() {
                        bsl::ut_check(pool.assigned_pp(VPID0) == PPID0);
                    };
                };
            };
        };

        bsl::ut_scenario{"dump invalid id"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        pool.dump(bsl::dontcare, syscall::BF_INVALID_ID);
                    };
                };
            };
        };

        bsl::ut_scenario{"dump success"} = []() {
            bsl::ut_given{} = []() {
                vp_pool_t<vp_t_success, INTEGRATION_MAX_VPS.get()> pool{};
                bsl::ut_when{} = [&pool]() {
                    bsl::ut_then{} = [&pool]() {
                        pool.dump(bsl::dontcare, VPID0);
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
