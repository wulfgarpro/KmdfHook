// An example KMDF driver that writes a file from kernel space to
// "C:\proof.txt" on load. See `NTLoadDriver` and `SCLoadDriver` loaders on how
// to deploy.

#include <ntddk.h>
#include <wdf.h>

DRIVER_INITIALIZE DriverEntry;

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
        DbgPrint("ZwCreateFile error: %X", status);
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

    DbgPrint("KmvdfHelloWorld: DriverEntry\n");

    // Write proof file!
    status = WriteFile();

    config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
    WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);

    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config,
        WDF_NO_HANDLE);

    return status;
}
