//
// Copyright (C) 2019 Assured Information Security, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <file/file.h>
#include <file/details/file.h>

#include <ioctl/ioctl_controller.h>
#include <ioctl/details/ioctl_controller.h>

#include <ioctl/ioctl_debug.h>
#include <ioctl/details/ioctl_debug.h>

#include <bfm/details/main.h>
using namespace host;

auto
main(int argc, char *argv[]) -> int
{
    using main_t = details::main<
        file<details::file>,
        ioctl_controller<details::ioctl_controller>,
        ioctl_debug<details::ioctl_debug>>;

    try {
        return main_t().execute(argc, argv);
    }
    catch (const std::exception &e) {
        std::cout << "error: " << e.what() << '\n';
    }

    return EXIT_FAILURE;
}
