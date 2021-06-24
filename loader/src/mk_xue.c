/**
 * @copyright
 * Copyright (C) 2020 Assured Information Security, Inc.
 *
 * @copyright
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * @copyright
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * @copyright
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <mutable_span_t.h>
#include <constants.h>
#include <debug.h>
#include <mutable_span_t.h>
#include <platform.h>
#include <types.h>
#include <root_page_table_t.h>


/** @brief stores mmio & dma regions associated with Xue */
struct mutable_span_t g_mk_xue_mmio = {0};
struct mutable_span_t g_mk_xue_dma = {0};


/**
 * <!-- description -->
 *   @brief Allocates a chunk of memory for the dma pages used by Xue
 *     usb3 serial. Note that the "size" parameter is in total pages and
 *     not in bytes. Finally, if the provided size is 0, this function
 *     will allocate a default number of pages.
 *
 * <!-- inputs/outputs -->
 *   @param size the total number of pages (not bytes) to allocate
 *   @param page_pool the mutable_span_t to store the page pool addr/size.
 *   @return 0 on success, LOADER_FAILURE on failure.
 */

int64_t
alloc_mk_xue_dma(uint32_t const size, struct mutable_span_t *const xue_dma_pool)
{

    xue_dma_pool->size = HYPERVISOR_PAGE_SIZE * (uint64_t)size;

    xue_dma_pool->addr = platform_alloc(xue_dma_pool->size);
    if (((void *)0) == xue_dma_pool->addr) {
        bferror("platform_alloc failed");
        goto platform_alloc_failed;
    }

    return LOADER_SUCCESS;

platform_alloc_failed:

    platform_memset(xue_dma_pool, 0, sizeof(struct mutable_span_t));
    return LOADER_FAILURE;
}


/**
 * <!-- description -->
 *   @brief Outputs the contents of a provided mk huge pool.
 *
 * <!-- inputs/outputs -->
 *   @param xue_dma_pool the xue dma pool to output
 *   @return 0 on success, LOADER_FAILURE on failure.
 */

void
dump_mk_xue_dma(struct mutable_span_t *const xue_dma_pool)
{
    bfdebug("xue dma pool:");
    bfdebug_ptr(" - addr", xue_dma_pool->addr);
    bfdebug_x64(" - size", xue_dma_pool->size);
}

/**
 * <!-- description -->
 *   @brief This function maps the microkernel's huge pool into the
 *     microkernel's root page tables.
 *
 * <!-- inputs/outputs -->
 *   @param huge_pool a pointer to a mutable_span_t that stores the huge pool
 *     being mapped
 *   @param rpt the root page table to map the huge pool into
 *   @return 0 on success, LOADER_FAILURE on failure.
 */

// TODO 

int64_t
map_mk_xue_dma_pool(struct mutable_span_t const *const xue_dma_pool, root_page_table_t *const rpt)
{
    uint64_t off;
    uint64_t base_phys;
    uint64_t const base_virt = HYPERVISOR_MK_DMA_ADDR;

    base_phys = platform_virt_to_phys(xue_dma_pool->addr);
    if (((uint64_t)0) == base_phys) {
        bferror("platform_virt_to_phys failed");
        return LOADER_FAILURE;
    }

    for (off = ((uint64_t)0); off < xue_dma_pool->size; off += HYPERVISOR_PAGE_SIZE) {

        uint64_t phys = platform_virt_to_phys(xue_dma_pool->addr + off);
        if (((uint64_t)0) == phys) {
            bferror("platform_virt_to_phys failed");
            return LOADER_FAILURE;
        }

        if (phys != base_phys + off) {
            bferror("huge pool is not physically contiguous");
            return LOADER_FAILURE;
        }

        if (map_4k_page_rw((void *)(base_virt + phys), phys, rpt)) {
            bferror("map_4k_page_rw failed");
            return LOADER_FAILURE;
        }
    }

    return LOADER_SUCCESS;
}



/**
 * <!-- description -->
 *   @brief Outputs the contents of a provided mk huge pool.
 *
 * <!-- inputs/outputs -->
 *   @param xue_dma_pool the xue dma pool to output
 *   @return 0 on success, LOADER_FAILURE on failure.
 */

void
dump_mk_xue_mmio(struct mutable_span_t *const xue_mmio)
{
    bfdebug("xue mmio space:");
    bfdebug_ptr(" - addr", xue_mmio->addr);
    bfdebug_x64(" - size", xue_mmio->size);
}

int64_t
map_mk_xue_mmio(struct mutable_span_t const *const xue_mmio, root_page_table_t *const rpt)
{
    uint64_t off;
    uint64_t base_phys;
    uint64_t const base_virt = HYPERVISOR_MK_MMIO_ADDR;

    base_phys = xue_mmio->addr;

    for (off = ((uint64_t)0); off < xue_mmio->size; off += HYPERVISOR_PAGE_SIZE) {

        uint64_t phys = xue_mmio->addr + off;

        if (phys != base_phys + off) {
            bferror("xue mmio is not physically contiguous");
            return LOADER_FAILURE;
        }

        if (map_4k_page_rw((void *)(base_virt + phys), phys, rpt)) {
            bferror("xue mmio: map_4k_page_rw failed");
            return LOADER_FAILURE;
        }
    }

    return LOADER_SUCCESS;
}