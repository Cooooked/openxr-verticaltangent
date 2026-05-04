// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "pch.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <filesystem>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef _DEBUG
#include <random>
#include <sstream>
#endif

namespace {
    const std::string LayerName = "XR_APILAYER_cooooked_verttangent";

    std::string dllHome;

    std::ofstream logStream;

    const std::array<const char*, 1> allowedApps = {
        "iRacingSim64DX11"
    };
    bool enabled = false;

    double verticalMultiplier = 0.2;

    PFN_xrGetInstanceProcAddr nextXrGetInstanceProcAddr = nullptr;
    PFN_xrLocateViews nextXrLocateViews = nullptr;
    PFN_xrEnumerateViewConfigurationViews nextXrEnumerateViewConfigurationViews = nullptr;

    void Log(const char* fmt, ...);

    void InternalLog(const char* fmt, va_list va)
    {
        char buf[1024];
        _vsnprintf_s(buf, sizeof(buf), fmt, va);
        OutputDebugStringA(buf);
        if (logStream.is_open())
        {
            logStream << buf;
            logStream.flush();
        }
    }

    void Log(const char* fmt, ...)
    {
        va_list va;
        va_start(va, fmt);
        InternalLog(fmt, va);
        va_end(va);
    }

    void DebugLog(const char* fmt, ...)
    {
#ifdef _DEBUG
        va_list va;
        va_start(va, fmt);
        InternalLog(fmt, va);
        va_end(va);
#endif
    }

    static void ClampVerticalFov(XrFovf& fov)
    {
        fov.angleUp = static_cast<float>(fov.angleUp * verticalMultiplier);
        fov.angleDown = static_cast<float>(fov.angleDown * verticalMultiplier);
    }

    XRAPI_ATTR XrResult XRAPI_CALL cooooked_verttangent_xrEnumerateViewConfigurationViews(
        XrInstance               instance,
        XrSystemId               systemId,
        XrViewConfigurationType  viewConfigurationType,
        uint32_t                 viewCapacityInput,
        uint32_t* viewCountOutput,
        XrViewConfigurationView* views)
    {
        if (enabled) {
            DebugLog("--> cooooked_verttangent_xrEnumerateViewConfigurationViews\n");
        }

        XrResult res = nextXrEnumerateViewConfigurationViews(instance, systemId,
            viewConfigurationType, viewCapacityInput,
            viewCountOutput, views);
        if (!enabled) {
            return res;
        }

        if (res != XR_SUCCESS || views == nullptr) {
            DebugLog("<-- cooooked_verttangent_xrEnumerateViewConfigurationViews EARLY %d\n", res);
            return res;
        }

        PFN_xrGetViewConfigurationProperties pfnGetProps = nullptr;
        XrResult props_result = nextXrGetInstanceProcAddr(instance, "xrGetViewConfigurationProperties", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetProps));
        if (props_result == XR_SUCCESS) {
            DebugLog("  --> got props ref\n");
            XrViewConfigurationProperties props{ XR_TYPE_VIEW_CONFIGURATION_PROPERTIES };
            if (pfnGetProps(instance, systemId, viewConfigurationType, &props) == XR_SUCCESS &&
                props.fovMutable == XR_TRUE) {
                DebugLog("  --> got props, and fovMutable is true\n");
                for (uint32_t i = 0; i < *viewCountOutput; ++i) {
                    const uint32_t w = views[i].recommendedImageRectWidth;
                    const uint32_t h = views[i].recommendedImageRectHeight;
                    DebugLog("  --> res for %u is %ux%u\n", i, w, h);
                    const uint32_t newH = static_cast<uint32_t>(
                        std::lround(static_cast<double>(h) * verticalMultiplier));
                    DebugLog("  --> Res clamp: view %u  %ux%u -> %ux%u\n",
                        i, w, h, w, newH);
                    views[i].recommendedImageRectHeight = std::max<uint32_t>(1u, newH);
                }
            }
        }

        DebugLog("<-- cooooked_verttangent_xrEnumerateViewConfigurationViews %d\n", res);

        return res;
    }

    XrResult cooooked_verttangent_xrLocateViews(
        const XrSession session,
        const XrViewLocateInfo* const viewLocateInfo,
        XrViewState* const viewState,
        const uint32_t viewCapacityInput,
        uint32_t* const viewCountOutput,
        XrView* const views)
    {
        if (enabled) {
            DebugLog("--> cooooked_verttangent_xrLocateViews\n");
        }

        XrResult result = nextXrLocateViews(session, viewLocateInfo, viewState, viewCapacityInput, viewCountOutput, views);
        if (!enabled) {
            return result;
        }

        if (result == XR_SUCCESS && viewLocateInfo->viewConfigurationType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO)
        {
            for (uint32_t i = 0; i < *viewCountOutput; ++i)
                ClampVerticalFov(views[i].fov);
        }

        DebugLog("<-- cooooked_verttangent_xrLocateViews %d\n", result);

        return result;
    }

    XrResult cooooked_verttangent_xrGetInstanceProcAddr(
        const XrInstance instance,
        const char* const name,
        PFN_xrVoidFunction* const function)
    {
        DebugLog("--> cooooked_verttangent_xrGetInstanceProcAddr \"%s\"\n", name);
        XrResult res = nextXrGetInstanceProcAddr(instance, name, function);

        if (strcmp(name, "xrLocateViews") == 0) {
            nextXrLocateViews = reinterpret_cast<PFN_xrLocateViews>(*function);
            *function = reinterpret_cast<PFN_xrVoidFunction>(
                cooooked_verttangent_xrLocateViews);
        }
        else if (strcmp(name, "xrEnumerateViewConfigurationViews") == 0) {
            nextXrEnumerateViewConfigurationViews =
                reinterpret_cast<PFN_xrEnumerateViewConfigurationViews>(*function);
            *function = reinterpret_cast<PFN_xrVoidFunction>(
                cooooked_verttangent_xrEnumerateViewConfigurationViews);
        }

        DebugLog("<-- cooooked_verttangent_xrGetInstanceProcAddr %d\n", res);
        return res;
    }

    XrResult cooooked_verttangent_xrCreateApiLayerInstance(
        const XrInstanceCreateInfo* const instanceCreateInfo,
        const struct XrApiLayerCreateInfo* const apiLayerInfo,
        XrInstance* const instance)
    {
        DebugLog("--> cooooked_verttangent_xrCreateApiLayerInstance\n");

        if (!apiLayerInfo ||
            apiLayerInfo->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_CREATE_INFO ||
            apiLayerInfo->structVersion != XR_API_LAYER_CREATE_INFO_STRUCT_VERSION ||
            apiLayerInfo->structSize != sizeof(XrApiLayerCreateInfo) ||
            !apiLayerInfo->nextInfo ||
            apiLayerInfo->nextInfo->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_NEXT_INFO ||
            apiLayerInfo->nextInfo->structVersion != XR_API_LAYER_NEXT_INFO_STRUCT_VERSION ||
            apiLayerInfo->nextInfo->structSize != sizeof(XrApiLayerNextInfo) ||
            apiLayerInfo->nextInfo->layerName != LayerName ||
            !apiLayerInfo->nextInfo->nextGetInstanceProcAddr ||
            !apiLayerInfo->nextInfo->nextCreateApiLayerInstance)
        {
            Log("xrCreateApiLayerInstance validation failed\n");
            return XR_ERROR_INITIALIZATION_FAILED;
        }

        nextXrGetInstanceProcAddr = apiLayerInfo->nextInfo->nextGetInstanceProcAddr;

        XrApiLayerCreateInfo chainApiLayerInfo = *apiLayerInfo;
        chainApiLayerInfo.nextInfo = apiLayerInfo->nextInfo->next;
        const XrResult result = apiLayerInfo->nextInfo->nextCreateApiLayerInstance(instanceCreateInfo, &chainApiLayerInfo, instance);

        const char* appName = instanceCreateInfo->applicationInfo.applicationName;
        for (const auto& allowed : allowedApps) {
            if (std::strstr(appName, allowed)) {
                enabled = true;
                break;
            }
        }
        Log("%s for \"%s\"\n", enabled ? "ENABLED" : "DISABLED", appName);

        DebugLog("<-- cooooked_verttangent_xrCreateApiLayerInstance %d\n", result);
        return result;
    }

}

extern "C" {

    XrResult __declspec(dllexport) XRAPI_CALL cooooked_verttangent_xrNegotiateLoaderApiLayerInterface(
        const XrNegotiateLoaderInfo* const loaderInfo,
        const char* const apiLayerName,
        XrNegotiateApiLayerRequest* const apiLayerRequest)
    {
        DebugLog("--> (early) cooooked_verttangent_xrNegotiateLoaderApiLayerInterface\n");

        if (dllHome.empty())
        {
            HMODULE module;
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&dllHome, &module))
            {
                char path[_MAX_PATH];
                GetModuleFileNameA(module, path, sizeof(path));
                dllHome = std::filesystem::path(path).parent_path().string();
            }
            else
            {
                DebugLog("Failed to locate DLL\n");
            }
        }

        if (!logStream.is_open())
        {
#ifdef _DEBUG
            std::mt19937 rng{ std::random_device{}() };
            std::uniform_int_distribution<uint32_t> dist;
            std::stringstream ss;
            ss << LayerName << "_" << std::hex << dist(rng) << ".log";
            std::string logFile = (std::filesystem::path(dllHome) / ss.str()).string();
#else
            std::string logFile = (std::filesystem::path(dllHome) / "XR-VertTangent.log").string();
#endif
            logStream.open(logFile, std::ios_base::ate);
            Log("dllHome is \"%s\"\n", dllHome.c_str());
        }

        std::filesystem::path config = std::filesystem::path(dllHome) / "XR-VertTangent.ini";
        if (std::filesystem::exists(config)) {
            wchar_t buffer[64];
            GetPrivateProfileStringW(L"Settings", L"vertical_multiplier", L"", buffer, 64, config.c_str());
            try {
                double val = std::stod(buffer);
                if (val > 0 && val <= 1.0) {
                    verticalMultiplier = val;
                }
            }
            catch (...) {}
        }
        Log("vertical multiplier is %f\n", verticalMultiplier);

        DebugLog("--> cooooked_verttangent_xrNegotiateLoaderApiLayerInterface\n");

        if (apiLayerName && apiLayerName != LayerName)
        {
            Log("Invalid apiLayerName \"%s\"\n", apiLayerName);
            return XR_ERROR_INITIALIZATION_FAILED;
        }

        if (!loaderInfo ||
            !apiLayerRequest ||
            loaderInfo->structType != XR_LOADER_INTERFACE_STRUCT_LOADER_INFO ||
            loaderInfo->structVersion != XR_LOADER_INFO_STRUCT_VERSION ||
            loaderInfo->structSize != sizeof(XrNegotiateLoaderInfo) ||
            apiLayerRequest->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_REQUEST ||
            apiLayerRequest->structVersion != XR_API_LAYER_INFO_STRUCT_VERSION ||
            apiLayerRequest->structSize != sizeof(XrNegotiateApiLayerRequest) ||
            loaderInfo->minInterfaceVersion > XR_CURRENT_LOADER_API_LAYER_VERSION ||
            loaderInfo->maxInterfaceVersion < XR_CURRENT_LOADER_API_LAYER_VERSION ||
            loaderInfo->maxInterfaceVersion > XR_CURRENT_LOADER_API_LAYER_VERSION ||
            loaderInfo->maxApiVersion < XR_CURRENT_API_VERSION ||
            loaderInfo->minApiVersion > XR_CURRENT_API_VERSION)
        {
            Log("xrNegotiateLoaderApiLayerInterface validation failed\n");
            return XR_ERROR_INITIALIZATION_FAILED;
        }

        apiLayerRequest->layerInterfaceVersion = XR_CURRENT_LOADER_API_LAYER_VERSION;
        apiLayerRequest->layerApiVersion = XR_CURRENT_API_VERSION;
        apiLayerRequest->getInstanceProcAddr = reinterpret_cast<PFN_xrGetInstanceProcAddr>(cooooked_verttangent_xrGetInstanceProcAddr);
        apiLayerRequest->createApiLayerInstance = reinterpret_cast<PFN_xrCreateApiLayerInstance>(cooooked_verttangent_xrCreateApiLayerInstance);

        DebugLog("<-- cooooked_verttangent_xrNegotiateLoaderApiLayerInterface\n");

        Log("%s layer is active\n", LayerName.c_str());

        return XR_SUCCESS;
    }

}