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

#ifndef PDT_T_H
#define PDT_T_H

#include <pdte_t.h>
#include <pt_t.h>
#include <types.h>

#pragma pack(push, 1)

/** @brief defines total number of entries in the PDT */
#define LOADER_NUM_PDT_ENTRIES ((uint64_t)512)

/**
 * @struct pdt_t
 *
 * <!-- description -->
 *   @brief Defines the layout of a page-directory table (pdt).
 */
struct pdt_t
{
    /** @brief stores the entries for this page table */
    struct pdte_t entires[LOADER_NUM_PDT_ENTRIES];
    /** @brief stores pointers to child tables */
    struct pt_t *tables[LOADER_NUM_PDT_ENTRIES];
};

#pragma pack(pop)

#endif
