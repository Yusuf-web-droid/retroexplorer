# Retro Explorer

A small native browser shell for Windows 98. It uses the WebBrowser ActiveX
control supplied by Internet Explorer, rather than a modern runtime that would
not run on the operating system.

The program provides Back, Next, Stop, Reload, Home, and an editable address
bar. Type an address and press Enter or **Go**. Addresses without a scheme are
treated as `http://` URLs.

## Requirements

- Windows 98 or Windows 98 SE
- Internet Explorer 4 or newer installed; Internet Explorer 6 SP1 is the
  final Internet Explorer release that supports Windows 98
- Visual C++ 6.0 SP6, or a 32-bit MinGW toolchain, to build it

No modern framework, manifest, or unsupported Windows API is required. The
source explicitly targets Windows 98 (`WINVER` and `_WIN32_WINNT` are `0x0410`).

## Build

On a Windows 98-era development system with Visual C++ 6:

```bat
nmake /f Makefile.msvc
```

Or from a current machine with a 32-bit MinGW cross-compiler:

```bat
mingw32-make -f Makefile.mingw
```

Copy `retroexplorer.exe` to the Windows 98 machine. It is a 32-bit desktop
program. Launching it with a URL as the command-line argument opens that URL.

## Setup package

A self-extracting browser setup is included as `retroexplorer-setup.exe`. On a
Windows system, running it will unpack the browser files to a selected folder and
launch `retroexplorer.exe` automatically.

## Important limitation

This is compatible with Windows 98, but it inherits Internet Explorer's
historical web engine and TLS stack. It is suitable for old sites, plain HTTP,
local HTML, and intranet pages—not for current HTTPS sites or security-sensitive
browsing. Do not use it for passwords, banking, or any private data. Accessing
modern web pages requires a separately configured HTTPS-rendering proxy; that
proxy is intentionally not bundled here.
