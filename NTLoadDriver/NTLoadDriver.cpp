// This PoC loads the `KmdfHelloWorld` KMDF driver using the undocumented
// ntdll.dll call `NtLoadDriver`.

#include "stdafx.h"

#pragma comment(lib, "ntdll.lib")

#define STATUS_PLUGPLAY_NO_DEVICE 0xC000025E
// Place the `KmdfHelloWorld.sys` file at this path before running this loader.
#define KMDF_DRIVER_PATH L"C:\\Users\\user\\Desktop\\KmdfHelloWorld.sys"

NTSTATUS(NTAPI *NtLoadDriver)(IN PUNICODE_STRING DriverServiceName);

// Enable `SE_LOAD_DRIVER_NAME` privilege for the current process.
// This will fail if the current process is not running as
// `NT AUTHORITY\SYSTEM`. A real loader would be executing from the kernel,
// e.g., via a patched vulnerable OEM driver.
BOOL setPrivilege()
{
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tp = { 0 };
    LUID luid = { 0 };

    if (!LookupPrivilegeValue(NULL, SE_LOAD_DRIVER_NAME, &luid)) {
        printf("LookupPrivilegeValue error: %X", GetLastError());
        return FALSE;
    }

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
        printf("OpenProcessToken error: %X", GetLastError());
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        printf("AdjustTokenPrivileges error: %X", GetLastError());
        (void)CloseHandle(hToken);
        return FALSE;
    }

    (void)CloseHandle(hToken);

    return TRUE;
}

// Setup registry driver service entry.
// The service entry is the `DriverServiceName` passed to `NtLoadDriver`. The
// registry entry must be cleaned up manually with `regedit`.
BOOL addRegEntry()
{
    HKEY phkResult = { 0 };
    DWORD lpdwDisposition = 0;
    WCHAR regPath[MAX_PATH] = L"System\\CurrentControlSet\\Services\\KmdfHelloWorld";
    SIZE_T imagePathSize = 0;
    DWORD keyValue = 0;
    LSTATUS createKey = ERROR_SUCCESS;

    if (!RegCreateKeyEx(HKEY_LOCAL_MACHINE, regPath, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &phkResult,
        &lpdwDisposition) == ERROR_SUCCESS) {
        printf("RegCreateKeyEx error: %X", GetLastError());
        return FALSE;
    }

    // TODO(imoldove): Below needs better error handling.
    // TODO(imoldove): Below needs better formatting.
    WCHAR driverPath[MAX_PATH] = { 0 };
    _snwprintf_s(driverPath, MAX_PATH, _TRUNCATE, L"%ws%ws", L"\\??\\", KMDF_DRIVER_PATH);

    imagePathSize = (DWORD)(sizeof(wchar_t) * (wcslen(driverPath) + 1));
    (void)RegSetValueEx(phkResult, L"ImagePath", 0, REG_EXPAND_SZ, (const BYTE*)driverPath,
        imagePathSize);
    keyValue = SERVICE_KERNEL_DRIVER;
    (void)RegSetValueEx(phkResult, L"Type", 0, REG_DWORD, (const BYTE*)&keyValue, sizeof(DWORD));
    keyValue = SERVICE_ERROR_IGNORE; // Don't be noisy.
    (void)RegSetValueEx(phkResult, L"ErrorControl", 0, REG_DWORD, (const BYTE*)&keyValue,
        sizeof(DWORD));
    keyValue = SERVICE_AUTO_START; // Persistence!
    (void)RegSetValueEx(phkResult, L"Start", 0, REG_DWORD, (const BYTE*)&keyValue, sizeof(DWORD));

    (void)RegCloseKey(phkResult);

    return TRUE;
}

BOOL loadDriver()
{
    HMODULE hNtdll = NULL;
    ANSI_STRING key = { 0 };
    UNICODE_STRING driver = { 0 };

    if (!setPrivilege()) {
        return FALSE;
    }

    if (!addRegEntry()) {
        return FALSE;
    }

    hNtdll = GetModuleHandle(L"ntdll.dll");
    if (!hNtdll) {
        printf("GetModuleHandleW error: %X", GetLastError());
        return FALSE;
    }

    *(FARPROC*)&NtLoadDriver = GetProcAddress(hNtdll, "NtLoadDriver");
    if (!NtLoadDriver) {
        printf("GetProcAddress error: %X", GetLastError());
        (void)CloseHandle(hNtdll);
        return FALSE;
    }

    RtlInitAnsiString(&key,
        "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\KmdfHelloWorld");
    (void)RtlAnsiStringToUnicodeString(&driver, &key, TRUE);

    NTSTATUS status = NtLoadDriver(&driver);
    if (!NT_SUCCESS(status)) {
        // Our driver has no enabled devices so we ignore
        // `STATUS_PLUGPLAY_NO_DEVICE`.
        if (status != STATUS_PLUGPLAY_NO_DEVICE) {
            printf("NtLoadDriver error: %X", status);
            (void)RtlFreeUnicodeString(&driver);
            (void)CloseHandle(hNtdll);
            return FALSE;
        }
    }

    (void)RtlFreeUnicodeString(&driver);
    (void)CloseHandle(hNtdll);

    return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
    (void)loadDriver();

    return 0;
}
