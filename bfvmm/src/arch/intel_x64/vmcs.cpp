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

#include <arch/intel_x64/vmcs/debug.h>
#include <arch/intel_x64/vmcs/16bit_control_fields.h>
#include <arch/intel_x64/vmcs/16bit_guest_state_fields.h>
#include <arch/intel_x64/vmcs/16bit_host_state_fields.h>
#include <arch/intel_x64/vmcs/32bit_control_fields.h>
#include <arch/intel_x64/vmcs/32bit_guest_state_fields.h>
#include <arch/intel_x64/vmcs/32bit_host_state_fields.h>
#include <arch/intel_x64/vmcs/32bit_read_only_data_fields.h>
#include <arch/intel_x64/vmcs/64bit_control_fields.h>
#include <arch/intel_x64/vmcs/64bit_guest_state_fields.h>
#include <arch/intel_x64/vmcs/64bit_host_state_fields.h>
#include <arch/intel_x64/vmcs/64bit_read_only_data_fields.h>
#include <arch/intel_x64/vmcs/natural_width_control_fields.h>
#include <arch/intel_x64/vmcs/natural_width_guest_state_fields.h>
#include <arch/intel_x64/vmcs/natural_width_host_state_fields.h>
#include <arch/intel_x64/vmcs/natural_width_read_only_data_fields.h>
#include <arch/intel_x64/cpuid.h>
#include <arch/intel_x64/crs.h>
#include <arch/intel_x64/msrs.h>
#include <arch/intel_x64/vmx.h>

#include <implementation/vcpu_t.h>
#include <implementation/arch/intel_x64/check.h>
#include <implementation/arch/intel_x64/vmcs.h>

// -----------------------------------------------------------------------------
// Prototypes
// -----------------------------------------------------------------------------

extern "C" bool _vmlaunch(uint64_t ptr) noexcept;
extern "C" bool _vmresume(uint64_t ptr) noexcept;
extern "C" bool _vmprmote(uint64_t ptr) noexcept;

// -----------------------------------------------------------------------------
// Implementation
// -----------------------------------------------------------------------------

namespace bfvmm::implementation::intel_x64
{

vmcs::vmcs()
{
    using namespace ::intel_x64::msrs::ia32_vmx_basic;

    auto view = m_vmcs_region.view();
    view[0] = gsl::narrow<uint32_t>(revision_id::get());
}

vmcs::~vmcs()
{
    ::intel_x64::vm::clear(m_vmcs_region.hpa());
}

void
vmcs::demote()
{
    if (vcpu_t_cast(this)->is_guest_vcpu()) {
        throw std::runtime_error("demoting a guest vCPU is unsupported");
    }

    this->launch();

    ::x64::cpuid::get(0x4BF00010, 0, 0, 0);
    ::x64::cpuid::get(0x4BF00011, 0, 0, 0);
}

void
vmcs::promote()
{
    if (vcpu_t_cast(this)->is_guest_vcpu()) {
        throw std::runtime_error("demoting a guest vCPU is unsupported");
    }

    ::x64::cpuid::get(0x4BF00020, 0, 0, 0);
    ::x64::cpuid::get(0x4BF00021, 0, 0, 0);
}

void
vmcs::check() const noexcept
{
    try {
        check::all();
        return;
    }
    catch (std::exception &e) {
        bfdebug_transaction(0, [&](std::string * msg) {
            bferror_lnbr(0, msg);
            bferror_brk1(0, msg);
            bferror_info(0, typeid(e).name(), msg);
            bferror_brk1(0, msg);
            bferror_info(0, e.what(), msg);
        });

        ::intel_x64::vmcs::dump();
    }
}

void
vmcs::launch()
{
    for (const auto &d : m_vmlaunch_delegates) {
        d(vcpu_t_cast(this));
    }

    if (!_vmlaunch(vcpu_t_cast(this)->state_hva())) {
        this->check();
        throw std::runtime_error("_vmlaunch failed");
    }

    m_launched = true;
}

void
vmcs::resume()
{
    for (const auto &d : m_vmresume_delegates) {
        d(vcpu_t_cast(this));
    }

    if (!_vmresume(vcpu_t_cast(this)->state_hva())) {
        this->check();
        throw std::runtime_error("_vmresume failed");
    }
}

void
vmcs::__arch_run()
{
    if (m_launched) {
        this->resume();
    }
    else {
        this->launch();
    }
}

bool
vmcs::__arch_advance_ip()
{
    this->set_rip(this->rip() + this->vmexit_instr_len());
    return true;
}

void
vmcs::__arch_load()
{
    for (const auto &d : m_vmload_delegates) {
        d(vcpu_t_cast(this));
    }

    ::intel_x64::vm::load(m_vmcs_region.hpa());
}

void
vmcs::__arch_clear()
{
    if (vcpu_t_cast(this)->is_host_vcpu()) {
        throw std::runtime_error("clearing a host vCPU is unsupported");
    }

    for (const auto &d : m_vmclear_delegates) {
        d(vcpu_t_cast(this));
    }

    m_launched = false;
    ::intel_x64::vm::clear(m_vmcs_region.hpa());
}

void
vmcs::__vmcs_add_vmlaunch_delegate(const vmcs_delegate_t &d)
{ m_vmlaunch_delegates.emplace_back(d); }

void
vmcs::__vmcs_add_vmresume_delegate(const vmcs_delegate_t &d)
{ m_vmresume_delegates.emplace_back(d); }

void
vmcs::__vmcs_add_vmload_delegate(const vmcs_delegate_t &d)
{ m_vmload_delegates.emplace_back(d); }

void
vmcs::__vmcs_add_vmclear_delegate(const vmcs_delegate_t &d)
{ m_vmclear_delegates.emplace_back(d); }

// -----------------------------------------------------------------------------
// VMCS Fields
// -----------------------------------------------------------------------------

vmcs::vmcs_field16_t
vmcs::__virtual_processor_identifier() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::virtual_processor_identifier::get()); }

void
vmcs::__set_virtual_processor_identifier(vmcs_field16_t val)
{ ::intel_x64::vmcs::virtual_processor_identifier::set(val); }

vmcs::vmcs_field16_t
vmcs::__posted_int_notification_vector() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::posted_int_notification_vector::get()); }

void
vmcs::__set_posted_int_notification_vector(vmcs_field16_t val)
{ ::intel_x64::vmcs::posted_int_notification_vector::set(val); }

vmcs::vmcs_field16_t
vmcs::__eptp_index() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::eptp_index::get()); }

void
vmcs::__set_eptp_index(vmcs_field16_t val)
{ ::intel_x64::vmcs::eptp_index::set(val); }

vmcs::vmcs_field16_t
vmcs::__es_selector() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::guest_es_selector::get()); }

void
vmcs::__set_es_selector(vmcs_field16_t val)
{ ::intel_x64::vmcs::guest_es_selector::set(val); }

vmcs::vmcs_field16_t
vmcs::__cs_selector() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::guest_cs_selector::get()); }

void
vmcs::__set_cs_selector(vmcs_field16_t val)
{ ::intel_x64::vmcs::guest_cs_selector::set(val); }

vmcs::vmcs_field16_t
vmcs::__ss_selector() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::guest_ss_selector::get()); }

void
vmcs::__set_ss_selector(vmcs_field16_t val)
{ ::intel_x64::vmcs::guest_ss_selector::set(val); }

vmcs::vmcs_field16_t
vmcs::__ds_selector() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::guest_ds_selector::get()); }

void
vmcs::__set_ds_selector(vmcs_field16_t val)
{ ::intel_x64::vmcs::guest_ds_selector::set(val); }

vmcs::vmcs_field16_t
vmcs::__fs_selector() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::guest_fs_selector::get()); }

void
vmcs::__set_fs_selector(vmcs_field16_t val)
{ ::intel_x64::vmcs::guest_fs_selector::set(val); }

vmcs::vmcs_field16_t
vmcs::__gs_selector() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::guest_gs_selector::get()); }

void
vmcs::__set_gs_selector(vmcs_field16_t val)
{ ::intel_x64::vmcs::guest_gs_selector::set(val); }

vmcs::vmcs_field16_t
vmcs::__ldtr_selector() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::guest_ldtr_selector::get()); }

void
vmcs::__set_ldtr_selector(vmcs_field16_t val)
{ ::intel_x64::vmcs::guest_ldtr_selector::set(val); }

vmcs::vmcs_field16_t
vmcs::__tr_selector() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::guest_tr_selector::get()); }

void
vmcs::__set_tr_selector(vmcs_field16_t val)
{ ::intel_x64::vmcs::guest_tr_selector::set(val); }

vmcs::vmcs_field16_t
vmcs::__int_status() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::guest_int_status::get()); }

void
vmcs::__set_int_status(vmcs_field16_t val)
{ ::intel_x64::vmcs::guest_int_status::set(val); }

vmcs::vmcs_field16_t
vmcs::__pml_index() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::pml_index::get()); }

void
vmcs::__set_pml_index(vmcs_field16_t val)
{ ::intel_x64::vmcs::pml_index::set(val); }

vmcs::vmcs_field64_t
vmcs::__io_bitmap_a_addr() const
{ return ::intel_x64::vmcs::io_bitmap_a_addr::get(); }

void
vmcs::__set_io_bitmap_a_addr(vmcs_field64_t val)
{ ::intel_x64::vmcs::io_bitmap_a_addr::set(val); }

vmcs::vmcs_field64_t
vmcs::__io_bitmap_b_addr() const
{ return ::intel_x64::vmcs::io_bitmap_b_addr::get(); }

void
vmcs::__set_io_bitmap_b_addr(vmcs_field64_t val)
{ ::intel_x64::vmcs::io_bitmap_b_addr::set(val); }

vmcs::vmcs_field64_t
vmcs::__msr_bitmaps_addr() const
{ return ::intel_x64::vmcs::msr_bitmaps_addr::get(); }

void
vmcs::__set_msr_bitmaps_addr(vmcs_field64_t val)
{ ::intel_x64::vmcs::msr_bitmaps_addr::set(val); }

vmcs::vmcs_field64_t
vmcs::__vmexit_msr_store_addr() const
{ return ::intel_x64::vmcs::vmexit_msr_store_addr::get(); }

void
vmcs::__set_vmexit_msr_store_addr(vmcs_field64_t val)
{ ::intel_x64::vmcs::vmexit_msr_store_addr::set(val); }

vmcs::vmcs_field64_t
vmcs::__vmexit_msr_load_addr() const
{ return ::intel_x64::vmcs::vmexit_msr_load_addr::get(); }

void
vmcs::__set_vmexit_msr_load_addr(vmcs_field64_t val)
{ ::intel_x64::vmcs::vmexit_msr_load_addr::set(val); }

vmcs::vmcs_field64_t
vmcs::__vmentry_msr_load_addr() const
{ return ::intel_x64::vmcs::vmentry_msr_load_addr::get(); }

void
vmcs::__set_vmentry_msr_load_addr(vmcs_field64_t val)
{ ::intel_x64::vmcs::vmentry_msr_load_addr::set(val); }

vmcs::vmcs_field64_t
vmcs::__executive_vmcs_ptr() const
{ return ::intel_x64::vmcs::executive_vmcs_ptr::get(); }

void
vmcs::__set_executive_vmcs_ptr(vmcs_field64_t val)
{ ::intel_x64::vmcs::executive_vmcs_ptr::set(val); }

vmcs::vmcs_field64_t
vmcs::__pml_addr() const
{ return ::intel_x64::vmcs::pml_addr::get(); }

void
vmcs::__set_pml_addr(vmcs_field64_t val)
{ ::intel_x64::vmcs::pml_addr::set(val); }

vmcs::vmcs_field64_t
vmcs::__tsc_offset() const
{ return ::intel_x64::vmcs::tsc_offset::get(); }

void
vmcs::__set_tsc_offset(vmcs_field64_t val)
{ ::intel_x64::vmcs::tsc_offset::set(val); }

vmcs::vmcs_field64_t
vmcs::__virtual_apic_addr() const
{ return ::intel_x64::vmcs::virtual_apic_addr::get(); }

void
vmcs::__set_virtual_apic_addr(vmcs_field64_t val)
{ ::intel_x64::vmcs::virtual_apic_addr::set(val); }

vmcs::vmcs_field64_t
vmcs::__apic_access_addr() const
{ return ::intel_x64::vmcs::apic_access_addr::get(); }

void
vmcs::__set_apic_access_addr(vmcs_field64_t val)
{ ::intel_x64::vmcs::apic_access_addr::set(val); }

vmcs::vmcs_field64_t
vmcs::__posted_int_descriptor_addr() const
{ return ::intel_x64::vmcs::posted_int_descriptor_addr::get(); }

void
vmcs::__set_posted_int_descriptor_addr(vmcs_field64_t val)
{ ::intel_x64::vmcs::posted_int_descriptor_addr::set(val); }

vmcs::vmcs_field64_t
vmcs::__vm_function_ctls() const
{ return ::intel_x64::vmcs::vm_function_ctls::get(); }

void
vmcs::__set_vm_function_ctls(vmcs_field64_t val)
{ ::intel_x64::vmcs::vm_function_ctls::set(val); }

vmcs::vmcs_field64_t
vmcs::__ept_ptr() const
{ return ::intel_x64::vmcs::ept_ptr::get(); }

void
vmcs::__set_ept_ptr(vmcs_field64_t val)
{ ::intel_x64::vmcs::ept_ptr::set(val); }

vmcs::vmcs_field64_t
vmcs::__eoi_exit_bitmap_0() const
{ return ::intel_x64::vmcs::eoi_exit_bitmap_0::get(); }

void
vmcs::__set_eoi_exit_bitmap_0(vmcs_field64_t val)
{ ::intel_x64::vmcs::eoi_exit_bitmap_0::set(val); }

vmcs::vmcs_field64_t
vmcs::__eoi_exit_bitmap_1() const
{ return ::intel_x64::vmcs::eoi_exit_bitmap_1::get(); }

void
vmcs::__set_eoi_exit_bitmap_1(vmcs_field64_t val)
{ ::intel_x64::vmcs::eoi_exit_bitmap_1::set(val); }

vmcs::vmcs_field64_t
vmcs::__eoi_exit_bitmap_2() const
{ return ::intel_x64::vmcs::eoi_exit_bitmap_2::get(); }

void
vmcs::__set_eoi_exit_bitmap_2(vmcs_field64_t val)
{ ::intel_x64::vmcs::eoi_exit_bitmap_2::set(val); }

vmcs::vmcs_field64_t
vmcs::__eoi_exit_bitmap_3() const
{ return ::intel_x64::vmcs::eoi_exit_bitmap_3::get(); }

void
vmcs::__set_eoi_exit_bitmap_3(vmcs_field64_t val)
{ ::intel_x64::vmcs::eoi_exit_bitmap_3::set(val); }

vmcs::vmcs_field64_t
vmcs::__eptp_list_addr() const
{ return ::intel_x64::vmcs::eptp_list_addr::get(); }

void
vmcs::__set_eptp_list_addr(vmcs_field64_t val)
{ ::intel_x64::vmcs::eptp_list_addr::set(val); }

vmcs::vmcs_field64_t
vmcs::__vmread_bitmap_addr() const
{ return ::intel_x64::vmcs::vmread_bitmap_addr::get(); }

void
vmcs::__set_vmread_bitmap_addr(vmcs_field64_t val)
{ ::intel_x64::vmcs::vmread_bitmap_addr::set(val); }

vmcs::vmcs_field64_t
vmcs::__vmwrite_bitmap_addr() const
{ return ::intel_x64::vmcs::vmwrite_bitmap_addr::get(); }

void
vmcs::__set_vmwrite_bitmap_addr(vmcs_field64_t val)
{ ::intel_x64::vmcs::vmwrite_bitmap_addr::set(val); }

vmcs::vmcs_field64_t
vmcs::__virtualization_exception_info_addr() const
{ return ::intel_x64::vmcs::virtualization_exception_info_addr::get(); }

void
vmcs::__set_virtualization_exception_info_addr(vmcs_field64_t val)
{ ::intel_x64::vmcs::virtualization_exception_info_addr::set(val); }

vmcs::vmcs_field64_t
vmcs::__encls_exiting_bitmap() const
{ return ::intel_x64::vmcs::encls_exiting_bitmap::get(); }

void
vmcs::__set_encls_exiting_bitmap(vmcs_field64_t val)
{ ::intel_x64::vmcs::encls_exiting_bitmap::set(val); }

vmcs::vmcs_field64_t
vmcs::__xss_exiting_bitmap() const
{ return ::intel_x64::vmcs::xss_exiting_bitmap::get(); }

void
vmcs::__set_xss_exiting_bitmap(vmcs_field64_t val)
{ ::intel_x64::vmcs::xss_exiting_bitmap::set(val); }

vmcs::vmcs_field64_t
vmcs::__tsc_multiplier() const
{ return ::intel_x64::vmcs::tsc_multiplier::get(); }

void
vmcs::__set_tsc_multiplier(vmcs_field64_t val)
{ ::intel_x64::vmcs::tsc_multiplier::set(val); }

vmcs::vmcs_field64_t
vmcs::__gpa() const
{ return ::intel_x64::vmcs::guest_physical_addr::get(); }

vmcs::vmcs_field64_t
vmcs::__vmcs_link_ptr() const
{ return ::intel_x64::vmcs::vmcs_link_ptr::get(); }

void
vmcs::__set_vmcs_link_ptr(vmcs_field64_t val)
{ ::intel_x64::vmcs::vmcs_link_ptr::set(val); }

vmcs::vmcs_field64_t
vmcs::__ia32_debugctl() const
{ return ::intel_x64::vmcs::guest_ia32_debugctl::get(); }

void
vmcs::__set_ia32_debugctl(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_ia32_debugctl::set(val); }

vmcs::vmcs_field64_t
vmcs::__ia32_pat() const
{ return ::intel_x64::vmcs::guest_ia32_pat::get(); }

void
vmcs::__set_ia32_pat(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_ia32_pat::set(val); }

vmcs::vmcs_field64_t
vmcs::__ia32_efer() const
{ return ::intel_x64::vmcs::guest_ia32_efer::get(); }

void
vmcs::__set_ia32_efer(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_ia32_efer::set(val); }

vmcs::vmcs_field64_t
vmcs::__ia32_perf_global_ctrl() const
{ return ::intel_x64::vmcs::guest_ia32_perf_global_ctrl::get(); }

void
vmcs::__set_ia32_perf_global_ctrl(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_ia32_perf_global_ctrl::set(val); }

vmcs::vmcs_field64_t
vmcs::__pdpte0() const
{ return ::intel_x64::vmcs::guest_pdpte0::get(); }

void
vmcs::__set_pdpte0(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_pdpte0::set(val); }

vmcs::vmcs_field64_t
vmcs::__pdpte1() const
{ return ::intel_x64::vmcs::guest_pdpte1::get(); }

void
vmcs::__set_pdpte1(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_pdpte1::set(val); }

vmcs::vmcs_field64_t
vmcs::__pdpte2() const
{ return ::intel_x64::vmcs::guest_pdpte2::get(); }

void
vmcs::__set_pdpte2(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_pdpte2::set(val); }

vmcs::vmcs_field64_t
vmcs::__pdpte3() const
{ return ::intel_x64::vmcs::guest_pdpte3::get(); }

void
vmcs::__set_pdpte3(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_pdpte3::set(val); }

vmcs::vmcs_field64_t
vmcs::__ia32_bndcfgs() const
{ return ::intel_x64::vmcs::guest_ia32_bndcfgs::get(); }

void
vmcs::__set_ia32_bndcfgs(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_ia32_bndcfgs::set(val); }

vmcs::vmcs_field32_t
vmcs::__pin_based_vm_execution_ctls() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::pin_based_vm_execution_ctls::get()); }

void
vmcs::__set_pin_based_vm_execution_ctls(vmcs_field32_t val)
{ ::intel_x64::vmcs::pin_based_vm_execution_ctls::set(val); }

vmcs::vmcs_field32_t
vmcs::__processor_based_vm_execution_ctls() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::processor_based_vm_execution_ctls::get()); }

void
vmcs::__set_processor_based_vm_execution_ctls(vmcs_field32_t val)
{ ::intel_x64::vmcs::processor_based_vm_execution_ctls::set(val); }

vmcs::vmcs_field32_t
vmcs::__exception_bitmap() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::exception_bitmap::get()); }

void
vmcs::__set_exception_bitmap(vmcs_field32_t val)
{ ::intel_x64::vmcs::exception_bitmap::set(val); }

vmcs::vmcs_field32_t
vmcs::__page_fault_error_code_mask() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::page_fault_error_code_mask::get()); }

void
vmcs::__set_page_fault_error_code_mask(vmcs_field32_t val)
{ ::intel_x64::vmcs::page_fault_error_code_mask::set(val); }

vmcs::vmcs_field32_t
vmcs::__page_fault_error_code_match() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::page_fault_error_code_match::get()); }

void
vmcs::__set_page_fault_error_code_match(vmcs_field32_t val)
{ ::intel_x64::vmcs::page_fault_error_code_match::set(val); }

vmcs::vmcs_field32_t
vmcs::__cr3_target_count() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::cr3_target_count::get()); }

void
vmcs::__set_cr3_target_count(vmcs_field32_t val)
{ ::intel_x64::vmcs::cr3_target_count::set(val); }

vmcs::vmcs_field32_t
vmcs::__vmexit_ctls() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::vmexit_ctls::get()); }

void
vmcs::__set_vmexit_ctls(vmcs_field32_t val)
{ ::intel_x64::vmcs::vmexit_ctls::set(val); }

vmcs::vmcs_field32_t
vmcs::__vmexit_msr_store_count() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::vmexit_msr_store_count::get()); }

void
vmcs::__set_vmexit_msr_store_count(vmcs_field32_t val)
{ ::intel_x64::vmcs::vmexit_msr_store_count::set(val); }

vmcs::vmcs_field32_t
vmcs::__vmexit_msr_load_count() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::vmexit_msr_load_count::get()); }

void
vmcs::__set_vmexit_msr_load_count(vmcs_field32_t val)
{ ::intel_x64::vmcs::vmexit_msr_load_count::set(val); }

vmcs::vmcs_field32_t
vmcs::__vmentry_ctls() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::vmentry_ctls::get()); }

void
vmcs::__set_vmentry_ctls(vmcs_field32_t val)
{ ::intel_x64::vmcs::vmentry_ctls::set(val); }

vmcs::vmcs_field32_t
vmcs::__vmentry_msr_load_count() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::vmentry_msr_load_count::get()); }

void
vmcs::__set_vmentry_msr_load_count(vmcs_field32_t val)
{ ::intel_x64::vmcs::vmentry_msr_load_count::set(val); }

vmcs::vmcs_field32_t
vmcs::__vmentry_interruption_info() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::vmentry_interruption_info::get()); }

void
vmcs::__set_vmentry_interruption_info(vmcs_field32_t val)
{ ::intel_x64::vmcs::vmentry_interruption_info::set(val); }

vmcs::vmcs_field32_t
vmcs::__vmentry_exception_error_code() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::vmentry_exception_error_code::get()); }

void
vmcs::__set_vmentry_exception_error_code(vmcs_field32_t val)
{ ::intel_x64::vmcs::vmentry_exception_error_code::set(val); }

vmcs::vmcs_field32_t
vmcs::__vmentry_instr_len() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::vmentry_instr_len::get()); }

void
vmcs::__set_vmentry_instr_len(vmcs_field32_t val)
{ ::intel_x64::vmcs::vmentry_instr_len::set(val); }

vmcs::vmcs_field32_t
vmcs::__tpr_threshold() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::tpr_threshold::get()); }

void
vmcs::__set_tpr_threshold(vmcs_field32_t val)
{ ::intel_x64::vmcs::tpr_threshold::set(val); }

vmcs::vmcs_field32_t
vmcs::__processor_based_vm_execution_ctls2() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::processor_based_vm_execution_ctls2::get()); }

void
vmcs::__set_processor_based_vm_execution_ctls2(vmcs_field32_t val)
{ ::intel_x64::vmcs::processor_based_vm_execution_ctls2::set(val); }

vmcs::vmcs_field32_t
vmcs::__ple_gap() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::ple_gap::get()); }

void
vmcs::__set_ple_gap(vmcs_field32_t val)
{ ::intel_x64::vmcs::ple_gap::set(val); }

vmcs::vmcs_field32_t
vmcs::__ple_window() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::ple_window::get()); }

void
vmcs::__set_ple_window(vmcs_field32_t val)
{ ::intel_x64::vmcs::ple_window::set(val); }

vmcs::vmcs_field32_t
vmcs::__vm_instr_error() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::vm_instr_error::get()); }

vmcs::vmcs_field32_t
vmcs::__vmexit_interruption_info() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::vmexit_interruption_info::get()); }

vmcs::vmcs_field32_t
vmcs::__vmexit_interruption_error_code() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::vmexit_interruption_error_code::get()); }

vmcs::vmcs_field32_t
vmcs::__idt_vectoring_info() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::idt_vectoring_info::get()); }

vmcs::vmcs_field32_t
vmcs::__idt_vectoring_error_code() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::idt_vectoring_error_code::get()); }

vmcs::vmcs_field32_t
vmcs::__vmexit_instr_len() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::vmexit_instr_len::get()); }

vmcs::vmcs_field32_t
vmcs::__vmexit_instr_info() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::vmexit_instr_info::get()); }

vmcs::vmcs_field32_t
vmcs::__es_limit() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_es_limit::get()); }

void
vmcs::__set_es_limit(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_es_limit::set(val); }

vmcs::vmcs_field32_t
vmcs::__cs_limit() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_cs_limit::get()); }

void
vmcs::__set_cs_limit(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_cs_limit::set(val); }

vmcs::vmcs_field32_t
vmcs::__ss_limit() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_ss_limit::get()); }

void
vmcs::__set_ss_limit(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_ss_limit::set(val); }

vmcs::vmcs_field32_t
vmcs::__ds_limit() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_ds_limit::get()); }

void
vmcs::__set_ds_limit(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_ds_limit::set(val); }

vmcs::vmcs_field32_t
vmcs::__fs_limit() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_fs_limit::get()); }

void
vmcs::__set_fs_limit(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_fs_limit::set(val); }

vmcs::vmcs_field32_t
vmcs::__gs_limit() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_gs_limit::get()); }

void
vmcs::__set_gs_limit(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_gs_limit::set(val); }

vmcs::vmcs_field32_t
vmcs::__ldtr_limit() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_ldtr_limit::get()); }

void
vmcs::__set_ldtr_limit(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_ldtr_limit::set(val); }

vmcs::vmcs_field32_t
vmcs::__tr_limit() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_tr_limit::get()); }

void
vmcs::__set_tr_limit(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_tr_limit::set(val); }

vmcs::vmcs_field32_t
vmcs::__gdtr_limit() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_gdtr_limit::get()); }

void
vmcs::__set_gdtr_limit(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_gdtr_limit::set(val); }

vmcs::vmcs_field32_t
vmcs::__idtr_limit() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_idtr_limit::get()); }

void
vmcs::__set_idtr_limit(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_idtr_limit::set(val); }

vmcs::vmcs_field32_t
vmcs::__es_access_rights() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_es_access_rights::get()); }

void
vmcs::__set_es_access_rights(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_es_access_rights::set(val); }

vmcs::vmcs_field32_t
vmcs::__cs_access_rights() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_cs_access_rights::get()); }

void
vmcs::__set_cs_access_rights(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_cs_access_rights::set(val); }

vmcs::vmcs_field32_t
vmcs::__ss_access_rights() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_ss_access_rights::get()); }

void
vmcs::__set_ss_access_rights(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_ss_access_rights::set(val); }

vmcs::vmcs_field32_t
vmcs::__ds_access_rights() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_ds_access_rights::get()); }

void
vmcs::__set_ds_access_rights(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_ds_access_rights::set(val); }

vmcs::vmcs_field32_t
vmcs::__fs_access_rights() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_fs_access_rights::get()); }

void
vmcs::__set_fs_access_rights(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_fs_access_rights::set(val); }

vmcs::vmcs_field32_t
vmcs::__gs_access_rights() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_gs_access_rights::get()); }

void
vmcs::__set_gs_access_rights(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_gs_access_rights::set(val); }

vmcs::vmcs_field32_t
vmcs::__ldtr_access_rights() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_ldtr_access_rights::get()); }

void
vmcs::__set_ldtr_access_rights(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_ldtr_access_rights::set(val); }

vmcs::vmcs_field32_t
vmcs::__tr_access_rights() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_tr_access_rights::get()); }

void
vmcs::__set_tr_access_rights(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_tr_access_rights::set(val); }

vmcs::vmcs_field32_t
vmcs::__interruptibility_state() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_interruptibility_state::get()); }

void
vmcs::__set_interruptibility_state(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_interruptibility_state::set(val); }

vmcs::vmcs_field32_t
vmcs::__activity_state() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_activity_state::get()); }

void
vmcs::__set_activity_state(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_activity_state::set(val); }

vmcs::vmcs_field32_t
vmcs::__smbase() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_smbase::get()); }

void
vmcs::__set_smbase(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_smbase::set(val); }

vmcs::vmcs_field32_t
vmcs::__ia32_sysenter_cs() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::guest_ia32_sysenter_cs::get()); }

void
vmcs::__set_ia32_sysenter_cs(vmcs_field32_t val)
{ ::intel_x64::vmcs::guest_ia32_sysenter_cs::set(val); }

vmcs::vmcs_field32_t
vmcs::__preemption_timer_value() const
{ return gsl::narrow_cast<vmcs_field32_t>(::intel_x64::vmcs::preemption_timer_value::get()); }

void
vmcs::__set_preemption_timer_value(vmcs_field32_t val)
{ ::intel_x64::vmcs::preemption_timer_value::set(val); }

vmcs::vmcs_field64_t
vmcs::__cr0_guest_host_mask() const
{ return ::intel_x64::vmcs::cr0_guest_host_mask::get(); }

void
vmcs::__set_cr0_guest_host_mask(vmcs_field64_t val)
{ ::intel_x64::vmcs::cr0_guest_host_mask::set(val); }

vmcs::vmcs_field64_t
vmcs::__cr4_guest_host_mask() const
{ return ::intel_x64::vmcs::cr4_guest_host_mask::get(); }

void
vmcs::__set_cr4_guest_host_mask(vmcs_field64_t val)
{ ::intel_x64::vmcs::cr4_guest_host_mask::set(val); }

vmcs::vmcs_field64_t
vmcs::__cr0_read_shadow() const
{ return ::intel_x64::vmcs::cr0_read_shadow::get(); }

void
vmcs::__set_cr0_read_shadow(vmcs_field64_t val)
{ ::intel_x64::vmcs::cr0_read_shadow::set(val); }

vmcs::vmcs_field64_t
vmcs::__cr4_read_shadow() const
{ return ::intel_x64::vmcs::cr4_read_shadow::get(); }

void
vmcs::__set_cr4_read_shadow(vmcs_field64_t val)
{ ::intel_x64::vmcs::cr4_read_shadow::set(val); }

vmcs::vmcs_field64_t
vmcs::__cr3_target0() const
{ return ::intel_x64::vmcs::cr3_target0::get(); }

void
vmcs::__set_cr3_target0(vmcs_field64_t val)
{ ::intel_x64::vmcs::cr3_target0::set(val); }

vmcs::vmcs_field64_t
vmcs::__cr3_target1() const
{ return ::intel_x64::vmcs::cr3_target1::get(); }

void
vmcs::__set_cr3_target1(vmcs_field64_t val)
{ ::intel_x64::vmcs::cr3_target1::set(val); }

vmcs::vmcs_field64_t
vmcs::__cr3_target2() const
{ return ::intel_x64::vmcs::cr3_target2::get(); }

void
vmcs::__set_cr3_target2(vmcs_field64_t val)
{ ::intel_x64::vmcs::cr3_target2::set(val); }

vmcs::vmcs_field64_t
vmcs::__cr3_target3() const
{ return ::intel_x64::vmcs::cr3_target3::get(); }

void
vmcs::__set_cr3_target3(vmcs_field64_t val)
{ ::intel_x64::vmcs::cr3_target3::set(val); }

vmcs::vmcs_field64_t
vmcs::__exit_qualification() const
{ return ::intel_x64::vmcs::exit_qualification::get(); }

vmcs::vmcs_field64_t
vmcs::__io_rcx() const
{ return ::intel_x64::vmcs::io_rcx::get(); }

vmcs::vmcs_field64_t
vmcs::__io_rsi() const
{ return ::intel_x64::vmcs::io_rsi::get(); }

vmcs::vmcs_field64_t
vmcs::__io_rdi() const
{ return ::intel_x64::vmcs::io_rdi::get(); }

vmcs::vmcs_field64_t
vmcs::__io_rip() const
{ return ::intel_x64::vmcs::io_rip::get(); }

vmcs::vmcs_field64_t
vmcs::__gva() const
{ return ::intel_x64::vmcs::guest_linear_addr::get(); }

vmcs::vmcs_field64_t
vmcs::__cr0() const
{ return ::intel_x64::vmcs::guest_cr0::get(); }

void
vmcs::__set_cr0(vmcs_field64_t val)
{
    auto shadow_cr0 = val;
    auto actual_cr0 = val | vcpu_t_cast(this)->ia32_vmx_cr0_fixed0();

    ::intel_x64::cr0::extension_type::enable(actual_cr0);
    ::intel_x64::cr0::not_write_through::disable(actual_cr0);
    ::intel_x64::cr0::cache_disable::disable(actual_cr0);

    ::intel_x64::vmcs::guest_cr0::set(actual_cr0);
    ::intel_x64::vmcs::cr0_read_shadow::set(shadow_cr0);
}

vmcs::vmcs_field64_t
vmcs::__cr3() const
{ return ::intel_x64::vmcs::guest_cr3::get(); }

void
vmcs::__set_cr3(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_cr3::set(val & 0x7FFFFFFFFFFFFFFF); }

vmcs::vmcs_field64_t
vmcs::__cr4() const
{ return ::intel_x64::vmcs::guest_cr4::get(); }

void
vmcs::__set_cr4(vmcs_field64_t val)
{
    auto shadow_cr4 = val;
    auto actual_cr4 = val | vcpu_t_cast(this)->ia32_vmx_cr4_fixed0();

    ::intel_x64::vmcs::guest_cr4::set(actual_cr4);
    ::intel_x64::vmcs::cr4_read_shadow::set(shadow_cr4);
}

vmcs::vmcs_field64_t
vmcs::__es_base() const
{ return ::intel_x64::vmcs::guest_es_base::get(); }

void
vmcs::__set_es_base(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_es_base::set(val); }

vmcs::vmcs_field64_t
vmcs::__cs_base() const
{ return ::intel_x64::vmcs::guest_cs_base::get(); }

void
vmcs::__set_cs_base(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_cs_base::set(val); }

vmcs::vmcs_field64_t
vmcs::__ss_base() const
{ return ::intel_x64::vmcs::guest_ss_base::get(); }

void
vmcs::__set_ss_base(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_ss_base::set(val); }

vmcs::vmcs_field64_t
vmcs::__ds_base() const
{ return ::intel_x64::vmcs::guest_ds_base::get(); }

void
vmcs::__set_ds_base(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_ds_base::set(val); }

vmcs::vmcs_field64_t
vmcs::__fs_base() const
{ return ::intel_x64::vmcs::guest_fs_base::get(); }

void
vmcs::__set_fs_base(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_fs_base::set(val); }

vmcs::vmcs_field64_t
vmcs::__gs_base() const
{ return ::intel_x64::vmcs::guest_gs_base::get(); }

void
vmcs::__set_gs_base(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_gs_base::set(val); }

vmcs::vmcs_field64_t
vmcs::__ldtr_base() const
{ return ::intel_x64::vmcs::guest_ldtr_base::get(); }

void
vmcs::__set_ldtr_base(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_ldtr_base::set(val); }

vmcs::vmcs_field64_t
vmcs::__tr_base() const
{ return ::intel_x64::vmcs::guest_tr_base::get(); }

void
vmcs::__set_tr_base(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_tr_base::set(val); }

vmcs::vmcs_field64_t
vmcs::__gdtr_base() const
{ return ::intel_x64::vmcs::guest_gdtr_base::get(); }

void
vmcs::__set_gdtr_base(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_gdtr_base::set(val); }

vmcs::vmcs_field64_t
vmcs::__idtr_base() const
{ return ::intel_x64::vmcs::guest_idtr_base::get(); }

void
vmcs::__set_idtr_base(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_idtr_base::set(val); }

vmcs::vmcs_field64_t
vmcs::__dr7() const
{ return ::intel_x64::vmcs::guest_dr7::get(); }

void
vmcs::__set_dr7(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_dr7::set(val); }

vmcs::vmcs_field64_t
vmcs::__rsp() const
{ return ::intel_x64::vmcs::guest_rsp::get(); }

void
vmcs::__set_rsp(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_rsp::set(val); }

vmcs::vmcs_field64_t
vmcs::__rip() const
{ return ::intel_x64::vmcs::guest_rip::get(); }

void
vmcs::__set_rip(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_rip::set(val); }

vmcs::vmcs_field64_t
vmcs::__rflags() const
{ return ::intel_x64::vmcs::guest_rflags::get(); }

void
vmcs::__set_rflags(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_rflags::set(val); }

vmcs::vmcs_field64_t
vmcs::__pending_debug_exceptions() const
{ return ::intel_x64::vmcs::guest_pending_debug_exceptions::get(); }

void
vmcs::__set_pending_debug_exceptions(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_pending_debug_exceptions::set(val); }

vmcs::vmcs_field64_t
vmcs::__ia32_sysenter_esp() const
{ return ::intel_x64::vmcs::guest_ia32_sysenter_esp::get(); }

void
vmcs::__set_ia32_sysenter_esp(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_ia32_sysenter_esp::set(val); }

vmcs::vmcs_field64_t
vmcs::__ia32_sysenter_eip() const
{ return ::intel_x64::vmcs::guest_ia32_sysenter_eip::get(); }

void
vmcs::__set_ia32_sysenter_eip(vmcs_field64_t val)
{ ::intel_x64::vmcs::guest_ia32_sysenter_eip::set(val); }

vmcs::vmcs_field16_t
vmcs::host_es_selector() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::host_es_selector::get()); }

void
vmcs::set_host_es_selector(vmcs_field16_t val)
{ ::intel_x64::vmcs::host_es_selector::set(val); }

vmcs::vmcs_field16_t
vmcs::host_cs_selector() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::host_cs_selector::get()); }

void
vmcs::set_host_cs_selector(vmcs_field16_t val)
{ ::intel_x64::vmcs::host_cs_selector::set(val); }

vmcs::vmcs_field16_t
vmcs::host_ss_selector() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::host_ss_selector::get()); }

void
vmcs::set_host_ss_selector(vmcs_field16_t val)
{ ::intel_x64::vmcs::host_ss_selector::set(val); }

vmcs::vmcs_field16_t
vmcs::host_ds_selector() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::host_ds_selector::get()); }

void
vmcs::set_host_ds_selector(vmcs_field16_t val)
{ ::intel_x64::vmcs::host_ds_selector::set(val); }

vmcs::vmcs_field16_t
vmcs::host_fs_selector() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::host_fs_selector::get()); }

void
vmcs::set_host_fs_selector(vmcs_field16_t val)
{ ::intel_x64::vmcs::host_fs_selector::set(val); }

vmcs::vmcs_field16_t
vmcs::host_gs_selector() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::host_gs_selector::get()); }

void
vmcs::set_host_gs_selector(vmcs_field16_t val)
{ ::intel_x64::vmcs::host_gs_selector::set(val); }

vmcs::vmcs_field16_t
vmcs::host_tr_selector() const
{ return gsl::narrow_cast<vmcs_field16_t>(::intel_x64::vmcs::host_tr_selector::get()); }

void
vmcs::set_host_tr_selector(vmcs_field16_t val)
{ ::intel_x64::vmcs::host_tr_selector::set(val); }

vmcs::vmcs_field64_t
vmcs::host_ia32_pat() const
{ return ::intel_x64::vmcs::host_ia32_pat::get(); }

void
vmcs::set_host_ia32_pat(vmcs_field64_t val)
{ ::intel_x64::vmcs::host_ia32_pat::set(val); }

vmcs::vmcs_field64_t
vmcs::host_ia32_efer() const
{ return ::intel_x64::vmcs::host_ia32_efer::get(); }

void
vmcs::set_host_ia32_efer(vmcs_field64_t val)
{ ::intel_x64::vmcs::host_ia32_efer::set(val); }

vmcs::vmcs_field64_t
vmcs::host_ia32_perf_global_ctrl() const
{ return ::intel_x64::vmcs::host_ia32_perf_global_ctrl::get(); }

void
vmcs::set_host_ia32_perf_global_ctrl(vmcs_field64_t val)
{ ::intel_x64::vmcs::host_ia32_perf_global_ctrl::set(val); }

vmcs::vmcs_field64_t
vmcs::host_ia32_sysenter_cs() const
{ return ::intel_x64::vmcs::host_ia32_sysenter_cs::get(); }

void
vmcs::set_host_ia32_sysenter_cs(vmcs_field64_t val)
{ ::intel_x64::vmcs::host_ia32_sysenter_cs::set(val); }

vmcs::vmcs_field64_t
vmcs::host_cr0() const
{ return ::intel_x64::vmcs::host_cr0::get(); }

void
vmcs::set_host_cr0(vmcs_field64_t val)
{ ::intel_x64::vmcs::host_cr0::set(val); }

vmcs::vmcs_field64_t
vmcs::host_cr3() const
{ return ::intel_x64::vmcs::host_cr3::get(); }

void
vmcs::set_host_cr3(vmcs_field64_t val)
{ ::intel_x64::vmcs::host_cr3::set(val); }

vmcs::vmcs_field64_t
vmcs::host_cr4() const
{ return ::intel_x64::vmcs::host_cr4::get(); }

void
vmcs::set_host_cr4(vmcs_field64_t val)
{ ::intel_x64::vmcs::host_cr4::set(val); }

vmcs::vmcs_field64_t
vmcs::host_fs_base() const
{ return ::intel_x64::vmcs::host_fs_base::get(); }

void
vmcs::set_host_fs_base(vmcs_field64_t val)
{ ::intel_x64::vmcs::host_fs_base::set(val); }

vmcs::vmcs_field64_t
vmcs::host_gs_base() const
{ return ::intel_x64::vmcs::host_gs_base::get(); }

void
vmcs::set_host_gs_base(vmcs_field64_t val)
{ ::intel_x64::vmcs::host_gs_base::set(val); }

vmcs::vmcs_field64_t
vmcs::host_tr_base() const
{ return ::intel_x64::vmcs::host_tr_base::get(); }

void
vmcs::set_host_tr_base(vmcs_field64_t val)
{ ::intel_x64::vmcs::host_tr_base::set(val); }

vmcs::vmcs_field64_t
vmcs::host_gdtr_base() const
{ return ::intel_x64::vmcs::host_gdtr_base::get(); }

void
vmcs::set_host_gdtr_base(vmcs_field64_t val)
{ ::intel_x64::vmcs::host_gdtr_base::set(val); }

vmcs::vmcs_field64_t
vmcs::host_idtr_base() const
{ return ::intel_x64::vmcs::host_idtr_base::get(); }

void
vmcs::set_host_idtr_base(vmcs_field64_t val)
{ ::intel_x64::vmcs::host_idtr_base::set(val); }

vmcs::vmcs_field64_t
vmcs::host_ia32_sysenter_esp() const
{ return ::intel_x64::vmcs::host_ia32_sysenter_esp::get(); }

void
vmcs::set_host_ia32_sysenter_esp(vmcs_field64_t val)
{ ::intel_x64::vmcs::host_ia32_sysenter_esp::set(val); }

vmcs::vmcs_field64_t
vmcs::host_ia32_sysenter_eip() const
{ return ::intel_x64::vmcs::host_ia32_sysenter_eip::get(); }

void
vmcs::set_host_ia32_sysenter_eip(vmcs_field64_t val)
{ ::intel_x64::vmcs::host_ia32_sysenter_eip::set(val); }

vmcs::vmcs_field64_t
vmcs::host_rsp() const
{ return ::intel_x64::vmcs::host_rsp::get(); }

void
vmcs::set_host_rsp(vmcs_field64_t val)
{ ::intel_x64::vmcs::host_rsp::set(val); }

vmcs::vmcs_field64_t
vmcs::host_rip() const
{ return ::intel_x64::vmcs::host_rip::get(); }

void
vmcs::set_host_rip(vmcs_field64_t val)
{ ::intel_x64::vmcs::host_rip::set(val); }

}
