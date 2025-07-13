/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_SYSTEM_INFO_C
#define COMS_PLATFORM_WIN32_SYSTEM_INFO_C

#include <stdio.h>
#include <stdint.h>
#include "../../stdlib/Types.h"
#include "../../utils/StringUtils.h"
#include "../../system/SystemInfo.h"
#include "../../architecture/CpuInfo.cpp"

#include <psapi.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wbemidl.h>
#include <comdef.h>
#include <winnls.h>
#include <wingdi.h>
#include <hidsdi.h>
#include <setupapi.h>
#include <cfgmgr32.h>

// @performance Do we really need all these libs, can't we simplify that?!
// At least we should dynamically load them, this way the application won't crash if the lib doesn't exist
#include <intrin.h>
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")
#pragma comment(lib, "comsuppw.lib")

uint64 system_private_memory_usage()
{
    PROCESS_MEMORY_COUNTERS_EX pmc;
    HANDLE process = GetCurrentProcess();

    GetProcessMemoryInfo(process, (PROCESS_MEMORY_COUNTERS *) &pmc, sizeof(pmc));

    CloseHandle(process);

    return pmc.PrivateUsage;
}

uint64 system_app_memory_usage()
{
    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T address = 0;
    size_t total_size = 0;

    // MEM_IMAGE = DLL memory
    // MEM_MAPPED = Mapped files
    while (VirtualQueryEx(GetCurrentProcess(), (LPCVOID) address, &mbi, sizeof(mbi)) == sizeof(mbi)) {
        if (mbi.State == MEM_COMMIT && (mbi.Type == MEM_IMAGE || mbi.Type == MEM_MAPPED)) {
            total_size += mbi.RegionSize;
        }

        address += mbi.RegionSize;
    }

    return total_size;
}

uint16 system_language_code()
{
    LANGID lang_id = GetUserDefaultUILanguage();
    wchar_t local_name[LOCALE_NAME_MAX_LENGTH];

    if (!LCIDToLocaleName(lang_id, local_name, LOCALE_NAME_MAX_LENGTH, 0)) {
        return 0;
    }

    return (local_name[0] << 8) | local_name[1];
}

uint16 system_country_code()
{
    LANGID lang_id = GetUserDefaultUILanguage();
    wchar_t local_name[LOCALE_NAME_MAX_LENGTH];

    if (!LCIDToLocaleName(lang_id, local_name, LOCALE_NAME_MAX_LENGTH, 0)) {
        return 0;
    }

    return (local_name[3] << 8) | local_name[4];
}

void mainboard_info_get(MainboardInfo* info) {
    info->name[sizeof(info->name) - 1] = '\0';
    info->serial_number[sizeof(info->serial_number) - 1] = '\0';

    HRESULT hres;

    // Step 1: Initialize COM library
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        return;
    }

    // Step 2: Set general COM security levels
    hres = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL
    );

    if (FAILED(hres)) {
        CoUninitialize();
        return;
    }

    // Step 3: Obtain initial locator to WMI
    IWbemLocator *pLoc = NULL;
    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID *)&pLoc
    );

    if (FAILED(hres)) {
        CoUninitialize();
        return;
    }

    // Step 4: Connect to WMI through IWbemLocator::ConnectServer
    IWbemServices *pSvc = NULL;
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL,
        NULL,
        0,
        NULL,
        0,
        0,
        &pSvc
    );

    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();
        return;
    }

    // Step 5: Set security levels on the proxy
    hres = CoSetProxyBlanket(
        pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE
    );

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return;
    }

    // Step 6: Use the IWbemServices pointer to make a WMI query
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_BaseBoard"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );

    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return;
    }

    // Step 7: Retrieve the data
    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
    while (pEnumerator) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (0 == uReturn) {
            break;
        }

        VARIANT vtProp;
        hr = pclsObj->Get(L"Product", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr)) {
            wchar_to_char(vtProp.bstrVal);
            sprintf_fast(info->name, sizeof(info->name), "%s", vtProp.bstrVal);
            VariantClear(&vtProp);
        }

        hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr)) {
            wchar_to_char(vtProp.bstrVal);
            sprintf_fast(info->serial_number, sizeof(info->serial_number), "%s", vtProp.bstrVal);
            VariantClear(&vtProp);
        }

        pclsObj->Release();
    }

    // Clean up
    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();

    info->name[sizeof(info->name) - 1] = '\0';
    info->serial_number[sizeof(info->serial_number) - 1] = '\0';
}

int32 network_info_get(NetworkInfo* info) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return 0;
    }

    DWORD dwSize = 0;
    PIP_ADAPTER_ADDRESSES pAdapterAddresses = NULL;
    PIP_ADAPTER_ADDRESSES pAdapter = NULL;

    // Get the size of the adapter addresses buffer
    if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &dwSize) == ERROR_BUFFER_OVERFLOW) {
        // @todo Remove malloc
        pAdapterAddresses = (PIP_ADAPTER_ADDRESSES) malloc(dwSize);
        if (pAdapterAddresses == NULL) {
            WSACleanup();
            return 0;
        }
    } else {
        WSACleanup();
        return 0;
    }

    // Get the adapter addresses
    if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAdapterAddresses, &dwSize) != NO_ERROR) {
        free(pAdapterAddresses);
        WSACleanup();
        return 0;
    }

    int32 i = 0;

    // Iterate over the adapters and print their MAC addresses
    pAdapter = pAdapterAddresses;
    while (pAdapter && i < 4) {
        if (pAdapter->PhysicalAddressLength != 0) {
            info[i].slot[63] = '\0';
            info[i].mac[23] = '\0';

            memcpy(info[i].mac, pAdapter->PhysicalAddress, 8);
            wcstombs(info[i].slot, pAdapter->FriendlyName, 63);

            ++i;
        }

        pAdapter = pAdapter->Next;
    }

    free(pAdapterAddresses);
    WSACleanup();

    return i;
}

void cpu_info_get(CpuInfo* info) {
    info->features = cpu_info_features();

    cpu_info_cache(1, &info->cache[0]);
    cpu_info_cache(2, &info->cache[1]);
    cpu_info_cache(3, &info->cache[2]);
    cpu_info_cache(4, &info->cache[3]);

    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    info->core_count = (uint16) sys_info.dwNumberOfProcessors;
    info->page_size = (uint16) sys_info.dwPageSize;

    int32 cpuInfo[4] = { 0 };
    __cpuid(cpuInfo, 0);

    memset(info->vendor, 0, sizeof(info->vendor));
    *((int32 *) info->vendor) = cpuInfo[1];
    *((int32 *) (info->vendor + 4)) = cpuInfo[3];
    *((int32 *) (info->vendor + 8)) = cpuInfo[2];
    info->vendor[12] = '\0';

    __cpuid(cpuInfo, 0x80000002);
    memcpy(info->brand, cpuInfo, sizeof(cpuInfo));
    __cpuid(cpuInfo, 0x80000003);
    memcpy(info->brand + 16, cpuInfo, sizeof(cpuInfo));
    __cpuid(cpuInfo, 0x80000004);
    memcpy(info->brand + 32, cpuInfo, sizeof(cpuInfo));
    info->brand[48] = '\0';

    __cpuid(cpuInfo, 1);
    info->model = (cpuInfo[0] >> 4) & 0xF;
    info->family = (cpuInfo[0] >> 8) & 0xF;

    DWORD bufSize = sizeof(DWORD);
    HKEY hKey;
    long lError = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0, KEY_READ, &hKey
    );

    if (lError == ERROR_SUCCESS) {
        RegQueryValueExA(hKey, "~MHz", NULL, NULL, (LPBYTE) &(info->mhz), &bufSize);
    }

    RegCloseKey(hKey);
}

void os_info_get(OSInfo* info) {
    info->vendor[15] = '\0';
    info->name[63] = '\0';

    OSVERSIONINFOEXW version_info;
    memset(&version_info, 0, sizeof(OSVERSIONINFOEXW));
    version_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    NTSTATUS(WINAPI *RtlGetVersion)(OSVERSIONINFOEXW*) = (NTSTATUS(WINAPI *)(OSVERSIONINFOEXW*))GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
    if (RtlGetVersion != nullptr) {
        RtlGetVersion(&version_info);
    }

    memcpy(info->vendor, "Microsoft", sizeof("Microsoft"));
    memcpy(info->name, "Windows", sizeof("Windows"));
    info->major = version_info.dwMajorVersion;
    info->minor = version_info.dwMinorVersion;
}

void ram_info_get(RamInfo* info) {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    info->memory = (uint32) (statex.ullTotalPhys / (1024 * 1024));
}

RamChannelType ram_channel_info() {
    HRESULT hres;
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        return RAM_CHANNEL_TYPE_FAILED;
    }

    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hres)) {
        CoUninitialize();

        return RAM_CHANNEL_TYPE_FAILED;
    }

    IWbemLocator *pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);
    if (FAILED(hres)) {
        CoUninitialize();

        return RAM_CHANNEL_TYPE_FAILED;
    }

    IWbemServices *pSvc = NULL;
    hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres)) {
        pLoc->Release();
        CoUninitialize();

        return RAM_CHANNEL_TYPE_FAILED;
    }

    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();

        return RAM_CHANNEL_TYPE_FAILED;
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_PhysicalMemory"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hres)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();

        return RAM_CHANNEL_TYPE_FAILED;
    }

    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
    int32 ram_module_count = 0;
    int32 dual_channel_capable = 0;

    while (pEnumerator) {
        hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (uReturn == 0) break;

        VARIANT vtProp;
        hres = pclsObj->Get(L"BankLabel", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hres)) {
            ++ram_module_count;
            if (wcscmp(vtProp.bstrVal, L"BANK 0") == 0 || wcscmp(vtProp.bstrVal, L"BANK 1") == 0) {
                dual_channel_capable = 1;
            }

            VariantClear(&vtProp);
        }
        pclsObj->Release();
    }

    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

    if (ram_module_count == 1) {
        return RAM_CHANNEL_TYPE_SINGLE_CHANNEL;
    } else if (ram_module_count == 2 && dual_channel_capable) {
        return RAM_CHANNEL_TYPE_DUAL_CHANNEL;
    } else if (ram_module_count == 2 && !dual_channel_capable) {
        return RAM_CHANNEL_TYPE_CAN_UPGRADE;
    } else {
        return RAM_CHANNEL_TYPE_FAILED;
    }
}

uint32 gpu_info_get(GpuInfo* info) {
    IDXGIFactory *pFactory = NULL;
    IDXGIAdapter *pAdapter = NULL;
    DXGI_ADAPTER_DESC adapterDesc;

    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
    if (FAILED(hr)) {
        return 0;
    }

    uint32 i = 0;
    while (pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND && i < 3) {
        hr = pAdapter->GetDesc(&adapterDesc);
        if (FAILED(hr)) {
            pAdapter->Release();
            break;
        }

        wcstombs(info[i].name, adapterDesc.Description, 63);
        info[i].name[63] = '\0';
        info[i].vram = (uint32) (adapterDesc.DedicatedVideoMemory / (1024 * 1024));

        pAdapter->Release();
        ++i;
    }

    pFactory->Release();

    return i;
}

uint32 display_info_get(DisplayInfo* info) {
    DISPLAY_DEVICEA device;
    DEVMODEA mode;

    device.cb = sizeof(DISPLAY_DEVICEA);

    uint32 i = 0;

    while (EnumDisplayDevicesA(NULL, i, &device, 0)) {
        mode.dmSize = sizeof(mode);

        if (EnumDisplaySettingsA(device.DeviceName, ENUM_CURRENT_SETTINGS, &mode)) {
            str_copy_short(info[i].name, device.DeviceName);
            info[i].width = mode.dmPelsWidth;
            info[i].height = mode.dmPelsHeight;
            info[i].hz = mode.dmDisplayFrequency;
            info[i].is_primary = (bool) (device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE);
        }

        ++i;
    }

    return i;
}

bool is_dedicated_gpu_connected() {
    DISPLAY_DEVICEA displayDevice;
    displayDevice.cb = sizeof(DISPLAY_DEVICEA);
    for (int32 i = 0; EnumDisplayDevicesA(NULL, i, &displayDevice, 0); ++i) {
        if (displayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
            DISPLAY_DEVICEA gpuDevice;
            gpuDevice.cb = sizeof(DISPLAY_DEVICEA);
            if (EnumDisplayDevicesA(displayDevice.DeviceName, 0, &gpuDevice, 0)) {
                if (gpuDevice.DeviceID
                    && (str_contains(gpuDevice.DeviceID, "PCI\\VEN_10DE") // Nvidia
                        || str_contains(gpuDevice.DeviceID, "PCI\\VEN_1002") // AMD
                        || str_contains(gpuDevice.DeviceID, "PCI\\VEN_8086") // Intel
                    )
                ) {
                    return true;
                }
            }
        }
    }

    return false;
}

#endif