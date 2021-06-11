#include <ntddk.h>

PVOID 
RtlPcToFileHeader( 
    PVOID PcValue, 
    PVOID *BaseOfImage);

// get_ntoskrnl_base() - get the ntoskrnl base address 
// needed to be able to enumerate the export dir of ntoskrnl pe -> scanning for the windows virtual addresses of the hook_targets

PVOID get_ntoskrnl_base();

// auto nt_base = UtilPcToFileHeader(KdDebuggerEnabled);
// if (!nt_base) {
//     return STATUS_UNSUCCESSFUL;
// }

// get_physaddr_of_exported_func(function_name, export_table)
// returns the physical address of the exported function as written in the export_table

// Enumerates all exports in a module specified by base_address.
// _Use_decl_annotations_ EXTERN_C static NTSTATUS DdimonpEnumExportedSymbols(
//     ULONG_PTR base_address, EnumExportedSymbolsCallbackType callback,
//     void* context) {
//   PAGED_CODE();

//   auto dos = reinterpret_cast<PIMAGE_DOS_HEADER>(base_address);
//   auto nt = reinterpret_cast<PIMAGE_NT_HEADERS>(base_address + dos->e_lfanew);
//   auto dir = reinterpret_cast<PIMAGE_DATA_DIRECTORY>(
//       &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]);
//   if (!dir->Size || !dir->VirtualAddress) {
//     return STATUS_SUCCESS;
//   }

//   auto dir_base = base_address + dir->VirtualAddress;
//   auto dir_end = base_address + dir->VirtualAddress + dir->Size - 1;
//   auto exp_dir = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(base_address +
//                                                            dir->VirtualAddress);
//   for (auto i = 0ul; i < exp_dir->NumberOfNames; i++) {
//     if (!callback(i, base_address, exp_dir, dir_base, dir_end, context)) {
//       return STATUS_SUCCESS;
//     }
//   }
//   return STATUS_SUCCESS;



// // convert the name to UNICODE_STRING
//   wchar_t name[100];
//   auto status =
//       RtlStringCchPrintfW(name, RTL_NUMBER_OF(name), L"%S", export_name);
//   if (!NT_SUCCESS(status)) {
//     return true;
//   }
//   UNICODE_STRING name_u = {};
//   RtlInitUnicodeString(&name_u, name);

//   for (auto& target : g_ddimonp_hook_targets) {
//     // Is this export listed as a target
//     if (!FsRtlIsNameInExpression(&target.target_name, &name_u, TRUE, nullptr)) {
//       continue;
//     }

//     // Yes, install a hook to the export
//     if (!ShInstallHook(reinterpret_cast<SharedShadowHookData*>(context),
//                        reinterpret_cast<void*>(export_address), &target)) {
//       // This is an error which should not happen
//       DdimonpFreeAllocatedTrampolineRegions();
//       return false;
//     }
//     HYPERPLATFORM_LOG_INFO("Hook has been installed at %016Ix %s.",
//                            export_address, export_name);
//   }
//   return true;
// }