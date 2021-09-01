# roscos
Visual Studio 2019 Rebased Source Directories of Windows Render-Only-Sample Driver (ROS)

https://github.com/TheMindVirus/roscos/blob/3f76cd6df9943bfe37826ead81aa43e9384dbbad/rosdriver/RosDriver.vcxproj#L144
```
<!--FIX - FilesToPackage is invalid for DriverType Package
  <ItemGroup>
    <FilesToPackage Include="$(TargetPath)" />
  </ItemGroup>
-->
```

```
RosUmd was loaded. (hmod = 0x00007FFA83DF0000)
RosUmd was loaded. (hmod = 0x00007FFA8A290000)
RosUmd was loaded. (hmod = 0x00007FFA8A290000)
```
