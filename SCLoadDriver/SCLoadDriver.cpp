// This PoC loads the kernel driver using the SC Manager (SCM).

#include "stdafx.h"

#define KMDF_SERVICE_NAME L"KmdfHelloWorld"
// Place the `KmdfHelloWorld.sys` file at this path before running the loader.
#define KMDF_DRIVER_PATH L"C:\\Users\\user\\Desktop\\KmdfHelloWorld.sys"

// Create and start a driver service using the SCM.
// This will fail if the current process is not running as `Administrator`. A
// real loader would be executing with sufficient privilege in userspace. The
// driver service will have to be deleted manually with `Process Hacker`.
BOOL install()
{
    SC_HANDLE scManager = NULL;
    SC_HANDLE srvHandle = NULL;
    DWORD lastError = 0;

    scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scManager) {
        printf("OpenSCManager failed: %X", GetLastError());
        return FALSE;
    }

    srvHandle = CreateService(scManager, KMDF_SERVICE_NAME, KMDF_SERVICE_NAME, SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER, SERVICE_AUTO_START /* Persistence! */, SERVICE_ERROR_NORMAL,
        KMDF_DRIVER_PATH, NULL, NULL, NULL, NULL, NULL);
    if (!srvHandle) {
        lastError = GetLastError();
        if (lastError == ERROR_SERVICE_EXISTS) {
            printf("Service already exists\n");
            srvHandle = OpenService(scManager, KMDF_SERVICE_NAME, SERVICE_ALL_ACCESS);
            if (!srvHandle) {
                printf("OpenService failed: %X\n", GetLastError());
                (void)CloseServiceHandle(scManager);
                return FALSE;
            }
        }
        else {
            printf("CreateService failed: %X\n", lastError);
            (void)CloseServiceHandle(scManager);
            return FALSE;
        }
    }

    printf("Service created!\n");

    if (!StartService(srvHandle, 0, NULL)) {
        lastError = GetLastError();
        // Driver has no enabled devices so ignore `ERROR_SERVICE_DISABLED`.
        if (lastError != ERROR_SERVICE_DISABLED) {
            printf("StartService failed: %X", lastError);
            (void)CloseServiceHandle(srvHandle);
            (void)CloseServiceHandle(scManager);
            return FALSE;
        }
    }

    printf("Service started!\n");

    (void)CloseServiceHandle(srvHandle);
    (void)CloseServiceHandle(scManager);

    return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
    (void)install();

    return 0;
}
