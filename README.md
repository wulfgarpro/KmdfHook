# KmdfHook

KmdfHook is a persistent Windows kernel-mode driver built using the Kernel-Mode
Driver Framework (KMDF). It features system call hooking with `ZwCreateFile` and
demonstrates two common driver loading techniques.

## Components

- **KmdfHelloWorld Driver**: Depending on the `HOOK` define in "Driver.c", this
  KMDF driver either creates "C:\hook_proof.txt" or "C:\proof.txt".
- **KmdfHelloWorld Package**: The full package required for deploying the KMDF
  driver.
- **NTLoadDriver**: A loader that utilizes the `NtLoadDriver` function for
  stealthier driver deployment.
- **SCLoadDriver**: A loader that employs the Service Control Manager (SCM) for
  driver deployment, which is less stealthy but uses standard Windows APIs.

## Motivation

KmdfHook serves as an educational tool for studying the deployment and operation
of Windows rootkits. By simulating the persistence mechanisms typically used by
rootkits, this driver ensures it is reloaded on system reboot and demonstrates
essential rootkit behaviors like system call hooking.

## Techniques

### System Call Hook

This technique involves modifying the System Service Descriptor Table (SSDT) to
intercept calls to `ZwCreateFile`, achieved by disabling the CR0 register's
Write Protect (WP) flag.

### NtLoadDriver

Employs the `NTAPI` function `NtLoadDriver` to load a kernel driver discreetly:

1. Enable `SeLoadDriverPrivilege` for the process.
2. Create a service registry key with necessary subkeys, setting `Start` to
   `SERVICE_AUTO_START`.
3. Deploy the driver by calling `NtLoadDriver` with the service registry key
   path.

### SC Manager

Utilises the Service Control API for a more overt method of loading kernel
drivers:

1. Create a service with `dwStartType` set to `SERVICE_AUTO_START` and add it to
the SCM database.
2. Start the service to deploy the driver.

## Build and Deployment Requirements

- **Development Tools**: Visual Studio 2013 with the Windows SDK and Driver Kit
  8.1.
- **Deployment**: Follow the
  [Assumptions & Constraints](#Assumptions & Constraints) section closely.
  Transfer "Win8.1[Debug|Release]\KmdfHelloWorld.sys" to the Desktop and update
  `KMDF_DRIVER_PATH` as needed in `NTLoadDriver` and `SCLoadDriver`.

## Scripts

- **install_driver.bat**: Script to install the driver using `SCLoadDriver`.
- **delete_driver.bat**: Script to remove the driver service using `sc.exe`.

## Cleanup

To uninstall the driver service, use `Process Hacker` to delete "KmdfHelloWorld"
from the Services tab, or execute the included `delete_driver.bat` script.
