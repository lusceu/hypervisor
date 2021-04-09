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

#ifndef ROOT_PAGE_TABLE_T_HPP
#define ROOT_PAGE_TABLE_T_HPP

#include <allocate_tags.hpp>
#include <map_page_flags.hpp>
#include <pdpt_t.hpp>
#include <pdpte_t.hpp>
#include <pdt_t.hpp>
#include <pdte_t.hpp>
#include <pml4t_t.hpp>
#include <pml4te_t.hpp>
#include <pt_t.hpp>
#include <pte_t.hpp>

#include <bsl/convert.hpp>
#include <bsl/debug.hpp>
#include <bsl/errc_type.hpp>
#include <bsl/finally.hpp>
#include <bsl/fmt.hpp>
#include <bsl/lock_guard.hpp>
#include <bsl/safe_integral.hpp>
#include <bsl/spinlock.hpp>
#include <bsl/touch.hpp>
#include <bsl/unlikely.hpp>

namespace mk
{
    /// @class mk::root_page_table_t
    ///
    /// <!-- description -->
    ///   @brief Implements the root pages tables used by the microkernel
    ///     for mapping extension memory.
    ///
    /// <!-- template parameters -->
    ///   @tparam INTRINSIC_CONCEPT defines the type of intrinsics to use
    ///   @tparam PAGE_POOL_CONCEPT defines the type of page pool to use
    ///   @tparam HUGE_POOL_CONCEPT defines the type of huge pool to use
    ///   @tparam PAGE_SIZE defines the size of a page
    ///   @tparam PAGE_SHIFT defines number of bits in a page
    ///
    template<
        typename INTRINSIC_CONCEPT,
        typename PAGE_POOL_CONCEPT,
        typename HUGE_POOL_CONCEPT,
        bsl::uintmax PAGE_SIZE,
        bsl::uintmax PAGE_SHIFT>
    class root_page_table_t final
    {
        /// @brief stores true if initialized() has been executed
        bool m_initialized{};
        /// @brief stores a reference to the intrinsics to use
        INTRINSIC_CONCEPT *m_intrinsic{};
        /// @brief stores a reference to the page pool to use
        PAGE_POOL_CONCEPT *m_page_pool{};
        /// @brief stores a reference to the huge pool to use
        HUGE_POOL_CONCEPT *m_huge_pool{};
        /// @brief stores a pointer to the pml4t
        pml4t_t *m_pml4t{};
        /// @brief stores the physical address of the pml4t
        bsl::safe_uintmax m_pml4t_phys{bsl::safe_uintmax::zero(true)};
        /// @brief safe guards operations on the RPT.
        mutable bsl::spinlock m_rpt_lock{};

        /// <!-- description -->
        ///   @brief Returns the index of the last entry present in a page
        ///     table.
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam TABLE_CONCEPT the type of page table to search
        ///   @param table the page table to search
        ///   @return Returns the index of the last entry present in a page
        ///     table.
        ///
        template<typename TABLE_CONCEPT>
        [[nodiscard]] constexpr auto
        get_last_index(TABLE_CONCEPT const *const table) const noexcept -> bsl::safe_uintmax
        {
            bsl::safe_uintmax last_index{};
            for (auto const elem : table->entries) {
                if (bsl::ZERO_UMAX == elem.data->p) {
                    continue;
                }

                last_index = elem.index;
            }

            return last_index;
        }

        /// <!-- description -->
        ///   @brief Given an outputter, index and the index of the last
        ///     present entry in the page table being dumped, this function
        ///     will output a decoration and the index.
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam T the type of outputter provided
        ///   @param o the instance of the outputter used to output the value.
        ///   @param index the current index of the entry being dumped
        ///   @param last_index the index of the last present entry in the page
        ///     table being dumped.
        ///
        template<typename T>
        constexpr void
        output_decoration_and_index(
            bsl::out<T> const o,
            bsl::safe_uintmax const &index,
            bsl::safe_uintmax const &last_index) const noexcept
        {
            o << bsl::rst;

            if (index != last_index) {
                o << "├── ";
            }
            else {
                o << "└── ";
            }

            o << "[" << bsl::ylw << bsl::fmt{"#05x", index} << bsl::rst << "] ";
        }

        /// <!-- description -->
        ///   @brief Given an outputter, and whether or not the page table
        ///     entry is the last entry in the table, this function will
        ///     either output whtspace, or a | and shitespace.
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam T the type of outputter provided
        ///   @param o the instance of the outputter used to output the value.
        ///   @param is_last_index true if the entry being outputted is the
        ///     last index in the table.
        ///
        template<typename T>
        constexpr void
        output_spacing(bsl::out<T> const o, bool const is_last_index) const noexcept
        {
            o << bsl::rst;

            if (!is_last_index) {
                o << "│   ";
            }
            else {
                o << "    ";
            }
        }

        /// <!-- description -->
        ///   @brief Given and outputter and a page table entry, this
        ///     function outputs the flags associated with the entry
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam T the type of outputter provided
        ///   @tparam ENTRY_CONCEPT the type of page table entry to output
        ///   @param o the instance of the outputter used to output the value.
        ///   @param entry the page table entry to output
        ///
        template<typename T, typename ENTRY_CONCEPT>
        constexpr void
        output_entry_and_flags(bsl::out<T> const o, ENTRY_CONCEPT const *const entry) const noexcept
        {
            bool add_comma{};

            o << bsl::hex(*static_cast<bsl::uint64 const *>(static_cast<void const *>(entry)));
            o << bsl::rst << " (";

            if (bsl::ZERO_UMAX != entry->rw) {
                o << bsl::grn << "W" << bsl::rst;
                add_comma = true;
            }
            else {
                bsl::touch();
            }

            if (bsl::ZERO_UMAX != entry->us) {
                if (add_comma) {
                    o << ", ";
                }
                else {
                    bsl::touch();
                }

                o << bsl::grn << "U" << bsl::rst;
                add_comma = true;
            }
            else {
                bsl::touch();
            }

            if (bsl::ZERO_UMAX != entry->nx) {
                if (add_comma) {
                    o << ", ";
                }
                else {
                    bsl::touch();
                }

                o << bsl::grn << "NX" << bsl::rst;
                add_comma = true;
            }
            else {
                bsl::touch();
            }

            if constexpr (bsl::is_same<ENTRY_CONCEPT, loader::pml4te_t>::value) {
                if (bsl::ZERO_UMAX != entry->alias) {
                    if (add_comma) {
                        o << ", ";
                    }
                    else {
                        bsl::touch();
                    }

                    o << bsl::grn << "alias" << bsl::rst;
                    add_comma = true;
                }
                else {
                    bsl::touch();
                }
            }

            if constexpr (bsl::is_same<ENTRY_CONCEPT, loader::pte_t>::value) {
                if (add_comma) {
                    o << ", ";
                }
                else {
                    bsl::touch();
                }

                switch (entry->auto_release) {
                    case MAP_PAGE_AUTO_RELEASE_ALLOC_PAGE.get(): {
                        o << bsl::grn << "auto_release_alloc_page" << bsl::rst;
                        break;
                    }

                    case MAP_PAGE_AUTO_RELEASE_ALLOC_HUGE.get(): {
                        o << bsl::grn << "auto_release_alloc_huge" << bsl::rst;
                        break;
                    }

                    case MAP_PAGE_AUTO_RELEASE_ALLOC_HEAP.get(): {
                        o << bsl::grn << "auto_release_alloc_heap" << bsl::rst;
                        break;
                    }

                    case MAP_PAGE_AUTO_RELEASE_STACK.get(): {
                        o << bsl::grn << "auto_release_stack" << bsl::rst;
                        break;
                    }

                    case MAP_PAGE_AUTO_RELEASE_TLS.get(): {
                        o << bsl::grn << "auto_release_tls" << bsl::rst;
                        break;
                    }

                    case MAP_PAGE_AUTO_RELEASE_ELF.get(): {
                        o << bsl::grn << "auto_release_elf" << bsl::rst;
                        break;
                    }

                    default: {
                        o << bsl::grn << "manual" << bsl::rst;
                        break;
                    }
                }

                add_comma = true;
            }

            o << ")" << bsl::endl;
        }

        /// <!-- description -->
        ///   @brief Returns the page-map level-4 (PML4T) offset given a
        ///     virtual address.
        ///
        /// <!-- inputs/outputs -->
        ///   @param virt the virtual address to get the PML4T offset from.
        ///   @return the PML4T offset from the virtual address
        ///
        [[nodiscard]] constexpr auto
        pml4to(bsl::safe_uintmax const &virt) const noexcept -> bsl::safe_uintmax
        {
            constexpr bsl::safe_uintmax mask{bsl::to_umax(0x1FF)};
            constexpr bsl::safe_uintmax shift{bsl::to_umax(39)};
            return (virt >> shift) & mask;
        }

        /// <!-- description -->
        ///   @brief Dumps the provided pml4t_t
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam T the type of outputter provided
        ///   @param o the instance of the outputter used to output the value.
        ///   @param pml4t the pml4t_t to dump
        ///
        template<typename T>
        constexpr void
        dump_pml4t(bsl::out<T> const o, pml4t_t const *const pml4t) const noexcept
        {
            bsl::safe_uintmax const last_index{this->get_last_index(pml4t)};

            o << bsl::blu                  // --
              << bsl::hex(m_pml4t_phys)    // --
              << bsl::endl;                // --

            for (auto const elem : pml4t->entries) {
                if (bsl::ZERO_UMAX == elem.data->p) {
                    continue;
                }

                this->output_decoration_and_index(o, elem.index, last_index);

                if (bsl::ZERO_UMAX != elem.data->us) {
                    o << bsl::blu;
                }
                else {
                    o << bsl::blk;
                }

                this->output_entry_and_flags(o, elem.data);

                if (bsl::ZERO_UMAX != elem.data->us) {
                    this->dump_pdpt(o, this->get_pdpt(elem.data), elem.index == last_index);
                }
                else {
                    bsl::touch();
                }
            }
        }

        /// <!-- description -->
        ///   @brief Adds a pdpt_t to the provided pml4te_t.
        ///
        /// <!-- inputs/outputs -->
        ///   @param pml4te the pml4te_t to add a pdpt_t too
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     otherwise
        ///
        [[nodiscard]] constexpr auto
        add_pdpt(loader::pml4te_t *const pml4te) noexcept -> bsl::errc_type
        {
            auto const *const table{m_page_pool->template allocate<void>(ALLOCATE_TAG_PDPTS)};
            if (bsl::unlikely(nullptr == table)) {
                bsl::print<bsl::V>() << bsl::here();
                return bsl::errc_failure;
            }

            auto const table_phys{m_page_pool->virt_to_phys(table)};
            if (bsl::unlikely(!table_phys)) {
                bsl::print<bsl::V>() << bsl::here();
                return bsl::errc_failure;
            }

            pml4te->phys = (table_phys >> PAGE_SHIFT).get();
            pml4te->p = bsl::ONE_UMAX.get();
            pml4te->rw = bsl::ONE_UMAX.get();
            pml4te->us = bsl::ONE_UMAX.get();

            return bsl::errc_success;
        }

        /// <!-- description -->
        ///   @brief Adds a pdpt_t to the provided pml4te_t.
        ///
        /// <!-- inputs/outputs -->
        ///   @param pml4te the pml4te_t to add a pdpt_t too
        ///
        constexpr void
        remove_pdpt(loader::pml4te_t *const pml4te) noexcept
        {
            for (auto const elem : get_pdpt(pml4te)->entries) {
                if (elem.data->p != bsl::ZERO_UMAX) {
                    this->remove_pdt(elem.data);
                }
                else {
                    bsl::touch();
                }
            }

            m_page_pool->deallocate(get_pdpt(pml4te), ALLOCATE_TAG_PDPTS);
        }

        /// <!-- description -->
        ///   @brief Returns the pdpt_t associated with the provided
        ///     pml4te_t.
        ///
        /// <!-- inputs/outputs -->
        ///   @param pml4te the pml4te_t to get the pdpt_t from
        ///   @return A pointer to the requested pdpt_t
        ///
        [[nodiscard]] constexpr auto
        get_pdpt(loader::pml4te_t *const pml4te) noexcept -> pdpt_t *
        {
            bsl::safe_uintmax entry_phys{pml4te->phys};
            entry_phys <<= PAGE_SHIFT;

            return m_page_pool->template phys_to_virt<pdpt_t>(entry_phys);
        }

        /// <!-- description -->
        ///   @brief Returns the pdpt_t associated with the provided
        ///     pml4te_t.
        ///
        /// <!-- inputs/outputs -->
        ///   @param pml4te the pml4te_t to get the pdpt_t from
        ///   @return A pointer to the requested pdpt_t
        ///
        [[nodiscard]] constexpr auto
        get_pdpt(loader::pml4te_t const *const pml4te) const noexcept -> pdpt_t const *
        {
            bsl::safe_uintmax entry_phys{pml4te->phys};
            entry_phys <<= PAGE_SHIFT;

            return m_page_pool->template phys_to_virt<pdpt_t const>(entry_phys);
        }

        /// <!-- description -->
        ///   @brief Returns the page-directory-pointer table (PDPT) offset
        ///     given a virtual address.
        ///
        /// <!-- inputs/outputs -->
        ///   @param virt the virtual address to get the PDPT offset from.
        ///   @return the PDPT offset from the virtual address
        ///
        [[nodiscard]] constexpr auto
        pdpto(bsl::safe_uintmax const &virt) const noexcept -> bsl::safe_uintmax
        {
            constexpr bsl::safe_uintmax mask{bsl::to_umax(0x1FF)};
            constexpr bsl::safe_uintmax shift{bsl::to_umax(30)};
            return (virt >> shift) & mask;
        }

        /// <!-- description -->
        ///   @brief Dumps the provided pdpt_t
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam T the type of outputter provided
        ///   @param o the instance of the outputter used to output the value.
        ///   @param pdpt the pdpt_t to dump
        ///   @param is_pml4te_last_index true if the parent pml4te was the
        ///     last pml4te in the table
        ///
        template<typename T>
        constexpr void
        dump_pdpt(
            bsl::out<T> const o, pdpt_t const *const pdpt, bool is_pml4te_last_index) const noexcept
        {
            bsl::safe_uintmax const last_index{this->get_last_index(pdpt)};

            for (auto const elem : pdpt->entries) {
                if (bsl::ZERO_UMAX == elem.data->p) {
                    continue;
                }

                this->output_spacing(o, is_pml4te_last_index);
                this->output_decoration_and_index(o, elem.index, last_index);

                o << bsl::blu;
                this->output_entry_and_flags(o, elem.data);

                this->dump_pdt(
                    o, this->get_pdt(elem.data), is_pml4te_last_index, elem.index == last_index);
            }
        }

        /// <!-- description -->
        ///   @brief Adds a pdt_t to the provided pdpte_t.
        ///
        /// <!-- inputs/outputs -->
        ///   @param pdpte the pdpte_t to add a pdt_t too
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     otherwise
        ///
        [[nodiscard]] constexpr auto
        add_pdt(loader::pdpte_t *const pdpte) noexcept -> bsl::errc_type
        {
            auto const *const table{m_page_pool->template allocate<void>(ALLOCATE_TAG_PDTS)};
            if (bsl::unlikely(nullptr == table)) {
                bsl::print<bsl::V>() << bsl::here();
                return bsl::errc_failure;
            }

            auto const table_phys{m_page_pool->virt_to_phys(table)};
            if (bsl::unlikely(!table_phys)) {
                bsl::print<bsl::V>() << bsl::here();
                return bsl::errc_failure;
            }

            pdpte->phys = (table_phys >> PAGE_SHIFT).get();
            pdpte->p = bsl::ONE_UMAX.get();
            pdpte->rw = bsl::ONE_UMAX.get();
            pdpte->us = bsl::ONE_UMAX.get();

            return bsl::errc_success;
        }

        /// <!-- description -->
        ///   @brief Adds a pdt_t to the provided pdpte_t.
        ///
        /// <!-- inputs/outputs -->
        ///   @param pdpte the pdpte_t to add a pdt_t too
        ///
        constexpr void
        remove_pdt(loader::pdpte_t *const pdpte) noexcept
        {
            for (auto const elem : get_pdt(pdpte)->entries) {
                if (elem.data->p != bsl::ZERO_UMAX) {
                    this->remove_pt(elem.data);
                }
                else {
                    bsl::touch();
                }
            }

            m_page_pool->deallocate(get_pdt(pdpte), ALLOCATE_TAG_PDTS);
        }

        /// <!-- description -->
        ///   @brief Returns the pdt_t associated with the provided
        ///     pdpte_t.
        ///
        /// <!-- inputs/outputs -->
        ///   @param pdpte the pdpte_t to get the pdt_t from
        ///   @return A pointer to the requested pdt_t
        ///
        [[nodiscard]] constexpr auto
        get_pdt(loader::pdpte_t *const pdpte) noexcept -> pdt_t *
        {
            bsl::safe_uintmax entry_phys{pdpte->phys};
            entry_phys <<= PAGE_SHIFT;

            return m_page_pool->template phys_to_virt<pdt_t>(entry_phys);
        }

        /// <!-- description -->
        ///   @brief Returns the pdt_t associated with the provided
        ///     pdpte_t.
        ///
        /// <!-- inputs/outputs -->
        ///   @param pdpte the pdpte_t to get the pdt_t from
        ///   @return A pointer to the requested pdt_t
        ///
        [[nodiscard]] constexpr auto
        get_pdt(loader::pdpte_t const *const pdpte) const noexcept -> pdt_t const *
        {
            bsl::safe_uintmax entry_phys{pdpte->phys};
            entry_phys <<= PAGE_SHIFT;

            return m_page_pool->template phys_to_virt<pdt_t const>(entry_phys);
        }

        /// <!-- description -->
        ///   @brief Returns the page-directory table (PDT) offset given a
        ///     virtual address.
        ///
        /// <!-- inputs/outputs -->
        ///   @param virt the virtual address to get the PDT offset from.
        ///   @return the PDT offset from the virtual address
        ///
        [[nodiscard]] constexpr auto
        pdto(bsl::safe_uintmax const &virt) const noexcept -> bsl::safe_uintmax
        {
            constexpr bsl::safe_uintmax mask{bsl::to_umax(0x1FF)};
            constexpr bsl::safe_uintmax shift{bsl::to_umax(21)};
            return (virt >> shift) & mask;
        }

        /// <!-- description -->
        ///   @brief Dumps the provided pdt_t
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam T the type of outputter provided
        ///   @param o the instance of the outputter used to output the value.
        ///   @param pdt the pdt_t to dump
        ///   @param is_pml4te_last_index true if the parent pml4te was the
        ///     last pml4te in the table
        ///   @param is_pdpte_last_index true if the parent pdpte was the
        ///     last pdpte in the table
        ///
        template<typename T>
        constexpr void
        dump_pdt(
            bsl::out<T> const o,
            pdt_t const *const pdt,
            bool is_pml4te_last_index,
            bool is_pdpte_last_index) const noexcept
        {
            bsl::safe_uintmax const last_index{this->get_last_index(pdt)};

            for (auto const elem : pdt->entries) {
                if (bsl::ZERO_UMAX == elem.data->p) {
                    continue;
                }

                this->output_spacing(o, is_pml4te_last_index);
                this->output_spacing(o, is_pdpte_last_index);
                this->output_decoration_and_index(o, elem.index, last_index);

                o << bsl::blu;
                this->output_entry_and_flags(o, elem.data);

                this->dump_pt(
                    o,
                    this->get_pt(elem.data),
                    is_pml4te_last_index,
                    is_pdpte_last_index,
                    elem.index == last_index);
            }
        }

        /// <!-- description -->
        ///   @brief Adds a pt_t to the provided pdte_t.
        ///
        /// <!-- inputs/outputs -->
        ///   @param pdte the pdte_t to add a pt_t too
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     otherwise
        ///
        [[nodiscard]] constexpr auto
        add_pt(loader::pdte_t *const pdte) noexcept -> bsl::errc_type
        {
            auto const *const table{m_page_pool->template allocate<void>(ALLOCATE_TAG_PTS)};
            if (bsl::unlikely(nullptr == table)) {
                bsl::print<bsl::V>() << bsl::here();
                return bsl::errc_failure;
            }

            auto const table_phys{m_page_pool->virt_to_phys(table)};
            if (bsl::unlikely(!table_phys)) {
                bsl::print<bsl::V>() << bsl::here();
                return bsl::errc_failure;
            }

            pdte->phys = (table_phys >> PAGE_SHIFT).get();
            pdte->p = bsl::ONE_UMAX.get();
            pdte->rw = bsl::ONE_UMAX.get();
            pdte->us = bsl::ONE_UMAX.get();

            return bsl::errc_success;
        }

        /// <!-- description -->
        ///   @brief Adds a pt_t to the provided pdte_t.
        ///
        /// <!-- inputs/outputs -->
        ///   @param pdte the pdte_t to add a pt_t too
        ///
        constexpr void
        remove_pt(loader::pdte_t *const pdte) noexcept
        {
            for (auto const elem : get_pt(pdte)->entries) {
                if (elem.data->p == bsl::ZERO_UMAX) {
                    continue;
                }

                switch (elem.data->auto_release) {
                    case MAP_PAGE_NO_AUTO_RELEASE.get(): {
                        break;
                    }

                    case MAP_PAGE_AUTO_RELEASE_ALLOC_PAGE.get(): {
                        m_page_pool->deallocate(
                            this->pte_from_page_pool_to_virt(elem.data),
                            ALLOCATE_TAG_BF_MEM_OP_ALLOC_PAGE);

                        break;
                    }

                    case MAP_PAGE_AUTO_RELEASE_ALLOC_HUGE.get(): {
                        m_huge_pool->deallocate(this->pte_from_huge_pool_to_virt(elem.data));
                        break;
                    }

                    case MAP_PAGE_AUTO_RELEASE_ALLOC_HEAP.get(): {
                        m_page_pool->deallocate(
                            this->pte_from_page_pool_to_virt(elem.data),
                            ALLOCATE_TAG_BF_MEM_OP_ALLOC_HEAP);

                        break;
                    }

                    case MAP_PAGE_AUTO_RELEASE_STACK.get(): {
                        m_page_pool->deallocate(
                            this->pte_from_page_pool_to_virt(elem.data), ALLOCATE_TAG_EXT_STACK);

                        break;
                    }

                    case MAP_PAGE_AUTO_RELEASE_TLS.get(): {
                        m_page_pool->deallocate(
                            this->pte_from_page_pool_to_virt(elem.data), ALLOCATE_TAG_EXT_TLS);

                        break;
                    }

                    case MAP_PAGE_AUTO_RELEASE_ELF.get(): {
                        m_page_pool->deallocate(
                            this->pte_from_page_pool_to_virt(elem.data), ALLOCATE_TAG_EXT_ELF);

                        break;
                    }

                    default: {
                        bsl::error() << "uknown tag\n" << bsl::here();
                        break;
                    }
                }
            }

            m_page_pool->deallocate(get_pt(pdte), ALLOCATE_TAG_PTS);
        }

        /// <!-- description -->
        ///   @brief Returns the pt_t associated with the provided
        ///     pdte_t.
        ///
        /// <!-- inputs/outputs -->
        ///   @param pdte the pdte_t to get the pt_t from
        ///   @return A pointer to the requested pt_t
        ///
        [[nodiscard]] constexpr auto
        get_pt(loader::pdte_t *const pdte) noexcept -> pt_t *
        {
            bsl::safe_uintmax entry_phys{pdte->phys};
            entry_phys <<= PAGE_SHIFT;

            return m_page_pool->template phys_to_virt<pt_t>(entry_phys);
        }

        /// <!-- description -->
        ///   @brief Returns the pt_t associated with the provided
        ///     pdte_t.
        ///
        /// <!-- inputs/outputs -->
        ///   @param pdte the pdte_t to get the pt_t from
        ///   @return A pointer to the requested pt_t
        ///
        [[nodiscard]] constexpr auto
        get_pt(loader::pdte_t const *const pdte) const noexcept -> pt_t const *
        {
            bsl::safe_uintmax entry_phys{pdte->phys};
            entry_phys <<= PAGE_SHIFT;

            return m_page_pool->template phys_to_virt<pt_t const>(entry_phys);
        }

        /// <!-- description -->
        ///   @brief Returns the page-table (PT) offset given a
        ///     virtual address.
        ///
        /// <!-- inputs/outputs -->
        ///   @param virt the virtual address to get the PT offset from.
        ///   @return the PT offset from the virtual address
        ///
        [[nodiscard]] constexpr auto
        pto(bsl::safe_uintmax const &virt) const noexcept -> bsl::safe_uintmax
        {
            constexpr bsl::safe_uintmax mask{bsl::to_umax(0x1FF)};
            constexpr bsl::safe_uintmax shift{bsl::to_umax(12)};
            return (virt >> shift) & mask;
        }

        /// <!-- description -->
        ///   @brief Dumps the provided pt_t
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam T the type of outputter provided
        ///   @param o the instance of the outputter used to output the value.
        ///   @param pt the pt_t to dump
        ///   @param is_pml4te_last_index true if the parent pml4te was the
        ///     last pml4te in the table
        ///   @param is_pdpte_last_index true if the parent pdpte was the
        ///     last pdpte in the table
        ///   @param is_pdte_last_index true if the parent pdte was the
        ///     last pdte in the table
        ///
        template<typename T>
        constexpr void
        dump_pt(
            bsl::out<T> const o,
            pt_t const *const pt,
            bool is_pml4te_last_index,
            bool is_pdpte_last_index,
            bool is_pdte_last_index) const noexcept
        {
            bsl::safe_uintmax const last_index{this->get_last_index(pt)};

            for (auto const elem : pt->entries) {
                if (bsl::ZERO_UMAX == elem.data->p) {
                    continue;
                }

                this->output_spacing(o, is_pml4te_last_index);
                this->output_spacing(o, is_pdpte_last_index);
                this->output_spacing(o, is_pdte_last_index);
                this->output_decoration_and_index(o, elem.index, last_index);

                o << bsl::rst;
                this->output_entry_and_flags(o, elem.data);
            }
        }

        /// <!-- description -->
        ///   @brief Returns the virtual address associated with a specific
        ///     pte that was allocated using the page pool.
        ///
        /// <!-- inputs/outputs -->
        ///   @param pte the pte_t to convert
        ///   @return Returns the virtual address associated with a specific
        ///     pte that was allocated using the page pool.
        ///
        [[nodiscard]] constexpr auto
        pte_from_page_pool_to_virt(loader::pte_t *const pte) noexcept -> void *
        {
            bsl::safe_uintmax entry_phys{pte->phys};
            entry_phys <<= PAGE_SHIFT;

            return m_page_pool->template phys_to_virt<void>(entry_phys);
        }

        /// <!-- description -->
        ///   @brief Returns the virtual address associated with a specific
        ///     pte that was allocated using the huge pool.
        ///
        /// <!-- inputs/outputs -->
        ///   @param pte the pte_t to convert
        ///   @return Returns the virtual address associated with a specific
        ///     pte that was allocated using the huge pool.
        ///
        [[nodiscard]] constexpr auto
        pte_from_huge_pool_to_virt(loader::pte_t *const pte) noexcept -> void *
        {
            bsl::safe_uintmax entry_phys{pte->phys};
            entry_phys <<= PAGE_SHIFT;

            return m_huge_pool->template phys_to_virt<void>(entry_phys);
        }

        /// <!-- description -->
        ///   @brief Returns the page aligned version of the addr
        ///
        /// <!-- inputs/outputs -->
        ///   @param addr the address to query
        ///   @return Returns the page aligned version of the addr
        ///
        [[nodiscard]] constexpr auto
        page_aligned(bsl::safe_uintmax const &addr) const noexcept -> bsl::safe_uintmax
        {
            return (addr & ~(PAGE_SIZE - bsl::ONE_UMAX));
        }

        /// <!-- description -->
        ///   @brief Returns true if the provided address is page aligned
        ///
        /// <!-- inputs/outputs -->
        ///   @param addr the address to query
        ///   @return Returns true if the provided address is page aligned
        ///
        [[nodiscard]] constexpr auto
        is_page_aligned(bsl::safe_uintmax const &addr) const noexcept -> bool
        {
            return (addr & (PAGE_SIZE - bsl::ONE_UMAX)) == bsl::ZERO_UMAX;
        }

        /// <!-- description -->
        ///   @brief Allocates a page from the provided page pool and maps it
        ///     into the root page table being managed by this class The page
        ///     is marked as "auto release", meaning when this root page table
        ///     is released, the pages allocated by this function will
        ///     automatically be deallocated and put back into the provided
        ///     page pool.
        ///
        /// <!-- inputs/outputs -->
        ///   @param page_virt the virtual address to map the allocated
        ///     page to
        ///   @param page_flags defines how memory should be mapped
        ///   @param auto_release defines what auto release tag to use
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     otherwise
        ///
        [[nodiscard]] constexpr auto
        allocate_page(
            bsl::safe_uintmax const &page_virt,
            bsl::safe_uintmax const &page_flags,
            bsl::safe_uintmax const &auto_release) &noexcept -> void *
        {
            bsl::errc_type ret{};

            if (bsl::unlikely(!m_initialized)) {
                bsl::error() << "root_page_table_t not initialized\n" << bsl::here();
                return nullptr;
            }

            void *page{};
            switch (auto_release.get()) {
                case MAP_PAGE_AUTO_RELEASE_STACK.get(): {
                    page = m_page_pool->template allocate<void>(ALLOCATE_TAG_EXT_STACK);
                    break;
                }

                case MAP_PAGE_AUTO_RELEASE_TLS.get(): {
                    page = m_page_pool->template allocate<void>(ALLOCATE_TAG_EXT_TLS);
                    break;
                }

                case MAP_PAGE_AUTO_RELEASE_ELF.get(): {
                    page = m_page_pool->template allocate<void>(ALLOCATE_TAG_EXT_ELF);
                    break;
                }

                default: {
                    bsl::error() << "unknown tag\n" << bsl::here();
                    return nullptr;
                }
            }

            if (bsl::unlikely(nullptr == page)) {
                bsl::print<bsl::V>() << bsl::here();
                return nullptr;
            }

            auto const page_phys{m_page_pool->virt_to_phys(page)};
            if (bsl::unlikely(!page_phys)) {
                bsl::error() << "physical address is invalid: "    // --
                             << bsl::hex(page_phys)                // --
                             << bsl::endl                          // --
                             << bsl::here();                       // --

                return nullptr;
            }

            ret = this->map_page(page_virt, page_phys, page_flags, auto_release);
            if (bsl::unlikely(!ret)) {
                bsl::print<bsl::V>() << bsl::here();
                return nullptr;
            }

            return page;
        }

        /// <!-- description -->
        ///   @brief Releases the memory allocated for tables
        ///
        constexpr void
        release_tables() &noexcept
        {
            if (bsl::unlikely(nullptr == m_pml4t)) {
                return;
            }

            if (bsl::unlikely(nullptr == m_page_pool)) {
                return;
            }

            if (bsl::unlikely(nullptr == m_huge_pool)) {
                return;
            }

            for (auto const elem : m_pml4t->entries) {
                if (elem.data->p == bsl::ZERO_UMAX) {
                    continue;
                }

                if (elem.data->alias != bsl::ZERO_UMAX) {
                    continue;
                }

                this->remove_pdpt(elem.data);
            }

            m_page_pool->deallocate(m_pml4t, ALLOCATE_TAG_PML4TS);
            m_pml4t = {};
            m_pml4t_phys = bsl::safe_uintmax::zero(true);
        }

    public:
        /// @brief an alias for INTRINSIC_CONCEPT
        using intrinsic_type = INTRINSIC_CONCEPT;
        /// @brief an alias for PAGE_POOL_CONCEPT
        using page_pool_type = PAGE_POOL_CONCEPT;
        /// @brief an alias for HUGE_POOL_CONCEPT
        using huge_pool_type = HUGE_POOL_CONCEPT;

        /// <!-- description -->
        ///   @brief Creates a root_page_table_t
        ///
        constexpr root_page_table_t() noexcept = default;

        /// <!-- description -->
        ///   @brief Initializes this root_page_table_t
        ///
        /// <!-- inputs/outputs -->
        ///   @param intrinsic the intrinsics to use
        ///   @param page_pool the page pool to use
        ///   @param huge_pool the huge pool to use
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     otherwise
        ///
        [[nodiscard]] constexpr auto
        initialize(
            INTRINSIC_CONCEPT *const intrinsic,
            PAGE_POOL_CONCEPT *const page_pool,
            HUGE_POOL_CONCEPT *const huge_pool) &noexcept -> bsl::errc_type
        {
            if (bsl::unlikely(m_initialized)) {
                bsl::error() << "root_page_table_t already initialized\n" << bsl::here();
                return bsl::errc_failure;
            }

            bsl::finally release_on_error{[this]() noexcept -> void {
                this->release();
            }};

            m_intrinsic = intrinsic;
            if (bsl::unlikely(nullptr == intrinsic)) {
                bsl::error() << "invalid intrinsic\n" << bsl::here();
                return bsl::errc_failure;
            }

            m_page_pool = page_pool;
            if (bsl::unlikely(nullptr == page_pool)) {
                bsl::error() << "invalid page_pool\n" << bsl::here();
                return bsl::errc_failure;
            }

            m_huge_pool = huge_pool;
            if (bsl::unlikely(nullptr == huge_pool)) {
                bsl::error() << "invalid huge_pool\n" << bsl::here();
                return bsl::errc_failure;
            }

            m_pml4t = m_page_pool->template allocate<pml4t_t>(ALLOCATE_TAG_PML4TS);
            if (bsl::unlikely(nullptr == m_pml4t)) {
                bsl::print<bsl::V>() << bsl::here();
                return bsl::errc_failure;
            }

            m_pml4t_phys = m_page_pool->virt_to_phys(m_pml4t);
            if (bsl::unlikely(!m_pml4t_phys)) {
                bsl::print<bsl::V>() << bsl::here();
                return bsl::errc_failure;
            }

            release_on_error.ignore();
            m_initialized = true;

            return bsl::errc_success;
        }

        /// <!-- description -->
        ///   @brief Releases all of the resources used by the RPT.
        ///
        constexpr void
        release() &noexcept
        {
            this->release_tables();

            m_huge_pool = {};
            m_page_pool = {};
            m_intrinsic = {};
            m_initialized = false;
        }

        /// <!-- description -->
        ///   @brief Returns true if this RPT is initialized.
        ///
        /// <!-- inputs/outputs -->
        ///   @return Returns true if this RPT is initialized.
        ///
        [[nodiscard]] constexpr auto
        is_initialized() const &noexcept -> bool
        {
            return m_initialized;
        }

        /// <!-- description -->
        ///   @brief Destructor
        ///
        constexpr ~root_page_table_t() noexcept = default;

        /// <!-- description -->
        ///   @brief copy constructor
        ///
        /// <!-- inputs/outputs -->
        ///   @param o the object being copied
        ///
        constexpr root_page_table_t(root_page_table_t const &o) noexcept = delete;

        /// <!-- description -->
        ///   @brief move constructor
        ///
        /// <!-- inputs/outputs -->
        ///   @param o the object being moved
        ///
        constexpr root_page_table_t(root_page_table_t &&o) noexcept = default;

        /// <!-- description -->
        ///   @brief copy assignment
        ///
        /// <!-- inputs/outputs -->
        ///   @param o the object being copied
        ///   @return a reference to *this
        ///
        [[maybe_unused]] constexpr auto operator=(root_page_table_t const &o) &noexcept
            -> root_page_table_t & = delete;

        /// <!-- description -->
        ///   @brief move assignment
        ///
        /// <!-- inputs/outputs -->
        ///   @param o the object being moved
        ///   @return a reference to *this
        ///
        [[maybe_unused]] constexpr auto operator=(root_page_table_t &&o) &noexcept
            -> root_page_table_t & = default;

        /// <!-- description -->
        ///   @brief Sets the current root page table to this root page table.
        ///
        /// <!-- inputs/outputs -->
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     otherwise
        ///
        [[nodiscard]] constexpr auto
        activate() const &noexcept -> bsl::errc_type
        {
            if (bsl::unlikely(!m_initialized)) {
                bsl::error() << "root_page_table_t not initialized\n" << bsl::here();
                return bsl::errc_failure;
            }

            m_intrinsic->set_cr3(m_pml4t_phys);
            return bsl::errc_success;
        }

        /// <!-- description -->
        ///   @brief Returns true if the current root page table is this root
        ///     page table.
        ///
        /// <!-- inputs/outputs -->
        ///   @return Returns true if the current root page table is this root
        ///     page table.
        ///
        [[nodiscard]] constexpr auto
        is_active() const &noexcept -> bool
        {
            return (m_intrinsic->cr3() == m_pml4t_phys);
        }

        /// <!-- description -->
        ///   @brief Given a root page table, the pml4te_t enties are aliased
        ///     into this page table, allowing software using this root page
        ///     table to access the memory mapped into the provided root page
        ///     table. The additions are aliases only, meaning when this root
        ///     page table loses scope, aliased entries added by this function
        ///     are not returned back to the page pool.
        ///
        /// <!-- inputs/outputs -->
        ///   @param rpt the root page table to add aliases to
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     otherwise
        ///
        [[nodiscard]] constexpr auto
        add_tables(void const *const rpt) &noexcept -> bsl::errc_type
        {
            bsl::lock_guard lock{m_rpt_lock};

            if (bsl::unlikely(!m_initialized)) {
                bsl::error() << "root_page_table_t not initialized\n" << bsl::here();
                return bsl::errc_failure;
            }

            auto const *const pml4t{static_cast<pml4t_t const *>(rpt)};
            if (bsl::unlikely(nullptr == pml4t)) {
                bsl::error() << "invalid rpt\n" << bsl::here();
                return bsl::errc_failure;
            }

            for (auto const elem : pml4t->entries) {
                if (elem.data->p != bsl::ZERO_UMAX) {
                    auto *const pml4e_dst{m_pml4t->entries.at_if(elem.index)};

                    *pml4e_dst = *elem.data;
                    pml4e_dst->alias = bsl::ONE_UMAX.get();
                }
                else {
                    bsl::touch();
                }
            }

            return bsl::errc_success;
        }

        /// <!-- description -->
        ///   @brief Given a root page table, the pml4te_t enties are aliased
        ///     into this page table, allowing software using this root page
        ///     table to access the memory mapped into the provided root page
        ///     table. The additions are aliases only, meaning when this root
        ///     page table loses scope, aliased entries added by this function
        ///     are not returned back to the page pool.
        ///
        /// <!-- inputs/outputs -->
        ///   @param rpt the root page table to add aliases to
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     otherwise
        ///
        [[nodiscard]] constexpr auto
        add_tables(root_page_table_t const &rpt) &noexcept -> bsl::errc_type
        {
            return this->add_tables(rpt.m_pml4t);
        }

        /// <!-- description -->
        ///   @brief Maps a page into the root page table being managed
        ///     by this class.
        ///
        /// <!-- inputs/outputs -->
        ///   @param page_virt the virtual address to map the physical address
        ///     too.
        ///   @param page_phys the physical address to map.
        ///   @param page_flags defines how memory should be mapped
        ///   @param auto_release defines what auto release tag to use
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     otherwise
        ///
        [[nodiscard]] constexpr auto
        map_page(
            bsl::safe_uintmax const &page_virt,
            bsl::safe_uintmax const &page_phys,
            bsl::safe_uintmax const &page_flags,
            bsl::safe_uintmax const &auto_release) &noexcept -> bsl::errc_type
        {
            bsl::lock_guard lock{m_rpt_lock};

            if (bsl::unlikely(!m_initialized)) {
                bsl::error() << "root_page_table_t not initialized\n" << bsl::here();
                return bsl::errc_failure;
            }

            if (bsl::unlikely(page_virt.is_zero())) {
                bsl::error() << "virtual address is invalid: "    // --
                             << bsl::hex(page_virt)               // --
                             << bsl::endl                         // --
                             << bsl::here();                      // --

                return bsl::errc_failure;
            }

            if (bsl::unlikely(!this->is_page_aligned(page_virt))) {
                bsl::error() << "virtual address is not page aligned: "    // --
                             << bsl::hex(page_virt)                        // --
                             << bsl::endl                                  // --
                             << bsl::here();                               // --

                return bsl::errc_failure;
            }

            if (bsl::unlikely(page_phys.is_zero())) {
                bsl::error() << "physical address is invalid: "    // --
                             << bsl::hex(page_phys)                // --
                             << bsl::endl                          // --
                             << bsl::here();                       // --

                return bsl::errc_failure;
            }

            if (bsl::unlikely(!this->is_page_aligned(page_phys))) {
                bsl::error() << "physical address is not page aligned: "    // --
                             << bsl::hex(page_phys)                         // --
                             << bsl::endl                                   // --
                             << bsl::here();                                // --

                return bsl::errc_failure;
            }

            if ((page_flags & MAP_PAGE_WRITE).is_pos()) {
                if ((page_flags & MAP_PAGE_EXECUTE).is_pos()) {
                    bsl::error() << "invalid page_flags: "    // --
                                 << bsl::hex(page_flags)      // --
                                 << bsl::endl                 // --
                                 << bsl::here();              // --

                    return bsl::errc_failure;
                }
            }

            auto *const pml4te{m_pml4t->entries.at_if(this->pml4to(page_virt))};
            if (pml4te->p == bsl::ZERO_UMAX) {
                if (bsl::unlikely(!this->add_pdpt(pml4te))) {
                    bsl::print<bsl::V>() << bsl::here();
                    return bsl::errc_failure;
                }

                bsl::touch();
            }
            else {

                /// NOTE:
                /// - The loader doesn't map in the memory associated with
                ///   the microkernel's page tables. This means this code
                ///   cannot walk any pages mapped to the microkernel, it
                ///   can only alias these pages. For this reason, mapping
                ///   must always take place on userspace specific memory
                ///   and the address spaces must be distinct.
                /// - This is one of the reasons (not the only reason) why
                ///   userspace sits in the other half of the canonical
                ///   address space, and why it has it's own direct map
                ///   region. This ensures userspace and the kernel do not
                ///   share the same pml4 entries.
                ///

                if (pml4te->us == bsl::ZERO_UMAX) {
                    bsl::error() << "attempt to map the userspace address "              // --
                                 << bsl::hex(page_virt)                                  // --
                                 << " in an address range owned by the kernel failed"    // --
                                 << bsl::endl                                            // --
                                 << bsl::here();                                         // --

                    return bsl::errc_failure;
                }

                bsl::touch();
            }

            auto *const pdpt{this->get_pdpt(pml4te)};
            auto *const pdpte{pdpt->entries.at_if(this->pdpto(page_virt))};
            if (pdpte->p == bsl::ZERO_UMAX) {
                if (bsl::unlikely(!this->add_pdt(pdpte))) {
                    bsl::print<bsl::V>() << bsl::here();
                    return bsl::errc_failure;
                }

                bsl::touch();
            }
            else {
                bsl::touch();
            }

            auto *const pdt{this->get_pdt(pdpte)};
            auto *const pdte{pdt->entries.at_if(this->pdto(page_virt))};
            if (pdte->p == bsl::ZERO_UMAX) {
                if (bsl::unlikely(!this->add_pt(pdte))) {
                    bsl::print<bsl::V>() << bsl::here();
                    return bsl::errc_failure;
                }

                bsl::touch();
            }
            else {
                bsl::touch();
            }

            auto *const pt{this->get_pt(pdte)};
            auto *const pte{pt->entries.at_if(this->pto(page_virt))};
            if (bsl::unlikely(pte->p != bsl::ZERO_UMAX)) {
                bsl::error() << "virtual address "     // --
                             << bsl::hex(page_virt)    // --
                             << " already mapped"      // --
                             << bsl::endl              // --
                             << bsl::here();           // --

                return bsl::errc_failure;
            }

            pte->phys = (page_phys >> PAGE_SHIFT).get();
            pte->p = bsl::ONE_UMAX.get();
            pte->us = bsl::ONE_UMAX.get();
            pte->auto_release = auto_release.get();

            if (!(page_flags & MAP_PAGE_WRITE).is_zero()) {
                pte->rw = bsl::ONE_UMAX.get();
            }
            else {
                pte->rw = bsl::ZERO_UMAX.get();
            }

            if (!(page_flags & MAP_PAGE_EXECUTE).is_zero()) {
                pte->nx = bsl::ZERO_UMAX.get();
            }
            else {
                pte->nx = bsl::ONE_UMAX.get();
            }

            return bsl::errc_success;
        }

        /// <!-- description -->
        ///   @brief Maps a page into the root page table being managed
        ///     by this class. This version allows for unaligned virtual and
        ///     physical addresses and will perform the alignment for you.
        ///     Note that you should only use this function if you actually
        ///     need unaligned support to ensure alignment mistakes are not
        ///     accidentally introduced.
        ///
        /// <!-- inputs/outputs -->
        ///   @param page_virt the virtual address to map the physical address
        ///     too.
        ///   @param page_phys the physical address to map. If the physical
        ///     address is set to 0, map_page will use the page pool to
        ///     determine the physical address.
        ///   @param page_flags defines how memory should be mapped
        ///   @param auto_release defines what auto release tag to use
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     otherwise
        ///
        [[nodiscard]] constexpr auto
        map_page_unaligned(
            bsl::safe_uintmax const &page_virt,
            bsl::safe_uintmax const &page_phys,
            bsl::safe_uintmax const &page_flags,
            bsl::safe_uintmax const &auto_release) &noexcept -> bsl::errc_type
        {
            return this->map_page(
                this->page_aligned(page_virt),
                this->page_aligned(page_phys),
                page_flags,
                auto_release);
        }

        /// <!-- description -->
        ///   @brief Allocates a page from the provided page pool and maps it
        ///     into the root page table being managed by this class The page
        ///     is marked as "auto release", meaning when this root page table
        ///     is released, the pages allocated by this function will
        ///     automatically be deallocated and put back into the provided
        ///     page pool. Note that this version maps the memory in as
        ///     read/write.
        ///
        /// <!-- inputs/outputs -->
        ///   @param page_virt the virtual address to map the allocated
        ///     page to
        ///   @param auto_release defines what auto release tag to use
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     otherwise
        ///
        [[nodiscard]] constexpr auto
        allocate_page_rw(
            bsl::safe_uintmax const &page_virt, bsl::safe_uintmax const &auto_release) &noexcept
            -> void *
        {
            return this->allocate_page(page_virt, MAP_PAGE_READ | MAP_PAGE_WRITE, auto_release);
        }

        /// <!-- description -->
        ///   @brief Allocates a page from the provided page pool and maps it
        ///     into the root page table being managed by this class The page
        ///     is marked as "auto release", meaning when this root page table
        ///     is released, the pages allocated by this function will
        ///     automatically be deallocated and put back into the provided
        ///     page pool. Note that this version maps the memory in as
        ///     read/execute.
        ///
        /// <!-- inputs/outputs -->
        ///   @param page_virt the virtual address to map the allocated
        ///     page to
        ///   @param auto_release defines what auto release tag to use
        ///   @return Returns bsl::errc_success on success, bsl::errc_failure
        ///     otherwise
        ///
        [[nodiscard]] constexpr auto
        allocate_page_rx(
            bsl::safe_uintmax const &page_virt, bsl::safe_uintmax const &auto_release) &noexcept
            -> void *
        {
            return this->allocate_page(page_virt, MAP_PAGE_READ | MAP_PAGE_EXECUTE, auto_release);
        }

        /// <!-- description -->
        ///   @brief Dumps the provided pml4_t
        ///
        /// <!-- inputs/outputs -->
        ///   @tparam T the type of outputter provided
        ///   @param o the instance of the outputter used to output the value.
        ///
        template<typename T>
        constexpr void
        dump(bsl::out<T> const o) const &noexcept
        {
            if (bsl::unlikely(!m_initialized)) {
                o << "[error]" << bsl::endl;
                return;
            }

            this->dump_pml4t(o, m_pml4t);
        }
    };

    /// <!-- description -->
    ///   @brief Outputs the provided mk::root_page_table_t to the provided
    ///     output type.
    ///   @related mk::root_page_table_t
    ///
    /// <!-- inputs/outputs -->
    ///   @tparam T the type of outputter provided
    ///   @tparam INTRINSIC_CONCEPT defines the type of intrinsics to use
    ///   @tparam PAGE_POOL_CONCEPT defines the type of page pool to use
    ///   @tparam HUGE_POOL_CONCEPT defines the type of huge pool to use
    ///   @tparam PAGE_SIZE defines the size of a page
    ///   @tparam PAGE_SHIFT defines number of bits in a page
    ///   @param o the instance of the outputter used to output the value.
    ///   @param rpt the mk::root_page_table_t to output
    ///   @return return o
    ///
    template<
        typename T,
        typename INTRINSIC_CONCEPT,
        typename PAGE_POOL_CONCEPT,
        typename HUGE_POOL_CONCEPT,
        bsl::uintmax PAGE_SIZE,
        bsl::uintmax PAGE_SHIFT>
    [[maybe_unused]] constexpr auto
    operator<<(
        bsl::out<T> const o,
        mk::root_page_table_t<
            INTRINSIC_CONCEPT,
            PAGE_POOL_CONCEPT,
            HUGE_POOL_CONCEPT,
            PAGE_SIZE,
            PAGE_SHIFT> const &rpt) noexcept -> bsl::out<T>
    {
        if constexpr (!o) {
            return o;
        }

        rpt.dump(o);
        return o;
    }
}

#endif
