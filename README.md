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

## If You Get Stuck
You may find yourself in the situation where whenever you uninstall the driver you encounter a BSOD. Don't worry, this is normal during testing.
First Power Off your Raspberry Pi and plug in the storage media into your Driver Development PC.

Then Install and make use of Dism++, a free utility to manage Drivers on an offline installation of Windows: \
https://www.majorgeeks.com/files/details/dism.html (be careful not to remove essential drivers, only Display Adaptors -> Barthouse -> roskmd.sys)

Also Install and make use of Unlocker, a free utility to manage immovable files on an offline installation of Windows: \
https://www.majorgeeks.com/files/details/unlocker.html (use with caution, only use it to force delete X:\Windows\System32\drivers\roskmd.sys)

Then safely eject your storage media and plug it back into the Raspberry Pi and reboot.
Once Windows loads again, open Device Manager -> View -> Show Hidden Devices (Checked) and look in Display Adaptors.
If "Render Only Sample Device" appears with an exclamation mark icon, uninstall it and optionally reboot.

You are now ready to repeat the process should this happen again.
Try to avoid the root causes of such BSOD's involving trying to free objects and pointers which have not been initialised.
Pointers which have not been initialised may not always be NULL. They can be a variety of actually valid values (especially 0xCCCCCCCC on 32-bit systems and 0xCDCDCDCDCDCDCDCD on 64-bit systems). Don't worry, this is most likely not your fault, it is a vaguely known compiler issue for detecting bad memory conditions.

## Quick Links
OpenAdapter10_2: https://github.com/TheMindVirus/roscos/blob/805f8cfaaf866edf38cf19b2498fd5f1a8d49ff3/render-only-sample/rosumd/RosUmdAdapter.cpp#L244

new RosUmdDevice: https://github.com/TheMindVirus/roscos/blob/805f8cfaaf866edf38cf19b2498fd5f1a8d49ff3/render-only-sample/rosumd/RosUmdAdapter.cpp#L85 \
m_pWDDM1_3DeviceFuncs: https://github.com/TheMindVirus/roscos/blob/805f8cfaaf866edf38cf19b2498fd5f1a8d49ff3/render-only-sample/rosumd/RosUmdDevice.cpp#L98 \
s_deviceFuncsWDDM1_3: https://github.com/TheMindVirus/roscos/blob/805f8cfaaf866edf38cf19b2498fd5f1a8d49ff3/render-only-sample/rosumd/RosUmdDeviceDdi.cpp#L26

new CosUmdDevice: https://github.com/TheMindVirus/roscos/blob/805f8cfaaf866edf38cf19b2498fd5f1a8d49ff3/compute-only-sample/cosumd12/CosUmd12Adapter.cpp#L64 \
CosUmd12Device::Standup: https://github.com/TheMindVirus/roscos/blob/805f8cfaaf866edf38cf19b2498fd5f1a8d49ff3/compute-only-sample/cosumd12/CosUmd12Device.cpp#L38 \
m_pUMCallbacks: https://github.com/TheMindVirus/roscos/blob/805f8cfaaf866edf38cf19b2498fd5f1a8d49ff3/compute-only-sample/cosumd12/CosUmd12Device.h#L75

debug(n/w): https://github.com/TheMindVirus/roscos/blob/805f8cfaaf866edf38cf19b2498fd5f1a8d49ff3/render-only-sample/rosumd/precomp.h#L23
