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

#ifndef MAP_MK_DEBUG_RING_H
#define MAP_MK_DEBUG_RING_H

#include <debug_ring_t.h>
#include <root_page_table_t.h>

/**
 * <!-- description -->
 *   @brief This function maps the microkernel's debug_ring into the
 *     microkernel's root page tables.
 *
 * <!-- inputs/outputs -->
 *   @param debug_ring a pointer to a debug_ring_t that stores the debug_ring
 *     being mapped
 *   @param rpt the root page table to map the debug_ring into
 *   @return 0 on success, LOADER_FAILURE on failure.
 */
int64_t
map_mk_debug_ring(struct debug_ring_t const *const debug_ring, root_page_table_t *const rpt);

#endif
