// Hooks `ZwCreateFile` with custom function that replaces the original
// "C:\proof.txt" filename with "C:\hook_proof.txt".

#include <ntddk.h>

#define DllImport __declspec(dllimport)

// The System Service Table (SST) structure to apply to the
// `nt!KeServiceDescriptorTable` memory region.
typedef struct SystemServiceTable {
    UINT32* 	ServiceTable; // System Service Dispatch Table (SSDT).
    UINT32* 	CounterTable;
    UINT32		ServiceLimit;
    UINT32*     ArgumentTable;
} SST;

// `ZwCreateFile` function prototype.
typedef NTSTATUS(*ZwCreateFilePrototype)(
    _Out_          PHANDLE            FileHandle,
    _In_           ACCESS_MASK        DesiredAccess,
    _In_           POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_          PIO_STATUS_BLOCK   IoStatusBlock,
    _In_opt_       PLARGE_INTEGER     AllocationSize,
    _In_           ULONG              FileAttributes,
    _In_           ULONG              ShareAccess,
    _In_           ULONG              CreateDisposition,
    _In_           ULONG              CreateOptions,
    _In_opt_       PVOID              EaBuffer,
    _In_           ULONG              EaLength
);

// The `KeServiceDescriptorTable` symbol is exported by `ntoskrnl.exe`, the NT
// OS Kernel image. Instruct the loader to lookup the symbol dynamically.
DllImport SST KeServiceDescriptorTable;

// Pointer to the original `ZwCreateFile` so that we can call it to achieve
// the real functionality within our hook, and restore it as required.
ZwCreateFilePrototype oldZwCreateFile = NULL;

// Set the Write Protect (WP) flag (bit 16) to 0 in CR0. When unset,
// kernel-mode processes can write into read-only pages regardless of their
// R/W and U/S flags.
void disableWP() {
    _asm {
        push edx;                   // Save the current edx.
        mov edx, cr0;               // Put the current cr0 in edx.
        and edx, 0xFFFEFFFF;        // Mask WP flag to 0.
        mov cr0, edx;               // Put the current edx in cr0.
        pop edx;                    // Restore edx.
    }
}

// Set the WP flag to 1 in CR0. Reverses `disableWP()`.
void enableWP() {
    _asm {
        push edx;
        mov edx, cr0;
        or edx, 0x00010000;
        mov cr0, edx;
        pop edx;
    }
}

// The hook function with the same signature as `ZwCreateFile`. The address
// of the hook function will replace the SSDT entry for `ZwCreateFile` so that
// a subsequent call to `ZwCreateFile` will instead call the hook function.
NTSTATUS Hook_ZwCreateFile(
    _Out_          PHANDLE            FileHandle,
    _In_           ACCESS_MASK        DesiredAccess,
    _In_           POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_          PIO_STATUS_BLOCK   IoStatusBlock,
    _In_opt_       PLARGE_INTEGER     AllocationSize,
    _In_           ULONG              FileAttributes,
    _In_           ULONG              ShareAccess,
    _In_           ULONG              CreateDisposition,
    _In_           ULONG              CreateOptions,
    _In_opt_       PVOID              EaBuffer,
    _In_           ULONG              EaLength
)
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING matchedFile = { 0 };
    UNICODE_STRING badFile = { 0 };
    
    RtlInitUnicodeString(&matchedFile, L"\\??\\C:\\proof.txt");

    // If this call to `ZwCreateFile` is for "C:\proof.txt", replace it with a
    // "C:\hook_proof.txt" file.
    if (RtlEqualUnicodeString(ObjectAttributes->ObjectName,  &matchedFile, TRUE)) {
        (void)DbgPrint("Hook_ZwCreateFile: replacing proof file with hook proof file.");
        RtlInitUnicodeString(&badFile, L"\\??\\C:\\hook_proof.txt");
        ObjectAttributes->ObjectName = &badFile;
    }

    // Call the original `ZwCreateFile`.
    status = oldZwCreateFile(
        FileHandle,
        DesiredAccess,
        ObjectAttributes,
        IoStatusBlock,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        EaBuffer,
        EaLength);

    return status;
}

// Overwite the `ZwCreateFile` SSDT entry with the hook function
// `hookAddr`.
NTSTATUS hookZwCreateFile(ULONG hookAddr) {
    UINT32 index;
    UINT32* ssdt;
    UINT32* target;

    // `ssdt` should match the KiServiceTable address reported by
    // `dps nt!KeServiceDescriptorTable l1`.
    ssdt = KeServiceDescriptorTable.ServiceTable;
    (void)DbgPrint("The address of the SSDT is: %X.\r\n", ssdt);

#pragma warning(push)
#pragma warning(disable: 4054)  // Cast function pointer to data type `PUCHAR`.
#pragma warning(disable: 4047)  // Cast function pointer to `ULONG`.
    // Identify the `ZwCreateFile` syscall number in the SSDT. A syscall such
    // as `ZwCreateFile` is structured such that the syscall number is stored
    // in the first byte at the address of the system call. By adding 0x1 to
    // the system call pointer, we're actually accessing the pointer to the
    // address of the syscall number. To read the syscall number, we
    // dereference the pointer. Since the SSDT is indexed by syscall number,
    // we name the variable `index` since it will be used to index into the
    // SSDT at the location that we must override with the address of the
    // hook function.

    // Cast address of `ZwCreateFile` to `PUCHAR` to add `sizeof(UCHAR)` to the
    // address, i.e. 1 byte, and then cast back to `PUINT32` and dereference to
    // get syscall number as `UINT32`.
    index = *((PUINT32)((PUCHAR)&ZwCreateFile + 0x1));
    (void)DbgPrint("The syscall number index is: %X\n", index);

    // Lookup the address of the SSDT entry to overwrite using the syscall
    // number index. The address should match `dps nt!KiServiceTable l0x43`
    // on Windows 7 since `ZwCreateFile` is syscall number 44 on Windows 7.
    target = &(ssdt[index]);
    (void)DbgPrint("The address of the SSDT routine entry to overwite is: %X\n", target);

    // Atomically set the target SSDT entry to the address of
    // `hookAddr`. A 64-bit PoC would need to use
    // `InterlockedExchange64`. Cast address of original `ZwCreateFile` to
    // `ULONG` is safe since a function pointer address on a 32-bit system can
    // fit in `ULONG`.
    oldZwCreateFile = (ULONG)InterlockedExchange((PLONG)target, hookAddr);
    (void)DbgPrint("Hook complete... oldZwCreateFile is: %X", oldZwCreateFile);

    return STATUS_SUCCESS;
}

// Overwite the `ZwCreateFile` SSDT entry with the hook function
// `Hook_ZwCreateFile`.
NTSTATUS doHook() {
    disableWP();
    (void)DbgPrint("WP has been disabled");

    // Cast address of `Hook_ZwCreateFile` to `ULONG` is safe since a function
    // pointer address on a 32-bit system can fit in `ULONG`. A 64-bit PoC
    // would have to cast to `ULONGLONG`.
    (void)hookZwCreateFile((ULONG)&Hook_ZwCreateFile);

    return STATUS_SUCCESS;
}

// Restore the original `ZwCreateFile` SSDT entry.
NTSTATUS doUnHook() {
    if (oldZwCreateFile != NULL) {
        // Cast address of `oldZwCreateFile` to `LONG` is safe since a function
        // pointer address on a 32-bit system can fit in `ULONG`. A 64-bit
        // PoC would have to cast to `ULONGLONG`.
        (void)hookZwCreateFile((ULONG)oldZwCreateFile);
    }

    enableWP();
    (void)DbgPrint("WP has been enabled");

    return STATUS_SUCCESS;
}
