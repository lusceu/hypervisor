/// @file
/// Declares interface to "vmcall" assembly function.


/// Executes vmcall 
/// @return Equivalent to 0
/// To start simple, just one vmcall is executed (with a target function hook address as param), 
/// so the VMM-extension just handles this single type of vmcall. 
/// In case the driver needs to send more than one type of "message" to the VMM, implement sth like 
/// unsigned char __stdcall AsmVmcall(_In_ ULONG_PTR hypercall_number, _In_opt_ void *context);

void __stdcall AsmVmcall(void *hook_target);
