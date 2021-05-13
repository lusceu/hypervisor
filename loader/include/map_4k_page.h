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

#ifndef MAP_4K_PAGE_H
#define MAP_4K_PAGE_H

#include <root_page_table_t.h>
#include <types.h>

/**
 * <!-- description -->
 *   @brief This function maps a 4k page given a physical address into a
 *     provided root page table at the provided virtual address. If the page
 *     is already mapped, this function will fail. Also note that this memory
 *     might need to allocate memory to expand the size of the page table
 *     tree. If this function fails, it will NOT attempt to cleanup memory
 *     that it allocated. Instead, you should free the provided root page
 *     table as a whole on error, or once it is no longer needed.
 *
 *   @note The microkernel could be started from an existing OS like Windows
 *     and Linux, or even from UEFI. If the microkernel is started using an
 *     OS from the kernel, the address space of the kernel will likely exist
 *     in the upper half of the canonical address space. If UEFI is used,
 *     the address space would be in the lower half. The problem is, during
 *     the demote phase of the microkernel, we need to map in memory
 *     from the OS/UEFI as well as the microkernel's address space. The kernel
 *     could end up using the entire upper half as most kernels have a direct
 *     map for virt to phys and phys to virt conversions, so the upper half
 *     of the address space would be difficult to use as collisions are
 *     possible. The kernel will not use the lower half, but UEFI will. The
 *     difference with UEFI is that the addresses that we get will almost
 *     certainly be as small as possible, meaning the upper addresses in the
 *     lower half of the canonical address space are the most likely addresses
 *     that will not collide with the OS kernel, or UEFI, which is why we
 *     use these addresses. Since the hypervisor as a whole cannot take a
 *     ton of memory, we can safely use the upper address of the lower half
 *     of the canonical address space, ensuring that our microkernel addresses
 *     are still easy to pick out from extension addresses. Finally, we
 *     add padding to each of these addresses for Hypervisor Address Space
 *     Layout Randomization, so the addresses shown here could have a fudge
 *     factor added to them to add some entropy.
 *
 * <!-- inputs/outputs -->
 *   @param virt the virtual address to map phys to
 *   @param phys the physical address to map
 *   @param flags the p_flags field from the segment associated with this page
 *   @param rpt the root page table to place the resulting map
 *   @return 0 on success, LOADER_FAILURE on failure.
 */
int64_t map_4k_page(
    uint64_t const virt, uint64_t const phys, uint32_t const flags, root_page_table_t *const rpt);

#endif
