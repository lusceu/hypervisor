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
#include <types.h>
#include <constants.h>
#include <debug.h>
#include <map_4k_page_rw.h>
#include <mutable_span_t.h>
#include <platform.h>
#include <root_page_table_t.h>

/** @brief stores the mmio / dma pool used by the microkernel for Xue */
extern struct mutable_span_t g_mk_xue_mmio;
extern struct mutable_span_t g_mk_xue_dma;

/**
 * <!-- description -->
 *   @brief Allocates a chunk of memory for the huge pool used by the
 *     microkernel. Note that the "size" parameter is in total pages and
 *     not in bytes. Finally, if the provided size is 0, this function
 *     will allocate a default number of pages.
 *
 * <!-- inputs/outputs -->
 *   @param size the total number of pages (not bytes) to allocate
 *   @param page_pool the mutable_span_t to store the huge pool addr/size.
 *   @return 0 on success, LOADER_FAILURE on failure.
 */
int64_t 
alloc_mk_xue_dma(uint32_t const size, struct mutable_span_t *const xue_dma_pool);

void 
dump_mk_xue_dma_pool(struct mutable_span_t *const xue_dma_pool);

int64_t
map_mk_xue_dma(struct mutable_span_t const *const xue_dma_pool, root_page_table_t *const rpt);

/* logic of MMIO to own function ?
int64_t 
alloc_mk_xue_mmio(uint32_t const size, struct mutable_span_t *const xue_mmio);
*/

void 
dump_mk_xue_mmio(struct mutable_span_t *const xue_mmio);

int64_t
map_mk_xue_mmio(struct mutable_span_t const *const xue_mmio, root_page_table_t *const rpt);


