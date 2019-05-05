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
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef IMPLEMENTATION_SERIAL_H
#define IMPLEMENTATION_SERIAL_H

#include <intrinsics.h>
#include <bfconstants.h>

// -----------------------------------------------------------------------------
// Definition
// -----------------------------------------------------------------------------

namespace bfvmm::implementation
{

class serial
{
public:

    enum baud_rate_t {
        baud_rate_50 = 0x0900,
        baud_rate_75 = 0x0600,
        baud_rate_110 = 0x0417,
        baud_rate_150 = 0x0300,
        baud_rate_300 = 0x0180,
        baud_rate_600 = 0x00C0,
        baud_rate_1200 = 0x0060,
        baud_rate_1800 = 0x0040,
        baud_rate_2000 = 0x003A,
        baud_rate_2400 = 0x0030,
        baud_rate_3600 = 0x0020,
        baud_rate_4800 = 0x0018,
        baud_rate_7200 = 0x0010,
        baud_rate_9600 = 0x000C,
        baud_rate_19200 = 0x0006,
        baud_rate_38400 = 0x0003,
        baud_rate_57600 = 0x0002,
        baud_rate_115200 = 0x0001
    };

    enum data_bits_t {
        char_length_5 = 0x00,
        char_length_6 = 0x01,
        char_length_7 = 0x02,
        char_length_8 = 0x03
    };

    enum stop_bits_t {
        stop_bits_1 = 0x00,
        stop_bits_2 = 0x04
    };

    enum parity_bits_t {
        parity_none = 0x00,
        parity_odd = 0x08,
        parity_even = 0x18,
        parity_mark = 0x28,
        parity_space = 0x38
    };

public:

    /// Constructor
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @param port the IO port of the serial device
    ///
    serial(uint16_t port = DEFAULT_COM_PORT) noexcept;

    /// Destructor
    ///
    /// @expects none
    /// @ensures none
    ///
    ~serial() = default;

    /// Get Instance
    ///
    /// Get an instance to the class.
    ///
    /// @expects none
    /// @ensures ret != nullptr
    ///
    /// @return a singleton instance of serial
    ///
    static serial *instance() noexcept;

    /// Set Baud Rate
    ///
    /// Sets the rate at which the serial device will operate. Note that the
    /// rate paramter is actually the divisor that is used, and a custom one
    /// can be used if desired. If 0 is provided, the default baud rate is
    /// used instead.
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @param rate desired baud rate
    ///
    void set_baud_rate(baud_rate_t rate) noexcept;

    /// Buad Rate
    ///
    /// Returns the baud rate of the serial device. If the serial device is
    /// set to a baud rate that this code does not recognize, unknown is
    /// returned.
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @return the baud rate
    ///
    baud_rate_t baud_rate() const noexcept;

    /// Set Data Bits
    ///
    /// Sets the size of the data that is transmitted. For more information
    /// on the this field, please see http://wiki.osdev.org/Serial_Ports.
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @param bits the desired data bits
    ///
    void set_data_bits(data_bits_t bits) noexcept;

    /// Data Bits
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @return the serial device's data bits
    ///
    data_bits_t data_bits() const noexcept;

    /// Set Stop Bits
    ///
    /// Sets the stop bits used for transmission. For more information
    /// on the this field, please see http://wiki.osdev.org/Serial_Ports.
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @param bits the desired stop bits
    ///
    void set_stop_bits(stop_bits_t bits) noexcept;

    /// Stop Bits
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @return the serial device's stop bits
    ///
    stop_bits_t stop_bits() const noexcept;

    /// Set Parity Bits
    ///
    /// Sets the parity bits used for transmission. For more information
    /// on the this field, please see http://wiki.osdev.org/Serial_Ports.
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @param bits the desired parity bits
    ///
    void set_parity_bits(parity_bits_t bits) noexcept;

    /// Parity Bits
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @return the serial device's parity bits
    ///
    parity_bits_t parity_bits() const noexcept;

    /// Write Character
    ///
    /// Writes a character to the serial device.
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @param c character to write
    ///
    void write(const char c) const noexcept;

private:

    void enable_dlab() const noexcept;
    void disable_dlab() const noexcept;
    bool is_transmit_empty() const noexcept;

    uint8_t inb(uint16_t addr) const noexcept;
    void outb(uint16_t addr, uint8_t data) const noexcept;

private:

    uint16_t m_port;

public:

    serial(serial &&) noexcept = default;
    serial &operator=(serial &&) noexcept = default;

    serial(const serial &) = delete;
    serial &operator=(const serial &) = delete;
};

}

#endif
