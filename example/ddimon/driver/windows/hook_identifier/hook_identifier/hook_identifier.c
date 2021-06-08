/*++

Module Name:

    hooki_identifier.c

Abstract:

    This driver is an extended version of the Microsoft sioctl.sys template driver.
    It supports enough of the IOCTL handler routines to make the driver work, 
    but the main functionality lies in sending hook_target addresses to the VMM.

Environment:

    Kernel mode only.

--*/

//
// Include files.
//

#include "asm.h"
#include "hook_identifier.h"

#include <ntddk.h>    // various NT definitions
#include <string.h>

#define NT_DEVICE_NAME L"\\Device\\HOOKIDNT"
#define DOS_DEVICE_NAME L"\\DosDevices\\HookIdentifier"

#if DBG
#define HOOK_IDENTIFIER_KDPRINT(_x_)                                                               \
    DbgPrint("hook_identifier.sys: ");                                                             \
    DbgPrint _x_;

#else
#define HOOK_IDENTIFIER_KDPRINT(_x_)
#endif

//
// Device driver routine declarations.
//

DRIVER_INITIALIZE DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE)
    _Dispatch_type_(IRP_MJ_CLOSE) DRIVER_DISPATCH HookidentifierCreateClose;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH HookidentifierDeviceControl;

DRIVER_UNLOAD HookidentifierUnloadDriver;

VOID PrintIrpInfo(PIRP Irp);
VOID PrintChars(_In_reads_(CountChars) PCHAR BufferAddress, _In_ size_t CountChars);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, HookidentifierCreateClose)
#pragma alloc_text(PAGE, HookidentifierDeviceControl)
#pragma alloc_text(PAGE, HookidentifierUnloadDriver)
#pragma alloc_text(PAGE, PrintIrpInfo)
#pragma alloc_text(PAGE, PrintChars)
#endif    // ALLOC_PRAGMA

NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
/*++

Routine Description:
    This routine is called by the Operating System to initialize the driver.

    It creates the device object, fills in the dispatch entry points and
    completes the initialization.

Arguments:
    DriverObject - a pointer to the object that represents this device
    driver.

    RegistryPath - a pointer to our Services key in the registry.

Return Value:
    STATUS_SUCCESS if initialized; an error otherwise.

--*/

{
    NTSTATUS ntStatus;
    UNICODE_STRING ntUnicodeString;        // NT Device Name HOOKIDNT
    UNICODE_STRING ntWin32NameString;      // Win32 Name HookIdentifier
    PDEVICE_OBJECT deviceObject = NULL;    // ptr to device object

    UNREFERENCED_PARAMETER(RegistryPath);

    RtlInitUnicodeString(&ntUnicodeString, NT_DEVICE_NAME);

    ntStatus = IoCreateDevice(
        DriverObject,               // Our Driver Object
        0,                          // We don't use a device extension
        &ntUnicodeString,           // Device name "\Device\HOOKIDNT"
        FILE_DEVICE_UNKNOWN,        // Device type
        FILE_DEVICE_SECURE_OPEN,    // Device characteristics
        FALSE,                      // Not an exclusive device
        &deviceObject);             // Returned ptr to Device Object

    if (!NT_SUCCESS(ntStatus)) {
        HOOK_IDENTIFIER_KDPRINT(("Couldn't create the device object\n"));
        return ntStatus;
    }

    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = HookidentifierCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = HookidentifierCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HookidentifierDeviceControl;
    DriverObject->DriverUnload = HookidentifierUnloadDriver;

    //
    // Initialize a Unicode String containing the Win32 name
    // for our device.
    //

    RtlInitUnicodeString(&ntWin32NameString, DOS_DEVICE_NAME);

    //
    // Create a symbolic link between our device name  and the Win32 name
    //

    ntStatus = IoCreateSymbolicLink(&ntWin32NameString, &ntUnicodeString);

    // Use a Vmcall
    //AsmVmcall((void *) 1);

    if (!NT_SUCCESS(ntStatus)) {
        //
        // Delete everything that this routine has allocated.
        //
        HOOK_IDENTIFIER_KDPRINT(("Couldn't create symbolic link\n"));
        IoDeleteDevice(deviceObject);
    }

    return ntStatus;
}

NTSTATUS
HookidentifierCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*++

Routine Description:

    This routine is called by the I/O system when the HOOK_IDENTIFIER is opened or
    closed.

    No action is performed other than completing the request successfully.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    NT status code

--*/

{
    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

VOID
HookidentifierUnloadDriver(_In_ PDRIVER_OBJECT DriverObject)
/*++

Routine Description:

    This routine is called by the I/O system to unload the driver.

    Any resources previously allocated must be freed.

Arguments:

    DriverObject - a pointer to the object that represents our driver.

Return Value:

    None
--*/

{
    PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
    UNICODE_STRING uniWin32NameString;

    PAGED_CODE();

    //
    // Create counted string version of our Win32 device name.
    //

    RtlInitUnicodeString(&uniWin32NameString, DOS_DEVICE_NAME);

    //
    // Delete the link from our device name to a name in the Win32 namespace.
    //

    IoDeleteSymbolicLink(&uniWin32NameString);

    if (deviceObject != NULL) {
        IoDeleteDevice(deviceObject);
    }
}

NTSTATUS
HookidentifierDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)

/*++

Routine Description:

    This routine is called by the I/O system to perform a device I/O
    control function.

Arguments:

    DeviceObject - a pointer to the object that represents the device
        that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    NT status code

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;    // Assume success

    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    //
    // Finish the I/O operation by simply completing the packet and returning
    // the same status as in the packet itself.
    //

    Irp->IoStatus.Status = ntStatus;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return ntStatus;
}

VOID
PrintIrpInfo(PIRP Irp)
{
    PIO_STACK_LOCATION irpSp;
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    PAGED_CODE();

    HOOK_IDENTIFIER_KDPRINT(
        ("\tIrp->AssociatedIrp.SystemBuffer = 0x%p\n", Irp->AssociatedIrp.SystemBuffer));
    HOOK_IDENTIFIER_KDPRINT(("\tIrp->UserBuffer = 0x%p\n", Irp->UserBuffer));
    HOOK_IDENTIFIER_KDPRINT(
        ("\tirpSp->Parameters.DeviceIoControl.Type3InputBuffer = 0x%p\n",
         irpSp->Parameters.DeviceIoControl.Type3InputBuffer));
    HOOK_IDENTIFIER_KDPRINT(
        ("\tirpSp->Parameters.DeviceIoControl.InputBufferLength = %d\n",
         irpSp->Parameters.DeviceIoControl.InputBufferLength));
    HOOK_IDENTIFIER_KDPRINT(
        ("\tirpSp->Parameters.DeviceIoControl.OutputBufferLength = %d\n",
         irpSp->Parameters.DeviceIoControl.OutputBufferLength));
    return;
}

VOID
PrintChars(_In_reads_(CountChars) PCHAR BufferAddress, _In_ size_t CountChars)
{
    PAGED_CODE();

    if (CountChars) {

        while (CountChars--) {

            if (*BufferAddress > 31 && *BufferAddress != 127) {

                KdPrint(("%c", *BufferAddress));
            }
            else {

                KdPrint(("."));
            }
            BufferAddress++;
        }
        KdPrint(("\n"));
    }
    return;
}
