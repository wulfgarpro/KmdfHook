// An example KMDF driver that writes a file from kernel space to
// "C:\proof.txt" on load. See `NTLoadDriver` and `SCLoadDriver` loaders on how
// to deploy. If `HOOK` is enabled, the driver will hook the `ZwCreateFile`
// syscall and the call by `WriteFile` to `CreateFile` will interdict
// "C:\proof.txt" and instead write "C:\hook_proof.txt".

#include <ntddk.h>
#include <wdf.h>

#define HOOK 1  // Change to 0 to disable `ZwCreateFile` hook.

extern NTSTATUS doHook();
extern NTSTATUS doUnHook();

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD DriverUnload;

NTSTATUS WriteFile()
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING filePath = { 0 };
    OBJECT_ATTRIBUTES objectAttributes = { 0 };
    HANDLE hFile = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    RtlInitUnicodeString(&filePath, L"\\??\\C:\\proof.txt");

    InitializeObjectAttributes(&objectAttributes, &filePath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwCreateFile(&hFile, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &objectAttributes,
        &ioStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_CREATE, FILE_SYNCHRONOUS_IO_NONALERT,
        NULL, 0);
    if (!NT_SUCCESS(status)) {
        (void)DbgPrint("ZwCreateFile error: %X", status);
        return status;
    }

    (void)ZwClose(hFile);

    return status;
}

// Entry point for all drivers.
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG config = { 0 };

    (void)DbgPrint("KmvdfHelloWorld: DriverEntry\n");

#if HOOK
    // Hook a syscall!
    (void)doHook();
#endif

    // Write proof file!
    (void)WriteFile();

    WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);
    config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
    config.EvtDriverUnload = DriverUnload;

    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config,
        WDF_NO_HANDLE);

    (void)DbgPrint("KmvdfHelloWorld: DriverEntry returns status code %X\n", status);

    return status;
}

void DriverUnload(_In_ WDFDRIVER DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);

#if HOOK
    // Unhook a syscall!
    (void)doUnHook();
#endif

    (void)DbgPrint("Driver unloaded");
};