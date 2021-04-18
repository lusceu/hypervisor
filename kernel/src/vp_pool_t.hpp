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

#ifndef VP_POOL_T_HPP
#define VP_POOL_T_HPP

#include <bsl/array.hpp>
#include <bsl/debug.hpp>
#include <bsl/errc_type.hpp>
#include <bsl/finally.hpp>
#include <bsl/lock_guard.hpp>
#include <bsl/spinlock.hpp>
#include <bsl/unlikely.hpp>

namespace mk
{
    /// @class mk::vp_pool_t
    ///
    /// <!-- description -->
    ///   @brief Defines the microkernel's VP pool
    ///
    /// <!-- template parameters -->
    ///   @tparam VP_CONCEPT the type of vp_t that this class manages.
    ///   @tparam MAX_VPS the max number of VPs supported
    ///
    template<typename VP_CONCEPT, bsl::uintmax MAX_VPS>
    class vp_pool_t final
    {
        /// @brief stores true if initialized() has been executed
        bool m_initialized;
        /// @brief stores the first VP_CONCEPT in the VP_CONCEPT linked list
        VP_CONCEPT *m_head;
        /// @brief stores the VP_CONCEPTs in the VP_CONCEPT linked list
        bsl::array<VP_CONCEPT, MAX_VPS> m_pool;
        /// @brief safe guards operations on the pool.
        mutable bsl::spinlock m_pool_lock;

    public:
        /// @brief an alias for VP_CONCEPT
        using vp_type = VP_CONCEPT;

        /// <!-- description -->
        ///   @brief Creates a vp_pool_t
        ///
        explicit constexpr vp_pool_t() noexcept    // --
            : m_initialized{}, m_head{}, m_pool{}, m_pool_lock{}
        {}

        /// <!-- description -->
        ///   @brief Initializes this vp_pool_t
        ///
        /// <!-- inputs/outputs -->
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     otherwise
        ///
        [[nodiscard]] constexpr auto
        initialize() &noexcept -> bsl::errc_type
        {
            bsl::errc_type ret{};

            if (bsl::unlikely(m_initialized)) {
                bsl::error() << "vp_pool_t already initialized\n" << bsl::here();
                return bsl::errc_failure;
            }

            bsl::finally release_on_error{[this]() noexcept -> void {
                this->release();
            }};

            VP_CONCEPT *prev{};
            for (auto const vp : m_pool) {
                ret = vp.data->initialize(bsl::to_u16(vp.index));
                if (bsl::unlikely(!ret)) {
                    bsl::print<bsl::V>() << bsl::here();
                    return bsl::errc_failure;
                }

                if (nullptr != prev) {
                    prev->set_next(vp.data);
                }
                else {
                    m_head = vp.data;
                }

                prev = vp.data;
            }

            release_on_error.ignore();
            m_initialized = true;

            return bsl::errc_success;
        }

        /// <!-- description -->
        ///   @brief Release the vp_t
        ///
        constexpr void
        release() &noexcept
        {
            for (auto const vp : m_pool) {
                vp.data->release();
            }

            m_head = {};
            m_initialized = {};
        }

        /// <!-- description -->
        ///   @brief Destroyes a previously created vp_pool_t
        ///
        constexpr ~vp_pool_t() noexcept = default;

        /// <!-- description -->
        ///   @brief copy constructor
        ///
        /// <!-- inputs/outputs -->
        ///   @param o the object being copied
        ///
        constexpr vp_pool_t(vp_pool_t const &o) noexcept = delete;

        /// <!-- description -->
        ///   @brief move constructor
        ///
        /// <!-- inputs/outputs -->
        ///   @param o the object being moved
        ///
        constexpr vp_pool_t(vp_pool_t &&o) noexcept = default;

        /// <!-- description -->
        ///   @brief copy assignment
        ///
        /// <!-- inputs/outputs -->
        ///   @param o the object being copied
        ///   @return a reference to *this
        ///
        [[maybe_unused]] constexpr auto operator=(vp_pool_t const &o) &noexcept
            -> vp_pool_t & = delete;

        /// <!-- description -->
        ///   @brief move assignment
        ///
        /// <!-- inputs/outputs -->
        ///   @param o the object being moved
        ///   @return a reference to *this
        ///
        [[maybe_unused]] constexpr auto operator=(vp_pool_t &&o) &noexcept -> vp_pool_t & = default;

        /// <!-- description -->
        ///   @brief Allocates a vp from the vp pool. We set the allocated
        ///     vp_t's next() to itself, which indicates that it has been
        ///     allocated.
        ///
        /// <!-- inputs/outputs -->
        ///   @return Returns ID of the newly allocated vp
        ///
        [[nodiscard]] constexpr auto
        allocate() &noexcept -> bsl::safe_uint16
        {
            bsl::lock_guard lock{m_pool_lock};

            if (bsl::unlikely(!m_initialized)) {
                bsl::error() << "vp_pool_t not initialized\n" << bsl::here();
                return bsl::safe_uint16::zero(true);
            }

            if (bsl::unlikely(nullptr == m_head)) {
                bsl::error() << "vp pool out of vps\n" << bsl::here();
                return bsl::safe_uint16::zero(true);
            }

            if (bsl::unlikely(!m_head->allocate())) {
                bsl::print<bsl::V>() << bsl::here();
                return bsl::safe_uint16::zero(true);
            }

            auto *const vp{m_head};
            m_head = m_head->next();

            vp->set_next(vp);
            return vp->id();
        }

        /// <!-- description -->
        ///   @brief Returns a vp previously allocated using the allocate
        ///     function to the vp pool.
        ///
        /// <!-- inputs/outputs -->
        ///   @param vpid the ID of the vp to deallocate
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     otherwise
        ///
        [[nodiscard]] constexpr auto
        deallocate(bsl::safe_uint16 const &vpid) &noexcept -> bsl::errc_type
        {
            bsl::lock_guard lock{m_pool_lock};

            if (bsl::unlikely(!m_initialized)) {
                bsl::error() << "vp_pool_t not initialized\n" << bsl::here();
                return bsl::errc_failure;
            }

            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "invalid vpid: "    // --
                             << bsl::hex(vpid)      // --
                             << bsl::endl           // --
                             << bsl::here();        // --

                return bsl::errc_failure;
            }

            if (!vp->is_allocated()) {
                bsl::error() << "vp with id "             // --
                             << bsl::hex(vpid)            // --
                             << " was never allocated"    // --
                             << bsl::endl                 // --
                             << bsl::here();              // --

                return bsl::errc_failure;
            }

            vp->deallocate();
            vp->set_next(m_head);
            m_head = vp;

            return bsl::errc_success;
        }

        /// <!-- description -->
        ///   @brief Returns true if the requested vp_t is allocated, false
        ///     if the provided VPID is invalid, or if the vp_t is not
        ///     allocated.
        ///
        /// <!-- inputs/outputs -->
        ///   @param vpid the ID of the vp_t to query
        ///   @return Returns true if the requested vp_t is allocated, false
        ///     if the provided VPID is invalid, or if the vp_t is not
        ///     allocated.
        ///
        [[nodiscard]] constexpr auto
        is_allocated(bsl::safe_uint16 const &vpid) &noexcept -> bool
        {
            if (bsl::unlikely(!m_initialized)) {
                bsl::error() << "vp_pool_t not initialized\n" << bsl::here();
                return false;
            }

            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "invalid vpid: "    // --
                             << bsl::hex(vpid)      // --
                             << bsl::endl           // --
                             << bsl::here();        // --

                return false;
            }

            return vp->is_allocated();
        }

        /// <!-- description -->
        ///   @brief Assigns the requested vp_t to a VM
        ///
        /// <!-- inputs/outputs -->
        ///   @param vpid the ID of the vp_t to assign to
        ///   @param vmid the VM the requested vp_t is assigned to
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     otherwise
        ///
        [[nodiscard]] constexpr auto
        assign_vm(bsl::safe_uint16 const &vpid, bsl::safe_uint16 const &vmid) &noexcept
            -> bsl::errc_type
        {
            if (bsl::unlikely(!m_initialized)) {
                bsl::error() << "vp_pool_t not initialized\n" << bsl::here();
                return bsl::errc_failure;
            }

            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "invalid vpid: "    // --
                             << bsl::hex(vpid)      // --
                             << bsl::endl           // --
                             << bsl::here();        // --

                return bsl::errc_failure;
            }

            return vp->assign_vm(vmid);
        }

        /// <!-- description -->
        ///   @brief Assigns the requested vp_t to a PP
        ///
        /// <!-- inputs/outputs -->
        ///   @param vpid the ID of the vp_t to assign to
        ///   @param ppid the PP the requested vp_t is assigned to
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     otherwise
        ///
        [[nodiscard]] constexpr auto
        assign_pp(bsl::safe_uint16 const &vpid, bsl::safe_uint16 const &ppid) &noexcept
            -> bsl::errc_type
        {
            if (bsl::unlikely(!m_initialized)) {
                bsl::error() << "vp_pool_t not initialized\n" << bsl::here();
                return bsl::errc_failure;
            }

            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "invalid vpid: "    // --
                             << bsl::hex(vpid)      // --
                             << bsl::endl           // --
                             << bsl::here();        // --

                return bsl::errc_failure;
            }

            return vp->assign_pp(ppid);
        }

        /// <!-- description -->
        ///   @brief Returns the ID of the VM the requested vp_t is assigned to
        ///
        /// <!-- inputs/outputs -->
        ///   @param vpid the ID of the vp_t to query
        ///   @return Returns the ID of the VM the requested vp_t is assigned to
        ///
        [[nodiscard]] constexpr auto
        assigned_vm(bsl::safe_uint16 const &vpid) const &noexcept -> bsl::safe_uint16
        {
            if (bsl::unlikely(!m_initialized)) {
                bsl::error() << "vp_pool_t not initialized\n" << bsl::here();
                return bsl::safe_uint16::zero(true);
            }

            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "invalid vpid: "    // --
                             << bsl::hex(vpid)      // --
                             << bsl::endl           // --
                             << bsl::here();        // --

                return bsl::safe_uint16::zero(true);
            }

            return vp->assigned_vm();
        }

        /// <!-- description -->
        ///   @brief Returns the ID of the PP the requested vp_t is assigned to
        ///
        /// <!-- inputs/outputs -->
        ///   @param vpid the ID of the vp_t to query
        ///   @return Returns the ID of the PP the requested vp_t is assigned to
        ///
        [[nodiscard]] constexpr auto
        assigned_pp(bsl::safe_uint16 const &vpid) const &noexcept -> bsl::safe_uint16
        {
            if (bsl::unlikely(!m_initialized)) {
                bsl::error() << "vp_pool_t not initialized\n" << bsl::here();
                return bsl::safe_uint16::zero(true);
            }

            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "invalid vpid: "    // --
                             << bsl::hex(vpid)      // --
                             << bsl::endl           // --
                             << bsl::here();        // --

                return bsl::safe_uint16::zero(true);
            }

            return vp->assigned_pp();
        }

        /// <!-- description -->
        ///   @brief Dumps the requested VP
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam TLS_CONCEPT defines the type of TLS block to use
        ///   @param tls the current TLS block
        ///   @param vpid the ID of the VP to dump
        ///
        template<typename TLS_CONCEPT>
        constexpr void
        dump(TLS_CONCEPT &tls, bsl::safe_uint16 const &vpid) &noexcept
        {
            if constexpr (BSL_DEBUG_LEVEL == bsl::CRITICAL_ONLY) {
                return;
            }

            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "invalid vpid: "    // --
                             << bsl::hex(vpid)      // --
                             << bsl::endl           // --
                             << bsl::here();        // --

                return;
            }

            vp->dump(tls);
        }
    };
}

#endif
