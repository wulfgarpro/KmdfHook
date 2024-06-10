# KmdfHook

<<<<<<< HEAD
KmdfHook is a persistent Windows kernel-mode driver built using the kernel-mode
Driver Framework (KMDF). It features system call hooking with `ZwCreateFile` and
demonstrates two common driver loading techniques.

## Components

* KmdfHelloWorld Driver: Depending on the HOOK define in "Driver.c", this KMDF
  driver either creates "C:\hook_proof.txt" or "C:\proof.txt".
* KmdfHelloWorld Package: The full package required for deploying the KMDF
  driver.
* NTLoadDriver: A loader that utilizes the NtLoadDriver function for stealthier
  driver deployment.
* SCLoadDriver: A loader that employs the Service Control Manager (SCM) for
  driver deployment, which is less stealthy but uses standard Windows APIs.
=======
An example persistent Windows kernel-mode driver (KMDF) that hooks the
`ZwCreateFile` system call, and two common loading techniques.

## Components

* KmdfHelloWorld: The KMDF driver that drops a file to "C:\hook_proof.txt"
  if the `HOOK` define in "Driver.c" is enabled, otherwise "C:\proof.txt".
* KmdfHelloWorld Package: The KMDF driver package.
* NTLoadDriver: An example loader using `NtLoadDriver`.
* SCLoadDriver: An example loader using the Service Control Manager (SCM).

## Background

A Windows rootkit is deployed by a staged loader whose goal is to drop a
Windows kernel-mode driver to disk and have the kernel load it. The technique
to load the driver should establish persistence, ensuring that the kernel will
load the driver on reboot. With the driver loaded in kernel space, the driver
can perform operations fundamental to a rootkit, e.g. system call hooking.
>>>>>>> f9d0219 (Added ZwCreateFile system call hook)

## Motivation

KmdfHook serves as an educational tool for studying the deployment and operation
of Windows rootkits. By simulating the persistence mechanisms typically used by
rootkits, this driver ensures it is reloaded on system reboot and demonstrates
essential rootkit behaviors like system call hooking.

## Techniques

### System Call Hook

<<<<<<< HEAD
This technique involves modifying the System Service Descriptor Table (SSDT) to
intercept calls to `ZwCreateFile`, achieved by disabling the CR0 register's
Write Protect (WP) flag.

### NtLoadDriver

Employs the NTAPI function `NtLoadDriver` to load a kernel driver discreetly:
=======
The `ZwCreateFile` system call hook overwrites the System Service Description
Table's (SSDT) entry after disabling the CR0 register's Write Protect (WP)
flag.

### NtLoadDriver

Uses the `NTAPI` function `NtLoadDriver` to manually load a kernel driver from
disk with increased stealth.
>>>>>>> f9d0219 (Added ZwCreateFile system call hook)

1. Enable `SeLoadDriverPrivilege` for the process.
2. Create a service registry key with necessary subkeys, setting Start to
   `SERVICE_AUTO_START`.
3. Deploy the driver by calling `NtLoadDriver` with the service registry key
   path.

### SC Manager

<<<<<<< HEAD
Utilizes the Service Control API for a more overt method of loading kernel drivers:
=======
Uses the Service Control API to load a kernel driver from disk with the SCM.
Although this technique is the official mechanism for loading kernel device
drivers, it is noisy.
>>>>>>> f9d0219 (Added ZwCreateFile system call hook)

1. Create a service with `dwStartType` set to `SERVICE_AUTO_START` and add it to the
   SCM database.
2. Start the service to deploy the driver.

## Assumptions & Constraints

* Target platform: Windows 7 32-bit Enterprise SP1 (Build 7601), chosen for its
  relevance in historical rootkit analysis.

<<<<<<< HEAD
* Assumes the driver is pre-installed on disk, as detailed in deployment
  instructions.
=======
Both loaders assume the kernel-mode driver is on disk as described in the
deployment instructions.

The project does not include a privilege escalation vector and so assumes
sufficient privilege, and so `SCLoadDriver` should be executed on target from
an `Administrator` `cmd.exe` and `NTLoadDriver` from a `NT AUTHORITY\SYSTEM`
`cmd.exe` started with `psexec`.
>>>>>>> f9d0219 (Added ZwCreateFile system call hook)

* Privilege level: Requires execution from an administrative or SYSTEM-level
  command prompt.

* Assumes Driver Signature Enforcement (DSE) is disabled:

```cmd
bcdedit.exe -set loadoptions DDISABLE_INTEGRITY_CHECKS
bcdedit.exe -set TESTSIGNING ON
```

* The driver service must be manually removed between loader executions using
  `Process Hacker`.

## Build and Deployment Requirements

* Development Tools: Visual Studio 2013 with the Windows SDK and Driver Kit 8.1.
* Deployment: Follow the [Assumptions & Constraints](#Assumptions & Constraints)
  section closely. Transfer "Win8.1[Debug|Release]\KmdfHelloWorld.sys" to the
  Desktop and update KMDF_DRIVER_PATH as needed in NTLoadDriver and
  SCLoadDriver.

## Scripts

<<<<<<< HEAD
* **install_driver.bat**: Script to install the driver using `SCLoadDriver`.
* **delete_driver.bat**: Script to remove the driver service using `sc.exe`.

## Cleanup

To uninstall the driver service, use `Process Hacker` to delete "KmdfHelloWorld"
from the Services tab, or execute the included `delete_driver.bat` script.
=======
## Deployment

Read [Assumptions & Constraints](#Assumptions & Constraints) before proceeding.

Copy "Win8.1[Debug|Release]\KmdfHelloWorld.sys" to the user's Desktop. If
`KMDF_DRIVER_PATH` in `NTLoadDriver` and `SCLoadDriver` is wrong, update it as
appropriate.

By default the `HOOK` define in "Driver.c" is enabled. If disabled the
`ZwCreateFile` system call will not be hooked.

Execute `NTLoadDriver` or `SCLoadDriver` and confirm "C:\hook_proof.txt" exists
if `HOOK` is enabled, otherwise "C:\proof.txt".

## Scripts

* "scripts\install_driver.bat": Example script that installs the driver with
`SCLoadDriver`.
* "scripts\delete_driver.bat": Remove driver with `sc.exe`.

## Cleanup

To remove the driver service, right click "KmdfHelloWorld" in `Process Hacker`'s
"Services" tab and select "Delete" or use `sc.exe` as demonstrated by the
included "scripts\delete_driver.bat" script.
>>>>>>> f9d0219 (Added ZwCreateFile system call hook)
