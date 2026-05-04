OpenXR Vertical Tangent
This is an OpenXR layer that reduces the vertical field of view by adjusting projection tangents. It removes rendering above and below the useful view to improve performance in sim racing setups.
Based on:


Matthieu Bucchianeri (OpenXR Toolkit architecture)


fommil (OpenXR Widescreen implementation)



Install
Download:
https://github.com/Cooooked/openxr-verticaltangent/releases
Run the .msi.
Uninstall via Windows “Add or Remove Programs”.

Config
Create this file:
C:\Program Files\XR-VertTangent\XR-VertTangent.ini
Example:
[Settings]verticalTangent = 0.8
Lower value = more vertical cropping (more performance, less FOV).

How it works


Runs as an OpenXR implicit layer


Intercepts projection tangents


Reduces vertical rendering range


Applied per supported OpenXR app (tested mainly in iRacing)



Build (dev)
Requires:


Visual Studio (C++ workload)


NuGet


WiX Toolset


Outputs:


OpenXR layer DLL


JSON manifest


MSI installer



Credits


Matthieu Bucchianeri — OpenXR Toolkit foundation


fommil — widescreen OpenXR layer fork


Khronos OpenXR specification
