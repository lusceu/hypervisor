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

#ifndef ALLOC_L1T_H
#define ALLOC_L1T_H

#include <l0t_t.h>
#include <l1t_t.h>
#include <types.h>

/**
 * <!-- description -->
 *   @brief Given a l0t_t and a virtual address, this function allocates a
 *     l1t_t and adds it to the l0t_t. If an l1t_t has already been allocated,
 *     this function will fail.
 *
 * <!-- inputs/outputs -->
 *   @param l0 the l0t_t to add the newly allocated l1t_t to
 *   @param virt the virtual address to get the l0t_t offset from.
 *   @return a pointer to the newly allocated l1t_t on success, ((void *)0)
 *     otherwise.
 */
struct l1t_t *alloc_l1t(struct l0t_t *const l0, uint64_t const virt);

#endif
