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

#include <lock_guard.hpp>
#include <mk_interface.hpp>
#include <spinlock.hpp>

#include <bsl/array.hpp>
#include <bsl/debug.hpp>
#include <bsl/errc_type.hpp>
#include <bsl/finally_assert.hpp>
#include <bsl/unlikely.hpp>
#include <bsl/unlikely_assert.hpp>

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
        /// @brief stores the VP_CONCEPTs in the VP_CONCEPT linked list
        bsl::array<VP_CONCEPT, MAX_VPS> m_pool{};
        /// @brief safe guards operations on the pool.
        mutable spinlock m_lock{};

    public:
        /// @brief an alias for VP_CONCEPT
        using vp_type = VP_CONCEPT;

        /// <!-- description -->
        ///   @brief Initializes this vp_pool_t
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam TLS_CONCEPT defines the type of TLS block to use
        ///   @tparam VPS_POOL_CONCEPT defines the type of VPS pool to use
        ///   @param tls the current TLS block
        ///   @param vps_pool the VPS pool to use
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     and friends otherwise
        ///
        template<typename TLS_CONCEPT, typename VPS_POOL_CONCEPT>
        [[nodiscard]] constexpr auto
        initialize(TLS_CONCEPT &tls, VPS_POOL_CONCEPT &vps_pool) &noexcept -> bsl::errc_type
        {
            bsl::finally_assert release_on_error{[this, &tls, &vps_pool]() noexcept -> void {
                auto const ret{this->release(tls, vps_pool)};
                if (bsl::unlikely(!ret)) {
                    bsl::print<bsl::V>() << bsl::here();
                    return;
                }

                bsl::touch();
            }};

            for (auto const vp : m_pool) {
                auto const ret{vp.data->initialize(bsl::to_u16(vp.index))};
                if (bsl::unlikely_assert(!ret)) {
                    bsl::print<bsl::V>() << bsl::here();
                    return ret;
                }

                bsl::touch();
            }

            release_on_error.ignore();
            return bsl::errc_success;
        }

        /// <!-- description -->
        ///   @brief Release the vp_pool_t. Note that if this function fails,
        ///     the microkernel is left in a corrupt state and all use of the
        ///     vp_pool_t after calling this function will results in UB.
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam TLS_CONCEPT defines the type of TLS block to use
        ///   @tparam VPS_POOL_CONCEPT defines the type of VPS pool to use
        ///   @param tls the current TLS block
        ///   @param vps_pool the VPS pool to use
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     and friends otherwise
        ///
        template<typename TLS_CONCEPT, typename VPS_POOL_CONCEPT>
        [[nodiscard]] constexpr auto
        release(TLS_CONCEPT &tls, VPS_POOL_CONCEPT &vps_pool) &noexcept -> bsl::errc_type
        {
            for (auto const vp : m_pool) {
                auto const ret{vp.data->release(tls, vps_pool)};
                if (bsl::unlikely(!ret)) {
                    bsl::print<bsl::V>() << bsl::here();
                    return ret;
                }

                bsl::touch();
            }

            return bsl::errc_success;
        }

        /// <!-- description -->
        ///   @brief Allocates a vp from the vp pool.
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam TLS_CONCEPT defines the type of TLS block to use
        ///   @tparam VM_POOL_CONCEPT defines the type of VM pool to use
        ///   @param tls the current TLS block
        ///   @param vm_pool the VM pool to use
        ///   @param vmid The ID of the VM to assign the newly created VP to
        ///   @param ppid The ID of the PP to assign the newly created VP to
        ///   @return Returns ID of the newly allocated vp
        ///
        template<typename TLS_CONCEPT, typename VM_POOL_CONCEPT>
        [[nodiscard]] constexpr auto
        allocate(
            TLS_CONCEPT &tls,
            VM_POOL_CONCEPT &vm_pool,
            bsl::safe_uint16 const &vmid,
            bsl::safe_uint16 const &ppid) &noexcept -> bsl::safe_uint16
        {
            lock_guard lock{tls, m_lock};

            VP_CONCEPT *vp{};
            for (auto const elem : m_pool) {
                if (elem.data->is_deallocated()) {
                    vp = elem.data;
                    break;
                }

                bsl::touch();
            }

            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "vp pool out of vps\n" << bsl::here();
                return bsl::safe_uint16::zero(true);
            }

            return vp->allocate(tls, vm_pool, vmid, ppid);
        }

        /// <!-- description -->
        ///   @brief Returns a vp previously allocated using the allocate
        ///     function to the vp pool.
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam TLS_CONCEPT defines the type of TLS block to use
        ///   @tparam VPS_POOL_CONCEPT defines the type of VPS pool to use
        ///   @param tls the current TLS block
        ///   @param vps_pool the VPS pool to use
        ///   @param vpid the ID of the vp to deallocate
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     and friends otherwise
        ///
        template<typename TLS_CONCEPT, typename VPS_POOL_CONCEPT>
        [[nodiscard]] constexpr auto
        deallocate(
            TLS_CONCEPT &tls, VPS_POOL_CONCEPT &vps_pool, bsl::safe_uint16 const &vpid) &noexcept
            -> bsl::errc_type
        {
            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "vpid "                                                   // --
                             << bsl::hex(vpid)                                            // --
                             << " is invalid or greater than or equal to the MAX_VPS "    // --
                             << bsl::hex(bsl::to_u16(MAX_VPS))                            // --
                             << bsl::endl                                                 // --
                             << bsl::here();                                              // --

                return bsl::errc_index_out_of_bounds;
            }

            return vp->deallocate(tls, vps_pool);
        }

        /// <!-- description -->
        ///   @brief Sets the requested vp_t's status as zombified, meaning
        ///     it is no longer usable.
        ///
        /// <!-- inputs/outputs -->
        ///   @param vpid the ID of the vp_t to set as a zombie
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     and friends otherwise
        ///
        [[nodiscard]] constexpr auto
        zombify(bsl::safe_uint16 const &vpid) &noexcept -> bsl::errc_type
        {
            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "vpid "                                                   // --
                             << bsl::hex(vpid)                                            // --
                             << " is invalid or greater than or equal to the MAX_VPS "    // --
                             << bsl::hex(bsl::to_u16(MAX_VPS))                            // --
                             << bsl::endl                                                 // --
                             << bsl::here();                                              // --

                return bsl::errc_index_out_of_bounds;
            }

            vp->zombify();
            return bsl::errc_success;
        }

        /// <!-- description -->
        ///   @brief Returns true if the requested vp_t is deallocated, false
        ///     if the provided VPID is invalid, or if the vp_t is not
        ///     deallocated.
        ///
        /// <!-- inputs/outputs -->
        ///   @param vpid the ID of the vp_t to query
        ///   @return Returns true if the requested vp_t is deallocated, false
        ///     if the provided VPID is invalid, or if the vp_t is not
        ///     deallocated.
        ///
        [[nodiscard]] constexpr auto
        is_deallocated(bsl::safe_uint16 const &vpid) const &noexcept -> bool
        {
            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "vpid "                                                   // --
                             << bsl::hex(vpid)                                            // --
                             << " is invalid or greater than or equal to the MAX_VPS "    // --
                             << bsl::hex(bsl::to_u16(MAX_VPS))                            // --
                             << bsl::endl                                                 // --
                             << bsl::here();                                              // --

                return false;
            }

            return vp->is_deallocated();
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
        is_allocated(bsl::safe_uint16 const &vpid) const &noexcept -> bool
        {
            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "vpid "                                                   // --
                             << bsl::hex(vpid)                                            // --
                             << " is invalid or greater than or equal to the MAX_VPS "    // --
                             << bsl::hex(bsl::to_u16(MAX_VPS))                            // --
                             << bsl::endl                                                 // --
                             << bsl::here();                                              // --

                return false;
            }

            return vp->is_allocated();
        }

        /// <!-- description -->
        ///   @brief Returns true if the requested vp_t is a zombie, false
        ///     if the provided VPID is invalid, or if the vp_t is not
        ///     a zombie.
        ///
        /// <!-- inputs/outputs -->
        ///   @param vpid the ID of the vp_t to query
        ///   @return Returns true if the requested vp_t is a zombie, false
        ///     if the provided VPID is invalid, or if the vp_t is not
        ///     a zombie.
        ///
        [[nodiscard]] constexpr auto
        is_zombie(bsl::safe_uint16 const &vpid) const &noexcept -> bool
        {
            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "vpid "                                                   // --
                             << bsl::hex(vpid)                                            // --
                             << " is invalid or greater than or equal to the MAX_VPS "    // --
                             << bsl::hex(bsl::to_u16(MAX_VPS))                            // --
                             << bsl::endl                                                 // --
                             << bsl::here();                                              // --

                return false;
            }

            return vp->is_zombie();
        }

        /// <!-- description -->
        ///   @brief If a vp_t in the pool is assigned to the requested VM,
        ///     the ID of the first vp_t found is returned. Otherwise, this
        ///     function will return bsl::safe_uint16::zero(true)
        ///
        /// <!-- inputs/outputs -->
        ///   @param vmid the ID fo the VM to query
        ///   @return If a vp_t in the pool is assigned to the requested VM,
        ///     the ID of the first vp_t found is returned. Otherwise, this
        ///     function will return bsl::safe_uint16::zero(true)
        ///
        [[nodiscard]] constexpr auto
        is_assigned_to_vm(bsl::safe_uint16 const &vmid) const &noexcept -> bsl::safe_uint16
        {
            if (bsl::unlikely_assert(!vmid)) {
                bsl::error() << "invalid vmid\n" << bsl::here();
                return bsl::safe_uint16::zero(true);
            }

            if (bsl::unlikely_assert(syscall::BF_INVALID_ID == vmid)) {
                bsl::error() << "invalid vmid\n" << bsl::here();
                return bsl::safe_uint16::zero(true);
            }

            for (auto const elem : m_pool) {
                if (elem.data->assigned_vm() == vmid) {
                    return elem.data->id();
                }

                bsl::touch();
            }

            return bsl::safe_uint16::zero(true);
        }

        /// <!-- description -->
        ///   @brief Sets the requested vp_t as active.
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam TLS_CONCEPT defines the type of TLS block to use
        ///   @param tls the current TLS block
        ///   @param vpid the ID of the vp_t to set as active
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     and friends otherwise
        ///
        template<typename TLS_CONCEPT>
        [[nodiscard]] constexpr auto
        set_active(TLS_CONCEPT &tls, bsl::safe_uint16 const &vpid) &noexcept -> bsl::errc_type
        {
            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "vpid "                                                   // --
                             << bsl::hex(vpid)                                            // --
                             << " is invalid or greater than or equal to the MAX_VPS "    // --
                             << bsl::hex(bsl::to_u16(MAX_VPS))                            // --
                             << bsl::endl                                                 // --
                             << bsl::here();                                              // --

                return bsl::errc_index_out_of_bounds;
            }

            return vp->set_active(tls);
        }

        /// <!-- description -->
        ///   @brief Sets the requested vp_t as inactive.
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam TLS_CONCEPT defines the type of TLS block to use
        ///   @param tls the current TLS block
        ///   @param vpid the ID of the vp_t to set as inactive
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     and friends otherwise
        ///
        template<typename TLS_CONCEPT>
        [[nodiscard]] constexpr auto
        set_inactive(TLS_CONCEPT &tls, bsl::safe_uint16 const &vpid) &noexcept -> bsl::errc_type
        {
            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "vpid "                                                   // --
                             << bsl::hex(vpid)                                            // --
                             << " is invalid or greater than or equal to the MAX_VPS "    // --
                             << bsl::hex(bsl::to_u16(MAX_VPS))                            // --
                             << bsl::endl                                                 // --
                             << bsl::here();                                              // --

                return bsl::errc_index_out_of_bounds;
            }

            return vp->set_inactive(tls);
        }

        /// <!-- description -->
        ///   @brief Returns true if the requested vp_t is active on the
        ///     current PP, false if the provided ID is invalid, or if the
        ///     vp_t is not active on the current PP.
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam TLS_CONCEPT defines the type of TLS block to use
        ///   @param tls the current TLS block
        ///   @param vpid the ID of the vp_t to query
        ///   @return Returns true if the requested vp_t is active on the
        ///     current PP, false if the provided ID is invalid, or if the
        ///     vp_t is not active on the current PP.
        ///
        template<typename TLS_CONCEPT>
        [[nodiscard]] constexpr auto
        is_active(TLS_CONCEPT &tls, bsl::safe_uint16 const &vpid) const &noexcept
            -> bsl::safe_uint16
        {
            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "vpid "                                                   // --
                             << bsl::hex(vpid)                                            // --
                             << " is invalid or greater than or equal to the MAX_VPS "    // --
                             << bsl::hex(bsl::to_u16(MAX_VPS))                            // --
                             << bsl::endl                                                 // --
                             << bsl::here();                                              // --

                return bsl::safe_uint16::zero(true);
            }

            return vp->is_active(tls);
        }

        /// <!-- description -->
        ///   @brief Returns true if this vp_t is active on the current PP,
        ///     false if the provided ID is invalid, or if the vp_t is not
        ///     active on the current PP.
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam TLS_CONCEPT defines the type of TLS block to use
        ///   @param tls the current TLS block
        ///   @param vpid the ID of the vp_t to query
        ///   @return Returns true if this vp_t is active on the current PP,
        ///     false if the provided ID is invalid, or if the vp_t is not
        ///     active on the current PP.
        ///
        template<typename TLS_CONCEPT>
        [[nodiscard]] constexpr auto
        is_active_on_current_pp(TLS_CONCEPT &tls, bsl::safe_uint16 const &vpid) const &noexcept
            -> bool
        {
            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "vpid "                                                   // --
                             << bsl::hex(vpid)                                            // --
                             << " is invalid or greater than or equal to the MAX_VPS "    // --
                             << bsl::hex(bsl::to_u16(MAX_VPS))                            // --
                             << bsl::endl                                                 // --
                             << bsl::here();                                              // --

                return false;
            }

            return vp->is_active_on_current_pp(tls);
        }

        /// <!-- description -->
        ///   @brief Migrates the requested vp_t from one PP to another.
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam TLS_CONCEPT defines the type of TLS block to use
        ///   @param tls the current TLS block
        ///   @param ppid the ID of the PP to migrate to
        ///   @param vpid the ID of the vp_t to migrate
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     and friends otherwise
        ///
        template<typename TLS_CONCEPT>
        [[nodiscard]] constexpr auto
        migrate(
            TLS_CONCEPT &tls, bsl::safe_uint16 const &ppid, bsl::safe_uint16 const &vpid) &noexcept
            -> bsl::errc_type
        {
            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "vpid "                                                   // --
                             << bsl::hex(vpid)                                            // --
                             << " is invalid or greater than or equal to the MAX_VPS "    // --
                             << bsl::hex(bsl::to_u16(MAX_VPS))                            // --
                             << bsl::endl                                                 // --
                             << bsl::here();                                              // --

                return bsl::errc_index_out_of_bounds;
            }

            return vp->migrate(tls, ppid);
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
            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "vpid "                                                   // --
                             << bsl::hex(vpid)                                            // --
                             << " is invalid or greater than or equal to the MAX_VPS "    // --
                             << bsl::hex(bsl::to_u16(MAX_VPS))                            // --
                             << bsl::endl                                                 // --
                             << bsl::here();                                              // --

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
            auto *const vp{m_pool.at_if(bsl::to_umax(vpid))};
            if (bsl::unlikely(nullptr == vp)) {
                bsl::error() << "vpid "                                                   // --
                             << bsl::hex(vpid)                                            // --
                             << " is invalid or greater than or equal to the MAX_VPS "    // --
                             << bsl::hex(bsl::to_u16(MAX_VPS))                            // --
                             << bsl::endl                                                 // --
                             << bsl::here();                                              // --

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
                bsl::error() << "vpid "                                                   // --
                             << bsl::hex(vpid)                                            // --
                             << " is invalid or greater than or equal to the MAX_VPS "    // --
                             << bsl::hex(bsl::to_u16(MAX_VPS))                            // --
                             << bsl::endl                                                 // --
                             << bsl::here();                                              // --

                return;
            }

            vp->dump(tls);
        }
    };
}

#endif
