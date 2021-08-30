# roscos
Visual Studio 2019 Compiled Source Directories of Windows Render/Compute-Only-Sample Drivers (ROS/COS)

![rosisalive](https://github.com/themindvirus/roscos/blob/main/rosisalive.png)

## Cautionary Note
The drivers listed in this repository have been attenuated so that they no longer function but are stable enough to install as legacy drivers through Device Manager.

They may be used as the building blocks for the Raspberry Pi Windows 10 DirectX/Direct3D Render and Graphics Drivers and they enumerate as pictured above.

Please refrain from installing if you are unfamiliar with their potentially undiscovered flaws and side-effects.

If the installation isn't working, you may need to recompile the drivers with Visual Studio 2019 WDK for Windows 10 SDK 19041. The following Registry Keys are also worth checking:

```
Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\GraphicsDrivers]
"DisableVersionMismatchCheck"=dword:1

[HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Services\RenderOnlySample]
; And all subkeys

[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\DirectX]
; And all subkeys

[HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\DirectX]
; And all subkeys

[HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Enum\ROOT\BasicRender\0000]
; And all subkeys (Informational)
```
