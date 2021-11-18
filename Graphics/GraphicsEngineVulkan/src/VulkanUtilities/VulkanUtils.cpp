
#include "VulkanUtilities/VulkanUtils.hpp"
#include <algorithm>

namespace VulkanUtilities
{

    PFN_vkGetInstanceProcAddr DiligentGetInstanceProcAddr = nullptr;

    VulkanInterface diligent_vk_interface = VulkanInterface();

#define ACQUIRE_PROC(name, instance, device) \
    fFunctions.f##name = reinterpret_cast<PFN_vk##name>(getProc("vk" #name, instance, device))

#define ACQUIRE_PROC_SUFFIX(name, suffix, instance, device) \
    fFunctions.f##name##suffix =                                    \
        reinterpret_cast<PFN_vk##name##suffix>(getProc("vk" #name #suffix, instance, device))

// finds the index of ext in infos or a negative result if ext is not found.
static int find_info(const std::vector<VulkanExtensions::Info>& infos, const char* ext)
{
    if (infos.empty())
    {
        return -1;
    }
  
    std::string extensionStr(ext);
    auto it = infos.begin();

    for (size_t i = 0; i < infos.size(); ++i){
        if (it->fName.compare(extensionStr) == 0)
            return i;
        ++it;
    }
    return -1;
}


void VulkanExtensions::init(DiligentGetProc getProc, VkInstance instance, VkPhysicalDevice physDev, uint32_t instanceExtensionCount, const char* const* instanceExtensions, uint32_t deviceExtensionCount, const char* const* deviceExtensions)
{
    for (uint32_t i = 0; i < instanceExtensionCount; ++i)
    {
        const char* extension = instanceExtensions[i];
        // if not already in the list, add it
        if (find_info(fExtensions, extension) < 0){
            fExtensions.push_back(VulkanExtensions::Info(extension));
        }
    }

    for (uint32_t i = 0; i < deviceExtensionCount; ++i)
    {
        const char* extension = deviceExtensions[i];
        // if not already in the list, add it
        if (find_info(fExtensions, extension) < 0){
            fExtensions.push_back(VulkanExtensions::Info(extension));
        }
    }

    this->getSpecVersions(getProc, instance, physDev);
};

bool VulkanExtensions::hasExtension(const char* ext, uint32_t minVersion) const
{
    int idx = find_info(fExtensions, ext);
    return idx >= 0 && fExtensions[idx].fSpecVersion >= minVersion;
};

#define GET_PROC(F, inst) \
    PFN_vk##F grVk##F = (PFN_vk##F)getProc("vk" #F, inst, VK_NULL_HANDLE)

void VulkanExtensions::getSpecVersions(DiligentGetProc getProc, VkInstance instance, VkPhysicalDevice physDevice)
{
    // We grab all the extensions for the VkInstance and VkDevice so we can look up what spec
    // version each of the supported extensions are. We do not grab the extensions for layers
    // because we don't know what layers the client has enabled and in general we don't do anything
    // special for those extensions.

    if (instance == VK_NULL_HANDLE)
        return;

    GET_PROC(EnumerateInstanceExtensionProperties, VK_NULL_HANDLE);
    if(grVkEnumerateInstanceExtensionProperties!=nullptr)
        return;

    VkResult res;
    // instance extensions
    uint32_t extensionCount = 0;
    res = grVkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    if (VK_SUCCESS != res)
    {
        return;
    }
    VkExtensionProperties* extensions = new VkExtensionProperties[extensionCount];
    res = grVkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);
    if (VK_SUCCESS != res)
    {
        delete[] extensions;
        return;
    }
    for (uint32_t i = 0; i < extensionCount; ++i)
    {
        int idx = find_info(fExtensions, extensions[i].extensionName);
        if (idx >= 0)
        {
            fExtensions[idx].fSpecVersion = extensions[i].specVersion;
        }
    }
    delete[] extensions;

    if (physDevice == VK_NULL_HANDLE)
    {
        return;
    }
    GET_PROC(EnumerateDeviceExtensionProperties, instance);

    // device extensions
    extensionCount = 0;
    res            = grVkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, nullptr);
    if (VK_SUCCESS != res)
    {
        return;
    }
    extensions = new VkExtensionProperties[extensionCount];
    res        = grVkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, extensions);
    if (VK_SUCCESS != res)
    {
        delete[] extensions;
        return;
    }
    for (uint32_t i = 0; i < extensionCount; ++i)
    {
        int idx = find_info(fExtensions, extensions[i].extensionName);
        if (idx >= 0)
        {
            fExtensions[idx].fSpecVersion = extensions[i].specVersion;
        }
    }
    delete[] extensions;
};


void VulkanInterface::loadGlobalFunctions(DiligentGetProc getProc)
{

#if defined(VK_VERSION_1_0)
    ACQUIRE_PROC(CreateInstance, VK_NULL_HANDLE, VK_NULL_HANDLE);
    ACQUIRE_PROC(EnumerateInstanceExtensionProperties, VK_NULL_HANDLE, VK_NULL_HANDLE);
    ACQUIRE_PROC(EnumerateInstanceLayerProperties, VK_NULL_HANDLE, VK_NULL_HANDLE);
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
    ACQUIRE_PROC(EnumerateInstanceVersion, VK_NULL_HANDLE, VK_NULL_HANDLE);
#endif /* defined(VK_VERSION_1_1) */
}

bool VulkanInterface::validateGlobalFunctions(uint32_t instanceVersion){
    if (instanceVersion >= VK_VERSION_1_0)
    {
        if (nullptr == fFunctions.fCreateInstance ||
            nullptr == fFunctions.fEnumerateInstanceExtensionProperties ||
            nullptr == fFunctions.fEnumerateInstanceLayerProperties)
            return false;
    };

    if (instanceVersion >= VK_VERSION_1_1)
    {
        if (nullptr == fFunctions.fEnumerateInstanceVersion)
            return false;
    };

    return true;
};

void VulkanInterface::loadInstanceFunctions(DiligentGetProc getProc, VkInstance instance)
{
#if defined(VK_VERSION_1_0)
    ACQUIRE_PROC(CreateDevice, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(DestroyInstance, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(EnumerateDeviceExtensionProperties, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(EnumerateDeviceLayerProperties, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(EnumeratePhysicalDevices, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(GetDeviceProcAddr, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(GetPhysicalDeviceFeatures, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(GetPhysicalDeviceFormatProperties, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(GetPhysicalDeviceImageFormatProperties, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(GetPhysicalDeviceQueueFamilyProperties, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(GetPhysicalDeviceSparseImageFormatProperties, instance, VK_NULL_HANDLE);
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
    ACQUIRE_PROC(EnumeratePhysicalDeviceGroups, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(GetPhysicalDeviceExternalBufferProperties, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(GetPhysicalDeviceExternalFenceProperties, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(GetPhysicalDeviceExternalSemaphoreProperties, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(GetPhysicalDeviceFeatures2, instance, VK_NULL_HANDLE);

    ACQUIRE_PROC(GetPhysicalDeviceFormatProperties2, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(GetPhysicalDeviceImageFormatProperties2, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(GetPhysicalDeviceMemoryProperties2, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(GetPhysicalDeviceProperties2, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC(GetPhysicalDeviceQueueFamilyProperties2, instance, VK_NULL_HANDLE);

    ACQUIRE_PROC(GetPhysicalDeviceSparseImageFormatProperties2, instance, VK_NULL_HANDLE);
#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_EXT_acquire_drm_display)
    ACQUIRE_PROC_SUFFIX(AcquireDrmDisplay,EXT, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetDrmDisplay,EXT, instance, VK_NULL_HANDLE);
#endif /* defined(VK_EXT_acquire_drm_display) */
#if defined(VK_EXT_acquire_xlib_display)
    ACQUIRE_PROC_SUFFIX(AcquireXlibDisplay, EXT, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetRandROutputDisplay, EXT, instance, VK_NULL_HANDLE);
#endif /* defined(VK_EXT_acquire_xlib_display) */
#if defined(VK_EXT_calibrated_timestamps)
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceCalibrateableTimeDomains, EXT, instance, VK_NULL_HANDLE);
#endif /* defined(VK_EXT_calibrated_timestamps) */
#if defined(VK_EXT_debug_report)
    ACQUIRE_PROC_SUFFIX(CreateDebugReportCallback, EXT, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(DebugReportMessage, EXT, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(DestroyDebugReportCallback, EXT, instance, VK_NULL_HANDLE);
#endif /* defined(VK_EXT_debug_report) */
#if defined(VK_EXT_debug_utils)
    ACQUIRE_PROC_SUFFIX(CmdBeginDebugUtilsLabel, EXT, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(CmdEndDebugUtilsLabel, EXT, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(CmdInsertDebugUtilsLabel, EXT, instance, VK_NULL_HANDLE);

    ACQUIRE_PROC_SUFFIX(CreateDebugUtilsMessenger, EXT, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(DestroyDebugUtilsMessenger, EXT, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(QueueBeginDebugUtilsLabel, EXT, instance, VK_NULL_HANDLE);

    ACQUIRE_PROC_SUFFIX(QueueEndDebugUtilsLabel, EXT, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(QueueInsertDebugUtilsLabel, EXT, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(SetDebugUtilsObjectName, EXT, instance, VK_NULL_HANDLE);

    ACQUIRE_PROC_SUFFIX(SetDebugUtilsObjectTag, EXT, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(SubmitDebugUtilsMessage, EXT, instance, VK_NULL_HANDLE);

#endif /* defined(VK_EXT_debug_utils) */
#if defined(VK_EXT_direct_mode_display)
    ACQUIRE_PROC_SUFFIX(ReleaseDisplay, EXT, instance, VK_NULL_HANDLE);
#endif /* defined(VK_EXT_direct_mode_display) */
#if defined(VK_EXT_directfb_surface)
    ACQUIRE_PROC_SUFFIX(CreateDirectFBSurface, EXT, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceDirectFBPresentationSupport, EXT, instance, VK_NULL_HANDLE);
#endif /* defined(VK_EXT_directfb_surface) */
#if defined(VK_EXT_display_surface_counter)
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceSurfaceCapabilities2, EXT, instance, VK_NULL_HANDLE);
#endif /* defined(VK_EXT_display_surface_counter) */
#if defined(VK_EXT_full_screen_exclusive)
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceSurfacePresentModes2, EXT, instance, VK_NULL_HANDLE);
#endif /* defined(VK_EXT_full_screen_exclusive) */
#if defined(VK_EXT_headless_surface)
    ACQUIRE_PROC_SUFFIX(CreateHeadlessSurface, EXT, instance, VK_NULL_HANDLE);
#endif /* defined(VK_EXT_headless_surface) */
#if defined(VK_EXT_metal_surface)
    ACQUIRE_PROC_SUFFIX(CreateMetalSurface, EXT, instance, VK_NULL_HANDLE);
#endif /* defined(VK_EXT_metal_surface) */
#if defined(VK_EXT_sample_locations)
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceMultisampleProperties, EXT, instance, VK_NULL_HANDLE);
#endif /* defined(VK_EXT_sample_locations) */
#if defined(VK_EXT_tooling_info)
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceToolProperties, EXT, instance, VK_NULL_HANDLE);
#endif /* defined(VK_EXT_tooling_info) */
#if defined(VK_FUCHSIA_imagepipe_surface)
    ACQUIRE_PROC_SUFFIX(CreateImagePipeSurface, FUCHSIA, instance, VK_NULL_HANDLE);
#endif /* defined(VK_FUCHSIA_imagepipe_surface) */
#if defined(VK_GGP_stream_descriptor_surface)
    ACQUIRE_PROC_SUFFIX(CreateStreamDescriptorSurface, GGP, instance, VK_NULL_HANDLE);
#endif /* defined(VK_GGP_stream_descriptor_surface) */
#if defined(VK_KHR_android_surface)
    ACQUIRE_PROC_SUFFIX(CreateAndroidSurface, KHR, instance, VK_NULL_HANDLE);
#endif /* defined(VK_KHR_android_surface) */
#if defined(VK_KHR_device_group_creation)
    ACQUIRE_PROC_SUFFIX(EnumeratePhysicalDeviceGroups, KHR, instance, VK_NULL_HANDLE);
#endif /* defined(VK_KHR_device_group_creation) */
#if defined(VK_KHR_display)
    ACQUIRE_PROC_SUFFIX(CreateDisplayMode, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(CreateDisplayPlaneSurface, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetDisplayModeProperties, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetDisplayPlaneCapabilities, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetDisplayPlaneSupportedDisplays, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceDisplayPlaneProperties, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceDisplayProperties, KHR, instance, VK_NULL_HANDLE);
#endif /* defined(VK_KHR_display) */
#if defined(VK_KHR_external_fence_capabilities)
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceExternalFenceProperties, KHR, instance, VK_NULL_HANDLE);
#endif /* defined(VK_KHR_external_fence_capabilities) */
#if defined(VK_KHR_external_memory_capabilities)
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceExternalBufferProperties, KHR, instance, VK_NULL_HANDLE);
#endif /* defined(VK_KHR_external_memory_capabilities) */
#if defined(VK_KHR_external_semaphore_capabilities)
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceExternalSemaphoreProperties, KHR, instance, VK_NULL_HANDLE);
#endif /* defined(VK_KHR_external_semaphore_capabilities) */
#if defined(VK_KHR_fragment_shading_rate)
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceFragmentShadingRates, KHR, instance, VK_NULL_HANDLE);
#endif /* defined(VK_KHR_fragment_shading_rate) */
#if defined(VK_KHR_get_display_properties2)
    ACQUIRE_PROC_SUFFIX(GetDisplayModeProperties2, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetDisplayPlaneCapabilities2, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceDisplayPlaneProperties2, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceDisplayProperties2, KHR, instance, VK_NULL_HANDLE);
#endif /* defined(VK_KHR_get_display_properties2) */
#if defined(VK_KHR_get_physical_device_properties2)
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceFeatures2, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceFormatProperties2, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceImageFormatProperties2, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceMemoryProperties2, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceProperties2, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceQueueFamilyProperties2, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceSparseImageFormatProperties2, KHR, instance, VK_NULL_HANDLE);
#endif /* defined(VK_KHR_get_physical_device_properties2) */
#if defined(VK_KHR_get_surface_capabilities2)
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceSurfaceCapabilities2, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceSurfaceFormats2, KHR, instance, VK_NULL_HANDLE);
#endif /* defined(VK_KHR_get_surface_capabilities2) */
#if defined(VK_KHR_performance_query)
    ACQUIRE_PROC_SUFFIX(EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCounters, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceQueueFamilyPerformanceQueryPasses, KHR, instance, VK_NULL_HANDLE);
#endif /* defined(VK_KHR_performance_query) */
#if defined(VK_KHR_surface)
    ACQUIRE_PROC_SUFFIX(DestroySurface, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceSurfaceCapabilities, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceSurfacePresentModes, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceSurfaceFormats, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceSurfaceSupport, KHR, instance, VK_NULL_HANDLE);
#endif /* defined(VK_KHR_surface) */
#if defined(VK_KHR_video_queue)
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceVideoCapabilities, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceVideoFormatProperties, KHR, instance, VK_NULL_HANDLE);
#endif /* defined(VK_KHR_video_queue) */
#if defined(VK_KHR_wayland_surface)
    ACQUIRE_PROC_SUFFIX(CreateWaylandSurface, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceWaylandPresentationSupport, KHR, instance, VK_NULL_HANDLE);
#endif /* defined(VK_KHR_wayland_surface) */
#if defined(VK_KHR_win32_surface)
    ACQUIRE_PROC_SUFFIX(CreateWin32Surface, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceWin32PresentationSupport, KHR, instance, VK_NULL_HANDLE);
#endif /* defined(VK_KHR_win32_surface) */
#if defined(VK_KHR_xcb_surface)
    ACQUIRE_PROC_SUFFIX(CreateXcbSurface, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceXcbPresentationSupport, KHR, instance, VK_NULL_HANDLE);
#endif /* defined(VK_KHR_xcb_surface) */
#if defined(VK_KHR_xlib_surface)
    ACQUIRE_PROC_SUFFIX(CreateXlibSurface, KHR, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceXlibPresentationSupport, KHR, instance, VK_NULL_HANDLE);
#endif /* defined(VK_KHR_xlib_surface) */
#if defined(VK_MVK_ios_surface)
    ACQUIRE_PROC_SUFFIX(CreateIOSSurface, MVK, instance, VK_NULL_HANDLE);
#endif /* defined(VK_MVK_ios_surface) */
#if defined(VK_MVK_macos_surface)
    ACQUIRE_PROC_SUFFIX(CreateMacOSSurface, MVK, instance, VK_NULL_HANDLE);
#endif /* defined(VK_MVK_macos_surface) */
#if defined(VK_NN_vi_surface)
    ACQUIRE_PROC_SUFFIX(CreateViSurface,NN, instance, VK_NULL_HANDLE);
#endif /* defined(VK_NN_vi_surface) */
#if defined(VK_NV_acquire_winrt_display)
    ACQUIRE_PROC_SUFFIX(AcquireWinrtDisplay,NV, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetWinrtDisplay,NV, instance, VK_NULL_HANDLE);
#endif /* defined(VK_NV_acquire_winrt_display) */
#if defined(VK_NV_cooperative_matrix)
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceCooperativeMatrixProperties,NV, instance, VK_NULL_HANDLE);
#endif /* defined(VK_NV_cooperative_matrix) */
#if defined(VK_NV_coverage_reduction_mode)
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceSupportedFramebufferMixedSamplesCombinations, NV, instance, VK_NULL_HANDLE);
#endif /* defined(VK_NV_coverage_reduction_mode) */
#if defined(VK_NV_external_memory_capabilities)
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceExternalImageFormatProperties, NV, instance, VK_NULL_HANDLE);
#endif /* defined(VK_NV_external_memory_capabilities) */
#if defined(VK_QNX_screen_surface)
    ACQUIRE_PROC_SUFFIX(CreateScreenSurface, QNX, instance, VK_NULL_HANDLE);
    ACQUIRE_PROC_SUFFIX(GetPhysicalDeviceScreenPresentationSupport, QNX, instance, VK_NULL_HANDLE);
#endif /* defined(VK_QNX_screen_surface) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
    ACQUIRE_PROC_SUFFIX(GetPhysicalDevicePresentRectangles, KHR, instance, VK_NULL_HANDLE);
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
}

bool VulkanInterface::validateInstanceFunctions(uint32_t instanceVersion, uint32_t physicalDeviceVersion, const VulkanExtensions* extensions)
{
    if (instanceVersion >= VK_VERSION_1_0)
    {
        if (nullptr == fFunctions.fCreateDevice ||
            nullptr == fFunctions.fDestroyInstance ||
            nullptr == fFunctions.fEnumerateDeviceExtensionProperties ||
            nullptr == fFunctions.fEnumerateDeviceLayerProperties ||
            nullptr == fFunctions.fEnumeratePhysicalDevices ||
            nullptr == fFunctions.fGetDeviceProcAddr ||
            nullptr == fFunctions.fGetPhysicalDeviceFeatures ||
            nullptr == fFunctions.fGetPhysicalDeviceFormatProperties ||
            nullptr == fFunctions.fGetPhysicalDeviceImageFormatProperties ||
            nullptr == fFunctions.fGetPhysicalDeviceMemoryProperties ||
            nullptr == fFunctions.fGetPhysicalDeviceProperties ||
            nullptr == fFunctions.fGetPhysicalDeviceQueueFamilyProperties ||
            nullptr == fFunctions.fGetPhysicalDeviceSparseImageFormatProperties)
            return false;
    };

    if (instanceVersion >= VK_VERSION_1_1)
    {
        if (nullptr == fFunctions.fEnumeratePhysicalDeviceGroups ||
            nullptr == fFunctions.fGetPhysicalDeviceExternalBufferProperties ||
            nullptr == fFunctions.fGetPhysicalDeviceExternalFenceProperties ||
            nullptr == fFunctions.fGetPhysicalDeviceExternalSemaphoreProperties ||
            nullptr == fFunctions.fGetPhysicalDeviceFeatures2 ||
            nullptr == fFunctions.fGetPhysicalDeviceFormatProperties2 ||
            nullptr == fFunctions.fGetPhysicalDeviceImageFormatProperties2 ||
            nullptr == fFunctions.fGetPhysicalDeviceMemoryProperties2 ||
            nullptr == fFunctions.fGetPhysicalDeviceProperties2 ||
            nullptr == fFunctions.fGetPhysicalDeviceQueueFamilyProperties2 ||
            nullptr == fFunctions.fGetPhysicalDeviceSparseImageFormatProperties2)
            return false;
    };

    #if defined(VK_EXT_acquire_drm_display)
    if (extensions->hasExtension(VK_EXT_ACQUIRE_DRM_DISPLAY_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fAcquireDrmDisplayEXT ||
            nullptr == fFunctions.fGetDrmDisplayEXT)
            return false;
    };
    #endif /* defined(VK_EXT_acquire_drm_display) */


#if defined(VK_EXT_acquire_xlib_display)
    if (extensions->hasExtension(VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fAcquireXlibDisplayEXT ||
            nullptr == fFunctions.fGetRandROutputDisplayEXT)
            return false;
    };


#endif /* defined(VK_EXT_acquire_xlib_display) */
#if defined(VK_EXT_calibrated_timestamps)
    if (extensions->hasExtension(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPhysicalDeviceCalibrateableTimeDomainsEXT)
            return false;
    };
#endif /* defined(VK_EXT_calibrated_timestamps) */

#if defined(VK_EXT_debug_report)
    if (extensions->hasExtension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateDebugReportCallbackEXT ||
            nullptr == fFunctions.fDebugReportMessageEXT ||
            nullptr == fFunctions.fDestroyDebugReportCallbackEXT)
            return false;
    };
#endif /* defined(VK_EXT_debug_report) */

#if defined(VK_EXT_debug_utils)
    if (extensions->hasExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdBeginDebugUtilsLabelEXT ||
            nullptr == fFunctions.fCmdEndDebugUtilsLabelEXT ||
            nullptr == fFunctions.fCmdInsertDebugUtilsLabelEXT ||

            nullptr == fFunctions.fCreateDebugUtilsMessengerEXT ||
            nullptr == fFunctions.fDestroyDebugUtilsMessengerEXT ||
            nullptr == fFunctions.fQueueBeginDebugUtilsLabelEXT ||

            nullptr == fFunctions.fQueueEndDebugUtilsLabelEXT ||
            nullptr == fFunctions.fQueueInsertDebugUtilsLabelEXT ||
            nullptr == fFunctions.fSetDebugUtilsObjectNameEXT ||
            
            nullptr == fFunctions.fSetDebugUtilsObjectTagEXT ||
            nullptr == fFunctions.fSubmitDebugUtilsMessageEXT)
            return false;
    };

#endif /* defined(VK_EXT_debug_utils) */
#if defined(VK_EXT_direct_mode_display)
    if (extensions->hasExtension(VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fReleaseDisplayEXT)
            return false;
    };
#endif /* defined(VK_EXT_direct_mode_display) */

#if defined(VK_EXT_directfb_surface)
    if (extensions->hasExtension(VK_EXT_DIRECTFB_SURFACE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateDirectFBSurfaceEXT ||
            nullptr == fFunctions.fGetPhysicalDeviceDirectFBPresentationSupportEXT)
            return false;
    };
#endif /* defined(VK_EXT_directfb_surface) */

#if defined(VK_EXT_display_surface_counter)
    if (extensions->hasExtension(VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPhysicalDeviceSurfaceCapabilities2EXT)
            return false;
    };
#endif /* defined(VK_EXT_display_surface_counter) */

#if defined(VK_EXT_full_screen_exclusive)
    if (extensions->hasExtension(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPhysicalDeviceSurfacePresentModes2EXT)
            return false;
    };
#endif /* defined(VK_EXT_full_screen_exclusive) */

#if defined(VK_EXT_headless_surface)
    if (extensions->hasExtension(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateHeadlessSurfaceEXT)
            return false;
    };
#endif /* defined(VK_EXT_headless_surface) */

#if defined(VK_EXT_metal_surface)
    if (extensions->hasExtension(VK_EXT_METAL_SURFACE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateMetalSurfaceEXT)
            return false;
    };
#endif /* defined(VK_EXT_metal_surface) */

#if defined(VK_EXT_sample_locations)
    if (extensions->hasExtension(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPhysicalDeviceMultisamplePropertiesEXT)
            return false;
    };
#endif /* defined(VK_EXT_sample_locations) */

#if defined(VK_EXT_tooling_info)
    if (extensions->hasExtension(VK_EXT_TOOLING_INFO_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPhysicalDeviceToolPropertiesEXT)
            return false;
    };
#endif /* defined(VK_EXT_tooling_info) */

#if defined(VK_FUCHSIA_imagepipe_surface)
    if (extensions->hasExtension(VK_EXT_SUCHSIA_IMAGEPIPE_SURFACE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateImagePipeSurfaceEXT)
            return false;
    };
#endif /* defined(VK_FUCHSIA_imagepipe_surface) */

#if defined(VK_GGP_stream_descriptor_surface)
    if (extensions->hasExtension(VK_EXT_GGP_STREAM_DESCRIPTOR_SURFACE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateStreamDescriptorSurfaceGGP)
            return false;
    };
#endif /* defined(VK_GGP_stream_descriptor_surface) */

#if defined(VK_KHR_android_surface)
    if (extensions->hasExtension(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateAndroidSurfaceKHR)
            return false;
    };
#endif /* defined(VK_KHR_android_surface) */

#if defined(VK_KHR_device_group_creation)
    if (extensions->hasExtension(VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fEnumeratePhysicalDeviceGroupsKHR)
            return false;
    };
#endif /* defined(VK_KHR_device_group_creation) */

#if defined(VK_KHR_display)
    if (extensions->hasExtension(VK_KHR_DISPLAY_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateDisplayModeKHR ||
            nullptr == fFunctions.fCreateDisplayPlaneSurfaceKHR ||
            nullptr == fFunctions.fGetDisplayModePropertiesKHR ||
            nullptr == fFunctions.fGetDisplayPlaneCapabilitiesKHR ||
            nullptr == fFunctions.fGetDisplayPlaneSupportedDisplaysKHR ||
            nullptr == fFunctions.fGetPhysicalDeviceDisplayPlanePropertiesKHR ||
            nullptr == fFunctions.fGetPhysicalDeviceDisplayPropertiesKHR)
            return false;
    };
#endif /* defined(VK_KHR_display) */

#if defined(VK_KHR_external_fence_capabilities)
    if (extensions->hasExtension(VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPhysicalDeviceExternalFencePropertiesKHR)
            return false;
    };
#endif /* defined(VK_KHR_external_fence_capabilities) */

#if defined(VK_KHR_external_memory_capabilities)
    if (extensions->hasExtension(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPhysicalDeviceExternalBufferPropertiesKHR)
            return false;
    };
#endif /* defined(VK_KHR_external_memory_capabilities) */

#if defined(VK_KHR_external_semaphore_capabilities)
    if (extensions->hasExtension(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPhysicalDeviceExternalSemaphorePropertiesKHR)
            return false;
    };
#endif /* defined(VK_KHR_external_semaphore_capabilities) */

#if defined(VK_KHR_fragment_shading_rate)
    if (extensions->hasExtension(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPhysicalDeviceFragmentShadingRatesKHR)
            return false;
    };
#endif /* defined(VK_KHR_fragment_shading_rate) */

#if defined(VK_KHR_get_display_properties2)
    if (extensions->hasExtension(VK_KHR_GET_DISPLAY_PROPERTIES_2_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetDisplayModeProperties2KHR ||
            nullptr == fFunctions.fGetDisplayPlaneCapabilities2KHR ||
            nullptr == fFunctions.fGetPhysicalDeviceDisplayPlaneProperties2KHR ||
            nullptr == fFunctions.fGetPhysicalDeviceDisplayProperties2KHR)
            return false;
    };
#endif /* defined(VK_KHR_get_display_properties2) */

#if defined(VK_KHR_get_physical_device_properties2)
    if (extensions->hasExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPhysicalDeviceFeatures2KHR ||
            nullptr == fFunctions.fGetPhysicalDeviceFormatProperties2KHR ||
            nullptr == fFunctions.fGetPhysicalDeviceImageFormatProperties2KHR ||
            nullptr == fFunctions.fGetPhysicalDeviceMemoryProperties2KHR ||
            nullptr == fFunctions.fGetPhysicalDeviceProperties2KHR ||
            nullptr == fFunctions.fGetPhysicalDeviceQueueFamilyProperties2KHR ||
            nullptr == fFunctions.fGetPhysicalDeviceSparseImageFormatProperties2KHR)
            return false;
    };
#endif /* defined(VK_KHR_get_physical_device_properties2) */

#if defined(VK_KHR_get_surface_capabilities2)
    if (extensions->hasExtension(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPhysicalDeviceSurfaceCapabilities2KHR ||
            nullptr == fFunctions.fGetPhysicalDeviceSurfaceFormats2KHR)
            return false;
    };
#endif /* defined(VK_KHR_get_surface_capabilities2) */

#if defined(VK_KHR_performance_query)
    if (extensions->hasExtension(VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR ||
            nullptr == fFunctions.fGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR)
            return false;
    };
#endif /* defined(VK_KHR_performance_query) */

#if defined(VK_KHR_surface)
    if (extensions->hasExtension(VK_KHR_SURFACE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fDestroySurfaceKHR ||
            nullptr == fFunctions.fGetPhysicalDeviceSurfaceCapabilitiesKHR ||
            nullptr == fFunctions.fGetPhysicalDeviceSurfacePresentModesKHR ||
            nullptr == fFunctions.fGetPhysicalDeviceSurfaceFormatsKHR ||
            nullptr == fFunctions.fGetPhysicalDeviceSurfaceSupportKHR)
            return false;
    };
#endif /* defined(VK_KHR_surface) */

#if defined(VK_KHR_video_queue)
    if (extensions->hasExtension(VK_KHR_VIDEO_QUEUE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPhysicalDeviceVideoCapabilitiesKHR ||
            nullptr == fFunctions.fGetPhysicalDeviceVideoFormatPropertiesKHR)
            return false;
    };
#endif /* defined(VK_KHR_video_queue) */

#if defined(VK_KHR_wayland_surface)
    if (extensions->hasExtension(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateWaylandSurfaceKHR ||
            nullptr == fFunctions.fGetPhysicalDeviceWaylandPresentationSupportKHR)
            return false;
    };
#endif /* defined(VK_KHR_wayland_surface) */

#if defined(VK_KHR_win32_surface)
    if (extensions->hasExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateWin32SurfaceKHR ||
            nullptr == fFunctions.fGetPhysicalDeviceWin32PresentationSupportKHR)
            return false;
    };
#endif /* defined(VK_KHR_win32_surface) */

#if defined(VK_KHR_xcb_surface)
    if (extensions->hasExtension(VK_KHR_XCB_SURFACE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateXcbSurfaceKHR ||
            nullptr == fFunctions.fGetPhysicalDeviceXcbPresentationSupportKHR)
            return false;
    };
#endif /* defined(VK_KHR_xcb_surface) */

#if defined(VK_KHR_xlib_surface)
    if (extensions->hasExtension(VK_KHR_XLIB_SURFACE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateXlibSurfaceKHR ||
            nullptr == fFunctions.fGetPhysicalDeviceXlibPresentationSupportKHR)
            return false;
    };
#endif /* defined(VK_KHR_xlib_surface) */

#if defined(VK_MVK_ios_surface)
    if (extensions->hasExtension(VK_MVK_IOS_SURFACE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateIOSSurfaceMVK)
            return false;
    };
#endif /* defined(VK_MVK_ios_surface) */

#if defined(VK_MVK_macos_surface)
    if (extensions->hasExtension(VK_MVK_MACOS_SURFACE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateMacOSSurfaceMVK)
            return false;
    };
#endif /* defined(VK_MVK_macos_surface) */

#if defined(VK_NN_vi_surface)
    if (extensions->hasExtension(VK_NN_VI_SURFACE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateViSurfaceNN)
            return false;
    };
#endif /* defined(VK_NN_vi_surface) */

#if defined(VK_NV_acquire_winrt_display)
    if (extensions->hasExtension(VK_NV_ACQUIRE_WINRT_DISPLAY_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fAcquireWinrtDisplayNV ||
            nullptr == fFunctions.fGetWinrtDisplayNV)
            return false;
    };
#endif /* defined(VK_NV_acquire_winrt_display) */

#if defined(VK_NV_cooperative_matrix)
    if (extensions->hasExtension(VK_NV_COOPERATIVE_MATRIX_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPhysicalDeviceCooperativeMatrixPropertiesNV)
            return false;
    };
#endif /* defined(VK_NV_cooperative_matrix) */

#if defined(VK_NV_coverage_reduction_mode)
    if (extensions->hasExtension(VK_NV_COVERAGE_REDUCTION_MODE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV)
            return false;
    };
#endif /* defined(VK_NV_coverage_reduction_mode) */

#if defined(VK_NV_external_memory_capabilities)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPhysicalDeviceExternalImageFormatPropertiesNV)
            return false;
    };
#endif /* defined(VK_NV_external_memory_capabilities) */

#if defined(VK_QNX_screen_surface)
    if (extensions->hasExtension(VK_QNX_SCREEN_SURFACE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateScreenSurfaceQNX ||
            nullptr == fFunctions.fGetPhysicalDeviceScreenPresentationSupportQNX)
            return false;
    };
#endif /* defined(VK_QNX_screen_surface) */

#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
    if (extensions->hasExtension(VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPhysicalDevicePresentRectanglesKHR)
            return false;
    };
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */

    return true;
};

void VulkanInterface::loadDeviceLevel(DiligentGetProc getProc, VkDevice device)
{
#if defined(VK_VERSION_1_0)
    ACQUIRE_PROC(AllocateCommandBuffers, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(AllocateDescriptorSets, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(AllocateMemory, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(BeginCommandBuffer, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(BindBufferMemory, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(BindImageMemory, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdBeginQuery, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdBeginRenderPass, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdBindDescriptorSets, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdBindIndexBuffer, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdBindPipeline, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdBindVertexBuffers, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdBlitImage, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdClearAttachments, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdClearColorImage, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdClearDepthStencilImage, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdCopyBuffer, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdCopyBufferToImage, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdCopyImage, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdCopyImageToBuffer, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdCopyQueryPoolResults, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdDispatch, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdDispatchIndirect, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdDraw, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdDrawIndexed, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdDrawIndexedIndirect, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdDrawIndirect, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdEndQuery, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdEndRenderPass, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdExecuteCommands, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdFillBuffer, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdNextSubpass, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdPipelineBarrier, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdPushConstants, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdResetEvent, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdResetQueryPool, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdResolveImage, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdSetBlendConstants, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdSetDepthBias, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdSetDepthBounds, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdSetEvent, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdSetLineWidth, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdSetScissor, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdSetStencilCompareMask, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdSetStencilReference, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdSetStencilWriteMask, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdSetViewport, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdUpdateBuffer, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdWaitEvents, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CmdWriteTimestamp, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreateBuffer, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreateBufferView, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreateCommandPool, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreateComputePipelines, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreateDescriptorPool, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreateDescriptorSetLayout, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreateEvent, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreateFence, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreateFramebuffer, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreateGraphicsPipelines, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreateImage, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreateImageView, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreatePipelineCache, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreatePipelineLayout, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreateQueryPool, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreateRenderPass, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreateSampler, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreateSemaphore, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(CreateShaderModule, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroyBuffer, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroyBufferView, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroyCommandPool, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroyDescriptorPool, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroyDescriptorSetLayout, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroyDevice, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroyEvent, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroyFence, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroyFramebuffer, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroyImage, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroyImageView, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroyPipeline, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroyPipelineCache, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroyPipelineLayout, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroyQueryPool, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroyRenderPass, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroySampler, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroySemaphore, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DestroyShaderModule, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(DeviceWaitIdle, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(EndCommandBuffer, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(FlushMappedMemoryRanges, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(FreeCommandBuffers, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(FreeDescriptorSets, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(FreeMemory, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(GetBufferMemoryRequirements, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(GetDeviceMemoryCommitment, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(GetDeviceQueue, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(GetEventStatus, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(GetFenceStatus, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(GetImageMemoryRequirements, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(GetImageSparseMemoryRequirements, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(GetImageSubresourceLayout, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(GetPipelineCacheData, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(GetQueryPoolResults, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(GetRenderAreaGranularity, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(InvalidateMappedMemoryRanges, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(MapMemory, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(MergePipelineCaches, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(QueueBindSparse, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(QueueSubmit, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(QueueWaitIdle, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(ResetCommandBuffer, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(ResetCommandPool, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(ResetDescriptorPool, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(ResetEvent, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(ResetFences, VK_NULL_HANDLE, device);
    ACQUIRE_PROC(SetEvent, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(UnmapMemory   , VK_NULL_HANDLE, device);
    ACQUIRE_PROC(UpdateDescriptorSets, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(WaitForFences, VK_NULL_HANDLE, device);
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
   ACQUIRE_PROC(BindBufferMemory2, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(BindImageMemory2, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(CmdDispatchBase, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(CmdSetDeviceMask, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(CreateDescriptorUpdateTemplate, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(CreateSamplerYcbcrConversion, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(DestroyDescriptorUpdateTemplate , VK_NULL_HANDLE, device);
   ACQUIRE_PROC(DestroySamplerYcbcrConversion, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(GetBufferMemoryRequirements2, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(GetDescriptorSetLayoutSupport, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(GetDeviceGroupPeerMemoryFeatures, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(GetDeviceQueue2, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(GetImageMemoryRequirements2, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(GetImageSparseMemoryRequirements2, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(TrimCommandPool, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(UpdateDescriptorSetWithTemplate, VK_NULL_HANDLE, device);
#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_VERSION_1_2)
   ACQUIRE_PROC(CmdBeginRenderPass2, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(CmdDrawIndexedIndirectCount, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(CmdDrawIndirectCount, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(CmdEndRenderPass2, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(CmdNextSubpass2, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(CreateRenderPass2, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(GetBufferDeviceAddress, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(GetBufferOpaqueCaptureAddress, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(GetDeviceMemoryOpaqueCaptureAddress, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(GetSemaphoreCounterValue, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(ResetQueryPool, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(SignalSemaphore, VK_NULL_HANDLE, device);
   ACQUIRE_PROC(WaitSemaphores, VK_NULL_HANDLE, device);
#endif /* defined(VK_VERSION_1_2) */
#if defined(VK_AMD_buffer_marker)
   ACQUIRE_PROC_SUFFIX(CmdWriteBufferMarker, AMD, VK_NULL_HANDLE, device);
#endif /* defined(VK_AMD_buffer_marker) */
#if defined(VK_AMD_display_native_hdr)
   ACQUIRE_PROC_SUFFIX(SetLocalDimming, AMD, VK_NULL_HANDLE, device);
#endif /* defined(VK_AMD_display_native_hdr) */
#if defined(VK_AMD_draw_indirect_count)
   ACQUIRE_PROC_SUFFIX(CmdDrawIndexedIndirectCount, AMD, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(CmdDrawIndirectCount, AMD, VK_NULL_HANDLE, device);
#endif /* defined(VK_AMD_draw_indirect_count) */
#if defined(VK_AMD_shader_info)
   ACQUIRE_PROC_SUFFIX(GetShaderInfo, AMD, VK_NULL_HANDLE, device);
#endif /* defined(VK_AMD_shader_info) */
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
   ACQUIRE_PROC_SUFFIX(GetAndroidHardwareBufferProperties, ANDROID, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(GetAndroidHardwareBufferProperties, ANDROID, VK_NULL_HANDLE, device);
#endif /* defined(VK_ANDROID_external_memory_android_hardware_buffer) */
#if defined(VK_EXT_buffer_device_address)
   ACQUIRE_PROC_SUFFIX(GetBufferDeviceAddress, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_buffer_device_address) */
#if defined(VK_EXT_calibrated_timestamps)
   ACQUIRE_PROC_SUFFIX(GetCalibratedTimestamps, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_calibrated_timestamps) */
#if defined(VK_EXT_color_write_enable)
   ACQUIRE_PROC_SUFFIX(CmdSetColorWriteEnable, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_color_write_enable) */
#if defined(VK_EXT_conditional_rendering)
   ACQUIRE_PROC_SUFFIX(CmdBeginConditionalRendering, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(CmdEndConditionalRendering, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_conditional_rendering) */
#if defined(VK_EXT_debug_marker)
   ACQUIRE_PROC_SUFFIX(CmdDebugMarkerBegin, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(CmdDebugMarkerEnd, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(CmdDebugMarkerInsert, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(DebugMarkerSetObjectName, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(DebugMarkerSetObjectTag, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_debug_marker) */
#if defined(VK_EXT_discard_rectangles)
   ACQUIRE_PROC_SUFFIX(CmdSetDiscardRectangle, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_discard_rectangles) */
#if defined(VK_EXT_display_control)
   ACQUIRE_PROC_SUFFIX(DisplayPowerControl, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(GetSwapchainCounter, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(RegisterDeviceEvent, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(RegisterDisplayEvent, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_display_control) */
#if defined(VK_EXT_extended_dynamic_state)
   ACQUIRE_PROC_SUFFIX(CmdBindVertexBuffers2, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(CmdSetCullMode, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(CmdSetDepthBoundsTestEnable, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(CmdSetDepthCompareOp, EXT, VK_NULL_HANDLE, device);

     ACQUIRE_PROC_SUFFIX(CmdSetDepthTestEnable, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(CmdSetDepthWriteEnable, EXT, VK_NULL_HANDLE, device);
     ACQUIRE_PROC_SUFFIX(CmdSetFrontFace, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(CmdSetPrimitiveTopology, EXT, VK_NULL_HANDLE, device);

     ACQUIRE_PROC_SUFFIX(CmdSetScissorWithCount, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(CmdSetStencilOp, EXT, VK_NULL_HANDLE, device);
     ACQUIRE_PROC_SUFFIX(CmdSetStencilTestEnable, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(CmdSetViewportWithCount, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_extended_dynamic_state) */
#if defined(VK_EXT_extended_dynamic_state2)
   ACQUIRE_PROC_SUFFIX(CmdSetDepthBiasEnable, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(CmdSetLogicOp, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(CmdSetPatchControlPoints, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(CmdSetPrimitiveRestartEnable, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(CmdSetRasterizerDiscardEnable, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_extended_dynamic_state2) */
#if defined(VK_EXT_external_memory_host)
   ACQUIRE_PROC_SUFFIX(GetMemoryHostPointerProperties, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_external_memory_host) */
#if defined(VK_EXT_full_screen_exclusive)
   ACQUIRE_PROC_SUFFIX(AcquireFullScreenExclusiveMode, EXT, VK_NULL_HANDLE, device);
   ACQUIRE_PROC_SUFFIX(ReleaseFullScreenExclusiveMode, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_full_screen_exclusive) */
#if defined(VK_EXT_hdr_metadata)
   ACQUIRE_PROC_SUFFIX(SetHdrMetadata, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_hdr_metadata) */
#if defined(VK_EXT_host_query_reset)
   ACQUIRE_PROC_SUFFIX(ResetQueryPool, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_host_query_reset) */
#if defined(VK_EXT_image_drm_format_modifier)
   ACQUIRE_PROC_SUFFIX(GetImageDrmFormatModifierProperties, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_image_drm_format_modifier) */
#if defined(VK_EXT_line_rasterization)
   ACQUIRE_PROC_SUFFIX(CmdSetLineStipple, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_line_rasterization) */
#if defined(VK_EXT_multi_draw)
    ACQUIRE_PROC_SUFFIX(CmdDrawMulti, EXT, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdDrawMultiIndexed, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_multi_draw) */
#if defined(VK_EXT_private_data)
    ACQUIRE_PROC_SUFFIX(CreatePrivateDataSlot, EXT, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(DestroyPrivateDataSlot, EXT, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetPrivateData, EXT, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(SetPrivateData, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_private_data) */
#if defined(VK_EXT_sample_locations)
    ACQUIRE_PROC_SUFFIX(CmdSetSampleLocations, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_sample_locations) */
#if defined(VK_EXT_transform_feedback)
    ACQUIRE_PROC_SUFFIX(CmdBeginQueryIndexed, EXT, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdBeginTransformFeedback, EXT, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdBindTransformFeedbackBuffers, EXT, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdDrawIndirectByteCount, EXT, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdEndQueryIndexed, EXT, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdEndTransformFeedback, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_transform_feedback) */
#if defined(VK_EXT_validation_cache)
    ACQUIRE_PROC_SUFFIX(CreateValidationCache, EXT, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(DestroyValidationCache, EXT, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetValidationCacheData, EXT, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(MergeValidationCaches, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_validation_cache) */
#if defined(VK_EXT_vertex_input_dynamic_state)
    ACQUIRE_PROC_SUFFIX(CmdSetVertexInput, EXT, VK_NULL_HANDLE, device);
#endif /* defined(VK_EXT_vertex_input_dynamic_state) */
#if defined(VK_FUCHSIA_external_memory)
    ACQUIRE_PROC_SUFFIX(GetMemoryZirconHandle, FUCHSIA, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetMemoryZirconHandleProperties, FUCHSIA, VK_NULL_HANDLE, device);
#endif /* defined(VK_FUCHSIA_external_memory) */
#if defined(VK_FUCHSIA_external_semaphore)
    ACQUIRE_PROC_SUFFIX(GetSemaphoreZirconHandle, FUCHSIA, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(ImportSemaphoreZirconHandle, FUCHSIA, VK_NULL_HANDLE, device);
#endif /* defined(VK_FUCHSIA_external_semaphore) */
#if defined(VK_GOOGLE_display_timing)
    ACQUIRE_PROC_SUFFIX(GetPastPresentationTiming, GOOGLE, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetRefreshCycleDuration, GOOGLE, VK_NULL_HANDLE, device);
#endif /* defined(VK_GOOGLE_display_timing) */
#if defined(VK_HUAWEI_invocation_mask)
    ACQUIRE_PROC_SUFFIX(CmdBindInvocationMask, HUAWEI, VK_NULL_HANDLE, device);
#endif /* defined(VK_HUAWEI_invocation_mask) */
#if defined(VK_HUAWEI_subpass_shading)
    ACQUIRE_PROC_SUFFIX(CmdSubpassShading, HUAWEI, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetDeviceSubpassShadingMaxWorkgroupSize, HUAWEI, VK_NULL_HANDLE, device);
#endif /* defined(VK_HUAWEI_subpass_shading) */
#if defined(VK_INTEL_performance_query)
    ACQUIRE_PROC_SUFFIX(AcquirePerformanceConfiguration, INTEL, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdSetPerformanceMarker, INTEL, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdSetPerformanceOverride, INTEL, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdSetPerformanceStreamMarker, INTEL, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetPerformanceParameter, INTEL, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(InitializePerformanceApi, INTEL, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(QueueSetPerformanceConfiguration, INTEL, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(ReleasePerformanceConfiguration, INTEL, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(UninitializePerformanceApi, INTEL, VK_NULL_HANDLE, device);
#endif /* defined(VK_INTEL_performance_query) */
#if defined(VK_KHR_acceleration_structure)
    ACQUIRE_PROC_SUFFIX(BuildAccelerationStructures, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdBuildAccelerationStructuresIndirect, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdBuildAccelerationStructures, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdCopyAccelerationStructure, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdCopyAccelerationStructureToMemory, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdCopyMemoryToAccelerationStructure, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdWriteAccelerationStructuresProperties, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CopyAccelerationStructure, KHR, VK_NULL_HANDLE, device);

    ACQUIRE_PROC_SUFFIX(CopyAccelerationStructureToMemory, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CopyMemoryToAccelerationStructure, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CreateAccelerationStructure, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(DestroyAccelerationStructure, KHR, VK_NULL_HANDLE, device);

    ACQUIRE_PROC_SUFFIX(GetAccelerationStructureBuildSizes, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetAccelerationStructureDeviceAddress, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetDeviceAccelerationStructureCompatibility, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(WriteAccelerationStructuresProperties, KHR, VK_NULL_HANDLE, device);

#endif /* defined(VK_KHR_acceleration_structure) */
#if defined(VK_KHR_bind_memory2)
    ACQUIRE_PROC_SUFFIX(BindBufferMemory2, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(BindImageMemory2, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_bind_memory2) */
#if defined(VK_KHR_buffer_device_address)
    ACQUIRE_PROC_SUFFIX(GetBufferDeviceAddress, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetBufferOpaqueCaptureAddress, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetDeviceMemoryOpaqueCaptureAddress, KHR, VK_NULL_HANDLE, device);

#endif /* defined(VK_KHR_buffer_device_address) */
#if defined(VK_KHR_copy_commands2)
    ACQUIRE_PROC_SUFFIX(CmdBlitImage2, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdCopyBuffer2, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdCopyBufferToImage2, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdCopyImage2, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdCopyImageToBuffer2, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdResolveImage2, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_copy_commands2) */
#if defined(VK_KHR_create_renderpass2)
    ACQUIRE_PROC_SUFFIX(CmdBeginRenderPass2, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdEndRenderPass2, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdNextSubpass2, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CreateRenderPass2, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_create_renderpass2) */
#if defined(VK_KHR_deferred_host_operations)
    ACQUIRE_PROC_SUFFIX(CreateDeferredOperation, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(DeferredOperationJoin, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(DestroyDeferredOperation, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetDeferredOperationMaxConcurrency, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetDeferredOperationResult, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_deferred_host_operations) */
#if defined(VK_KHR_descriptor_update_template)
    ACQUIRE_PROC_SUFFIX(CreateDescriptorUpdateTemplate, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(DestroyDescriptorUpdateTemplate, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(UpdateDescriptorSetWithTemplate, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_descriptor_update_template) */
#if defined(VK_KHR_device_group)
    ACQUIRE_PROC_SUFFIX(CmdDispatchBase, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdSetDeviceMask, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetDeviceGroupPeerMemoryFeatures, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_device_group) */
#if defined(VK_KHR_display_swapchain)
    ACQUIRE_PROC_SUFFIX(CreateSharedSwapchains, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_display_swapchain) */
#if defined(VK_KHR_draw_indirect_count)
    ACQUIRE_PROC_SUFFIX(CmdDrawIndexedIndirectCount, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdDrawIndirectCount, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_draw_indirect_count) */
#if defined(VK_KHR_external_fence_fd)
    ACQUIRE_PROC_SUFFIX(GetFenceFd, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(ImportFenceFd, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_external_fence_fd) */
#if defined(VK_KHR_external_fence_win32)
    ACQUIRE_PROC_SUFFIX(GetFenceWin32Handle, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(ImportFenceWin32Handle, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_external_fence_win32) */
#if defined(VK_KHR_external_memory_fd)
    ACQUIRE_PROC_SUFFIX(GetMemoryFd, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetMemoryFdProperties, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_external_memory_fd) */
#if defined(VK_KHR_external_memory_win32)
    ACQUIRE_PROC_SUFFIX(GetMemoryWin32Handle, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetMemoryWin32HandleProperties, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_external_memory_win32) */
#if defined(VK_KHR_external_semaphore_fd)
    ACQUIRE_PROC_SUFFIX(GetSemaphoreFd, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(ImportSemaphoreFd, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_external_semaphore_fd) */
#if defined(VK_KHR_external_semaphore_win32)
    ACQUIRE_PROC_SUFFIX(GetSemaphoreWin32Handle, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(ImportSemaphoreWin32Handle, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_external_semaphore_win32) */
#if defined(VK_KHR_fragment_shading_rate)
    ACQUIRE_PROC_SUFFIX(CmdSetFragmentShadingRate, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_fragment_shading_rate) */
#if defined(VK_KHR_get_memory_requirements2)
    ACQUIRE_PROC_SUFFIX(GetBufferMemoryRequirements2, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetImageMemoryRequirements2, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetImageSparseMemoryRequirements2, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_get_memory_requirements2) */
#if defined(VK_KHR_maintenance1)
    ACQUIRE_PROC_SUFFIX(TrimCommandPool, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_maintenance1) */
#if defined(VK_KHR_maintenance3)
    ACQUIRE_PROC_SUFFIX(GetDescriptorSetLayoutSupport, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_maintenance3) */
#if defined(VK_KHR_performance_query)
    ACQUIRE_PROC_SUFFIX(AcquireProfilingLock, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(ReleaseProfilingLock, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_performance_query) */
#if defined(VK_KHR_pipeline_executable_properties)
    ACQUIRE_PROC_SUFFIX(GetPipelineExecutableInternalRepresentations, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetPipelineExecutableProperties, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetPipelineExecutableStatistics, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_pipeline_executable_properties) */
#if defined(VK_KHR_present_wait)
    ACQUIRE_PROC_SUFFIX(WaitForPresent, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_present_wait) */
#if defined(VK_KHR_push_descriptor)
    ACQUIRE_PROC_SUFFIX(CmdPushDescriptorSet, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_push_descriptor) */
#if defined(VK_KHR_ray_tracing_pipeline)
    ACQUIRE_PROC_SUFFIX(CmdSetRayTracingPipelineStackSize, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdTraceRaysIndirect, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdTraceRays, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CreateRayTracingPipelines, KHR, VK_NULL_HANDLE, device);

    ACQUIRE_PROC_SUFFIX(GetRayTracingCaptureReplayShaderGroupHandles, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetRayTracingShaderGroupHandles, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetRayTracingShaderGroupStackSize, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_ray_tracing_pipeline) */
#if defined(VK_KHR_sampler_ycbcr_conversion)
    ACQUIRE_PROC_SUFFIX(CreateSamplerYcbcrConversion, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(DestroySamplerYcbcrConversion, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_sampler_ycbcr_conversion) */
#if defined(VK_KHR_shared_presentable_image)
    ACQUIRE_PROC_SUFFIX(GetSwapchainStatus, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_shared_presentable_image) */
#if defined(VK_KHR_swapchain)
    ACQUIRE_PROC_SUFFIX(AcquireNextImage, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CreateSwapchain, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(DestroySwapchain, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetSwapchainImages, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(QueuePresent, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_swapchain) */
#if defined(VK_KHR_synchronization2)
    ACQUIRE_PROC_SUFFIX(CmdPipelineBarrier2, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdResetEvent2, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdSetEvent2, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdWaitEvents2, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdWriteTimestamp2, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(QueueSubmit2, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_synchronization2) */
#if defined(VK_KHR_synchronization2) && defined(VK_AMD_buffer_marker)
    ACQUIRE_PROC_SUFFIX(CmdWriteBufferMarker2, AMD, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_synchronization2) && defined(VK_AMD_buffer_marker) */
#if defined(VK_KHR_synchronization2) && defined(VK_NV_device_diagnostic_checkpoints)
    ACQUIRE_PROC_SUFFIX(GetQueueCheckpointData2, NV, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_synchronization2) && defined(VK_NV_device_diagnostic_checkpoints) */
#if defined(VK_KHR_timeline_semaphore)
    ACQUIRE_PROC_SUFFIX(GetSemaphoreCounterValue, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(SignalSemaphore, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(WaitSemaphores, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_timeline_semaphore) */
#if defined(VK_KHR_video_decode_queue)
    ACQUIRE_PROC_SUFFIX(CmdDecodeVideo, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_video_decode_queue) */
#if defined(VK_KHR_video_encode_queue)
    ACQUIRE_PROC_SUFFIX(CmdEncodeVideo, KHR, VK_NULL_HANDLE, device);
#endif /* defined(VK_KHR_video_encode_queue) */
#if defined(VK_KHR_video_queue)
    ACQUIRE_PROC_SUFFIX(BindVideoSessionMemory, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdBeginVideoCoding, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdControlVideoCoding, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdEndVideoCoding, KHR, VK_NULL_HANDLE, device);

    ACQUIRE_PROC_SUFFIX(CreateVideoSession, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CreateVideoSessionParameters, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(DestroyVideoSession, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(DestroyVideoSessionParameters, KHR, VK_NULL_HANDLE, device);

    ACQUIRE_PROC_SUFFIX(GetVideoSessionMemoryRequirements, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(UpdateVideoSessionParameters, KHR, VK_NULL_HANDLE, device);

#endif /* defined(VK_KHR_video_queue) */
#if defined(VK_NVX_binary_import)
    ACQUIRE_PROC_SUFFIX(CmdCuLaunchKernel, NVX, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CreateCuFunction, NVX, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CreateCuModule, NVX, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(DestroyCuFunction, NVX, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(DestroyCuModule, NVX, VK_NULL_HANDLE, device);
#endif /* defined(VK_NVX_binary_import) */
#if defined(VK_NVX_image_view_handle)
    ACQUIRE_PROC_SUFFIX(GetImageViewAddress, NVX, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetImageViewHandle, NVX, VK_NULL_HANDLE, device);
#endif /* defined(VK_NVX_image_view_handle) */
#if defined(VK_NV_clip_space_w_scaling)
    ACQUIRE_PROC_SUFFIX(CmdSetViewportWScaling, NV, VK_NULL_HANDLE, device);
#endif /* defined(VK_NV_clip_space_w_scaling) */
#if defined(VK_NV_device_diagnostic_checkpoints)
    ACQUIRE_PROC_SUFFIX(CmdSetCheckpoint, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetQueueCheckpointData, NV, VK_NULL_HANDLE, device);
#endif /* defined(VK_NV_device_diagnostic_checkpoints) */
#if defined(VK_NV_device_generated_commands)
    ACQUIRE_PROC_SUFFIX(CmdBindPipelineShaderGroup, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdExecuteGeneratedCommands, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdPreprocessGeneratedCommands, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CreateIndirectCommandsLayout, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(DestroyIndirectCommandsLayout, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetGeneratedCommandsMemoryRequirements, NV, VK_NULL_HANDLE, device);
#endif /* defined(VK_NV_device_generated_commands) */
#if defined(VK_NV_external_memory_rdma)
    ACQUIRE_PROC_SUFFIX(GetMemoryRemoteAddress, NV, VK_NULL_HANDLE, device);
#endif /* defined(VK_NV_external_memory_rdma) */
#if defined(VK_NV_external_memory_win32)
    ACQUIRE_PROC_SUFFIX(GetMemoryWin32Handle, NV, VK_NULL_HANDLE, device);
#endif /* defined(VK_NV_external_memory_win32) */
#if defined(VK_NV_fragment_shading_rate_enums)
    ACQUIRE_PROC_SUFFIX(CmdSetFragmentShadingRateEnum, NV, VK_NULL_HANDLE, device);
#endif /* defined(VK_NV_fragment_shading_rate_enums) */
#if defined(VK_NV_mesh_shader)
    ACQUIRE_PROC_SUFFIX(CmdDrawMeshTasksIndirectCount, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdDrawMeshTasksIndirect, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdDrawMeshTasks, NV, VK_NULL_HANDLE, device);
#endif /* defined(VK_NV_mesh_shader) */
#if defined(VK_NV_ray_tracing)
    ACQUIRE_PROC_SUFFIX(BindAccelerationStructureMemory, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdBuildAccelerationStructure, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdCopyAccelerationStructure, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdTraceRays, NV, VK_NULL_HANDLE, device);

    ACQUIRE_PROC_SUFFIX(CmdWriteAccelerationStructuresProperties, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CompileDeferred, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CreateAccelerationStructure, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CreateRayTracingPipelines, NV, VK_NULL_HANDLE, device);

    ACQUIRE_PROC_SUFFIX(DestroyAccelerationStructure, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetAccelerationStructureHandle, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetAccelerationStructureMemoryRequirements, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetRayTracingShaderGroupHandles, NV, VK_NULL_HANDLE, device);
#endif /* defined(VK_NV_ray_tracing) */
#if defined(VK_NV_scissor_exclusive)
    ACQUIRE_PROC_SUFFIX(CmdSetExclusiveScissor, NV, VK_NULL_HANDLE, device);
#endif /* defined(VK_NV_scissor_exclusive) */
#if defined(VK_NV_shading_rate_image)
    ACQUIRE_PROC_SUFFIX(CmdBindShadingRateImage, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdSetCoarseSampleOrder, NV, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(CmdSetViewportShadingRatePalette, NV, VK_NULL_HANDLE, device);
#endif /* defined(VK_NV_shading_rate_image) */
#if (defined(VK_EXT_full_screen_exclusive) && defined(VK_KHR_device_group)) || (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1))
    ACQUIRE_PROC_SUFFIX(GetDeviceGroupSurfacePresentModes2, EXT, VK_NULL_HANDLE, device);
#endif /* (defined(VK_EXT_full_screen_exclusive) && defined(VK_KHR_device_group)) || (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_push_descriptor) && defined(VK_KHR_descriptor_update_template))
    ACQUIRE_PROC_SUFFIX(CmdPushDescriptorSetWithTemplate, KHR, VK_NULL_HANDLE, device);
#endif /* (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_push_descriptor) && defined(VK_KHR_descriptor_update_template)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
    ACQUIRE_PROC_SUFFIX(GetDeviceGroupPresentCapabilities, KHR, VK_NULL_HANDLE, device);
    ACQUIRE_PROC_SUFFIX(GetDeviceGroupSurfacePresentModes, KHR, VK_NULL_HANDLE, device);
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
    ACQUIRE_PROC_SUFFIX(AcquireNextImage2, KHR, VK_NULL_HANDLE, device);
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
}

bool VulkanInterface::validateDeviceFunctions(uint32_t instanceVersion, uint32_t physicalDeviceVersion, const VulkanExtensions* extensions)
{
    if (instanceVersion >= VK_VERSION_1_0)
    {
        if (nullptr == fFunctions.fAllocateCommandBuffers ||
            nullptr == fFunctions.fAllocateDescriptorSets ||
            nullptr == fFunctions.fAllocateMemory ||
            nullptr == fFunctions.fBeginCommandBuffer ||
            nullptr == fFunctions.fBindBufferMemory ||
            nullptr == fFunctions.fBindImageMemory ||
            nullptr == fFunctions.fCmdBeginQuery ||
            nullptr == fFunctions.fCmdBeginRenderPass ||
            nullptr == fFunctions.fCmdBindDescriptorSets ||
            nullptr == fFunctions.fCmdBindIndexBuffer ||
            nullptr == fFunctions.fCmdBindPipeline ||
            nullptr == fFunctions.fCmdBindVertexBuffers ||
            nullptr == fFunctions.fCmdBlitImage ||
            nullptr == fFunctions.fCmdClearAttachments ||
            nullptr == fFunctions.fCmdClearColorImage ||
            nullptr == fFunctions.fCmdClearDepthStencilImage ||
            nullptr == fFunctions.fCmdCopyBuffer ||
            nullptr == fFunctions.fCmdCopyBufferToImage ||
            nullptr == fFunctions.fCmdCopyImage ||
            nullptr == fFunctions.fCmdCopyImageToBuffer ||
            nullptr == fFunctions.fCmdCopyQueryPoolResults ||
            nullptr == fFunctions.fCmdDispatch ||
            nullptr == fFunctions.fCmdDispatchIndirect ||
            nullptr == fFunctions.fCmdDraw ||
            nullptr == fFunctions.fCmdDrawIndexed ||
            nullptr == fFunctions.fCmdDrawIndexedIndirect ||
            nullptr == fFunctions.fCmdDrawIndirect ||
            nullptr == fFunctions.fCmdEndQuery ||
            nullptr == fFunctions.fCmdEndRenderPass ||
            nullptr == fFunctions.fCmdExecuteCommands ||
            nullptr == fFunctions.fCmdFillBuffer ||
            nullptr == fFunctions.fCmdNextSubpass ||
            nullptr == fFunctions.fCmdPipelineBarrier ||
            nullptr == fFunctions.fCmdPushConstants ||
            nullptr == fFunctions.fCmdResetEvent ||
            nullptr == fFunctions.fCmdResetQueryPool ||
            nullptr == fFunctions.fCmdResolveImage ||
            nullptr == fFunctions.fCmdSetBlendConstants ||
            nullptr == fFunctions.fCmdSetDepthBias ||
            nullptr == fFunctions.fCmdSetDepthBounds ||
            nullptr == fFunctions.fCmdSetEvent ||
            nullptr == fFunctions.fCmdSetLineWidth ||
            nullptr == fFunctions.fCmdSetScissor ||
            nullptr == fFunctions.fCmdSetStencilCompareMask ||
            nullptr == fFunctions.fCmdSetStencilReference ||
            nullptr == fFunctions.fCmdSetStencilWriteMask ||
            nullptr == fFunctions.fCmdWaitEvents ||
            nullptr == fFunctions.fCmdUpdateBuffer ||
            nullptr == fFunctions.fCmdWriteTimestamp ||
            nullptr == fFunctions.fCreateBuffer ||
            nullptr == fFunctions.fCreateBufferView ||
            nullptr == fFunctions.fCreateCommandPool ||
            nullptr == fFunctions.fCreateComputePipelines ||
            nullptr == fFunctions.fCreateDescriptorPool ||
            nullptr == fFunctions.fCreateDescriptorSetLayout ||
            nullptr == fFunctions.fCreateEvent ||
            nullptr == fFunctions.fCreateFence ||
            nullptr == fFunctions.fCreateFramebuffer ||
            nullptr == fFunctions.fCreateGraphicsPipelines ||
            nullptr == fFunctions.fCreateImage ||
            nullptr == fFunctions.fCreateImageView ||
            nullptr == fFunctions.fCreatePipelineCache ||
            nullptr == fFunctions.fCreatePipelineLayout ||
            nullptr == fFunctions.fCreateQueryPool ||
            nullptr == fFunctions.fCreateRenderPass ||
            nullptr == fFunctions.fCreateSampler ||
            nullptr == fFunctions.fCreateSemaphore ||
            nullptr == fFunctions.fCreateShaderModule ||
            nullptr == fFunctions.fDestroyBuffer ||
            nullptr == fFunctions.fDestroyBufferView ||
            nullptr == fFunctions.fDestroyCommandPool ||
            nullptr == fFunctions.fDestroyDescriptorPool ||
            nullptr == fFunctions.fDestroyDescriptorSetLayout ||
            nullptr == fFunctions.fDestroyDevice ||
            nullptr == fFunctions.fDestroyEvent ||
            nullptr == fFunctions.fDestroyFence ||
            nullptr == fFunctions.fDestroyFramebuffer ||
            nullptr == fFunctions.fDestroyImage ||
            nullptr == fFunctions.fDestroyImageView ||
            nullptr == fFunctions.fDestroyPipeline ||
            nullptr == fFunctions.fDestroyPipelineCache ||
            nullptr == fFunctions.fDestroyPipelineLayout ||
            nullptr == fFunctions.fDestroyQueryPool ||
            nullptr == fFunctions.fDestroyRenderPass ||
            nullptr == fFunctions.fDestroySampler ||
            nullptr == fFunctions.fDestroySemaphore ||
            nullptr == fFunctions.fDestroyShaderModule ||
            nullptr == fFunctions.fDeviceWaitIdle ||
            nullptr == fFunctions.fEndCommandBuffer ||
            nullptr == fFunctions.fFlushMappedMemoryRanges ||
            nullptr == fFunctions.fFreeCommandBuffers ||
            nullptr == fFunctions.fFreeDescriptorSets ||
            nullptr == fFunctions.fFreeMemory ||
            nullptr == fFunctions.fGetBufferMemoryRequirements ||
            nullptr == fFunctions.fGetDeviceMemoryCommitment ||
            nullptr == fFunctions.fGetDeviceQueue ||
            nullptr == fFunctions.fGetEventStatus ||
            nullptr == fFunctions.fGetFenceStatus ||
            nullptr == fFunctions.fGetImageMemoryRequirements ||
            nullptr == fFunctions.fGetImageSparseMemoryRequirements ||
            nullptr == fFunctions.fGetImageSubresourceLayout ||
            nullptr == fFunctions.fGetPipelineCacheData ||
            nullptr == fFunctions.fGetQueryPoolResults ||
            nullptr == fFunctions.fGetRenderAreaGranularity ||
            nullptr == fFunctions.fInvalidateMappedMemoryRanges ||
            nullptr == fFunctions.fMapMemory ||
            nullptr == fFunctions.fMergePipelineCaches ||
            nullptr == fFunctions.fQueueBindSparse ||
            nullptr == fFunctions.fQueueSubmit ||
            nullptr == fFunctions.fQueueWaitIdle ||
            nullptr == fFunctions.fResetCommandBuffer ||
            nullptr == fFunctions.fResetCommandPool ||
            nullptr == fFunctions.fResetDescriptorPool ||
            nullptr == fFunctions.fResetEvent ||
            nullptr == fFunctions.fResetFences ||
            nullptr == fFunctions.fSetEvent ||
            nullptr == fFunctions.fUnmapMemory ||
            nullptr == fFunctions.fUpdateDescriptorSets ||
            nullptr == fFunctions.fWaitForFences)
            return false;
    };

    if (instanceVersion >= VK_VERSION_1_1)
    {
        if (nullptr == fFunctions.fBindBufferMemory2 ||
            nullptr == fFunctions.fBindImageMemory2 ||
            nullptr == fFunctions.fCmdDispatchBase ||
            nullptr == fFunctions.fCmdSetDeviceMask ||
            nullptr == fFunctions.fCreateDescriptorUpdateTemplate ||
            nullptr == fFunctions.fCreateSamplerYcbcrConversion ||
            nullptr == fFunctions.fDestroyDescriptorUpdateTemplate ||
            nullptr == fFunctions.fDestroySamplerYcbcrConversion ||
            nullptr == fFunctions.fGetBufferMemoryRequirements2 ||
            nullptr == fFunctions.fGetDescriptorSetLayoutSupport ||
            nullptr == fFunctions.fGetDeviceGroupPeerMemoryFeatures ||
            nullptr == fFunctions.fGetDeviceQueue2 ||
            nullptr == fFunctions.fGetImageMemoryRequirements2 ||
            nullptr == fFunctions.fGetImageSparseMemoryRequirements2 ||
            nullptr == fFunctions.fTrimCommandPool ||
            nullptr == fFunctions.fUpdateDescriptorSetWithTemplate)
            return false;
    };

        if (instanceVersion >= VK_VERSION_1_2)
    {
            if (nullptr == fFunctions.fCmdBeginRenderPass2 ||
            nullptr == fFunctions.fCmdDrawIndexedIndirectCount ||
            nullptr == fFunctions.fCmdDrawIndirectCount ||
            nullptr == fFunctions.fCmdEndRenderPass2 ||
            nullptr == fFunctions.fCmdEndRenderPass2 ||
            nullptr == fFunctions.fCmdNextSubpass2 ||
            nullptr == fFunctions.fCreateRenderPass2 ||
            nullptr == fFunctions.fGetBufferDeviceAddress ||
            nullptr == fFunctions.fGetBufferOpaqueCaptureAddress ||
            nullptr == fFunctions.fGetDeviceMemoryOpaqueCaptureAddress ||
            nullptr == fFunctions.fGetSemaphoreCounterValue ||
            nullptr == fFunctions.fResetQueryPool ||
            nullptr == fFunctions.fSignalSemaphore ||
            nullptr == fFunctions.fWaitSemaphores)
            return false;
    };

#if defined(VK_AMD_buffer_marker)
    if (extensions->hasExtension(VK_AMD_BUFFER_MARKER_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdWriteBufferMarkerAMD)
            return false;
    };
#endif /* defined(VK_AMD_buffer_marker) */

#if defined(VK_AMD_display_native_hdr)
    if (extensions->hasExtension(VK_AMD_DISPLAY_NATIVE_HDR_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fSetLocalDimmingAMD)
            return false;
    };
#endif /* defined(VK_AMD_display_native_hdr) */

#if defined(VK_AMD_draw_indirect_count)
    if (extensions->hasExtension(VK_AMD_DRAW_INDIRECT_COUNT_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdDrawIndexedIndirectCountAMD ||
            nullptr == fFunctions.fCmdDrawIndirectCountAMD)
            return false;
    };
#endif /* defined(VK_AMD_draw_indirect_count) */

#if defined(VK_AMD_shader_info)
    if (extensions->hasExtension(VK_AMD_SHADER_INFO_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetShaderInfoAMD)
            return false;
    };
#endif /* defined(VK_AMD_shader_info) */

#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
    if (extensions->hasExtension(VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetAndroidHardwareBufferPropertiesANDROID ||
            nullptr == fFunctions.fGetAndroidHardwareBufferPropertiesANDROID)
            return false;

        // TODO: possible error, need to check the volk implementation
    };
#endif /* defined(VK_ANDROID_external_memory_android_hardware_buffer) */

#if defined(VK_EXT_buffer_device_address)
    if (extensions->hasExtension(VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetBufferDeviceAddressEXT)
            return false;
    };
#endif /* defined(VK_EXT_buffer_device_address) */

#if defined(VK_EXT_calibrated_timestamps)
    if (extensions->hasExtension(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetCalibratedTimestampsEXT)
            return false;
    };
#endif /* defined(VK_EXT_calibrated_timestamps) */

#if defined(VK_EXT_color_write_enable)
    if (extensions->hasExtension(VK_EXT_COLOR_WRITE_ENABLE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdSetColorWriteEnableEXT)
            return false;
    };
#endif /* defined(VK_EXT_color_write_enable) */

#if defined(VK_EXT_conditional_rendering)
    if (extensions->hasExtension(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdBeginConditionalRenderingEXT ||
            nullptr == fFunctions.fCmdEndConditionalRenderingEXT)
            return false;
    };
#endif /* defined(VK_EXT_conditional_rendering) */

#if defined(VK_EXT_debug_marker)
    if (extensions->hasExtension(VK_EXT_DEBUG_MARKER_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdDebugMarkerBeginEXT ||
            nullptr == fFunctions.fCmdDebugMarkerEndEXT ||
            nullptr == fFunctions.fCmdDebugMarkerInsertEXT ||
            nullptr == fFunctions.fDebugMarkerSetObjectNameEXT ||
            nullptr == fFunctions.fDebugMarkerSetObjectTagEXT)
            return false;
    };
#endif /* defined(VK_EXT_debug_marker) */

#if defined(VK_EXT_discard_rectangles)
    if (extensions->hasExtension(VK_EXT_DISCARD_RECTANGLES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdSetDiscardRectangleEXT)
            return false;
    };
#endif /* defined(VK_EXT_discard_rectangles) */

#if defined(VK_EXT_display_control)
    if (extensions->hasExtension(VK_EXT_DISPLAY_CONTROL_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fDisplayPowerControlEXT ||
            nullptr == fFunctions.fGetSwapchainCounterEXT ||
            nullptr == fFunctions.fRegisterDeviceEventEXT ||
            nullptr == fFunctions.fRegisterDisplayEventEXT)
            return false;
    };
#endif /* defined(VK_EXT_display_control) */

#if defined(VK_EXT_extended_dynamic_state)
    if (extensions->hasExtension(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdBindVertexBuffers2EXT ||
            nullptr == fFunctions.fCmdSetCullModeEXT ||
            nullptr == fFunctions.fCmdSetDepthBoundsTestEnableEXT ||
            nullptr == fFunctions.fCmdSetDepthCompareOpEXT ||
            
            nullptr == fFunctions.fCmdSetDepthTestEnableEXT ||
            nullptr == fFunctions.fCmdSetDepthWriteEnableEXT ||
            nullptr == fFunctions.fCmdSetFrontFaceEXT ||
            nullptr == fFunctions.fCmdSetPrimitiveTopologyEXT ||

            nullptr == fFunctions.fCmdSetScissorWithCountEXT ||
            nullptr == fFunctions.fCmdSetStencilOpEXT ||
            nullptr == fFunctions.fCmdSetStencilTestEnableEXT ||
            nullptr == fFunctions.fCmdSetViewportWithCountEXT 
            )
            return false;
    };
#endif /* defined(VK_EXT_extended_dynamic_state) */

#if defined(VK_EXT_extended_dynamic_state2)
    if (extensions->hasExtension(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdSetDepthBiasEnableEXT ||
            nullptr == fFunctions.fCmdSetLogicOpEXT ||
            nullptr == fFunctions.fCmdSetPatchControlPointsEXT ||
            nullptr == fFunctions.fCmdSetPrimitiveRestartEnableEXT ||
            nullptr == fFunctions.fCmdSetRasterizerDiscardEnableEXT)
            return false;
    };
#endif /* defined(VK_EXT_extended_dynamic_state2) */

#if defined(VK_EXT_external_memory_host)
    if (extensions->hasExtension(VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetMemoryHostPointerPropertiesEXT)
            return false;
    };
#endif /* defined(VK_EXT_external_memory_host) */

#if defined(VK_EXT_full_screen_exclusive)
    if (extensions->hasExtension(VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fAcquireFullScreenExclusiveModeEXT ||
            nullptr == fFunctions.fReleaseFullScreenExclusiveModeEXT)
            return false;
    };
#endif /* defined(VK_EXT_full_screen_exclusive) */

#if defined(VK_EXT_hdr_metadata)
    if (extensions->hasExtension(VK_EXT_HDR_METADATA_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fSetHdrMetadataEXT)
            return false;
    };
#endif /* defined(VK_EXT_hdr_metadata) */

#if defined(VK_EXT_host_query_reset)
    if (extensions->hasExtension(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fResetQueryPoolEXT)
            return false;
    };
#endif /* defined(VK_EXT_host_query_reset) */

#if defined(VK_EXT_image_drm_format_modifier)
    if (extensions->hasExtension(VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetImageDrmFormatModifierPropertiesEXT)
            return false;
    };
#endif /* defined(VK_EXT_image_drm_format_modifier) */

#if defined(VK_EXT_line_rasterization)
    if (extensions->hasExtension(VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdSetLineStippleEXT)
            return false;
    };
#endif /* defined(VK_EXT_line_rasterization) */

#if defined(VK_EXT_multi_draw)
    if (extensions->hasExtension(VK_EXT_MULTI_DRAW_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdDrawMultiEXT ||
            nullptr == fFunctions.fCmdDrawMultiIndexedEXT)
            return false;
    };
#endif /* defined(VK_EXT_multi_draw) */

#if defined(VK_EXT_private_data)
    if (extensions->hasExtension(VK_EXT_PRIVATE_DATA_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreatePrivateDataSlotEXT ||
            nullptr == fFunctions.fDestroyPrivateDataSlotEXT ||
            nullptr == fFunctions.fGetPrivateDataEXT ||
            nullptr == fFunctions.fSetPrivateDataEXT)
            return false;
    };
#endif /* defined(VK_EXT_private_data) */

#if defined(VK_EXT_sample_locations)
    if (extensions->hasExtension(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdSetSampleLocationsEXT)
            return false;
    };
#endif /* defined(VK_EXT_sample_locations) */

#if defined(VK_EXT_transform_feedback)
    if (extensions->hasExtension(VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdBeginQueryIndexedEXT ||
            nullptr == fFunctions.fCmdBeginTransformFeedbackEXT ||
            nullptr == fFunctions.fCmdBindTransformFeedbackBuffersEXT ||
            nullptr == fFunctions.fCmdDrawIndirectByteCountEXT ||
            nullptr == fFunctions.fCmdEndQueryIndexedEXT ||
            nullptr == fFunctions.fCmdEndTransformFeedbackEXT)
            return false;
    };
#endif /* defined(VK_EXT_transform_feedback) */

#if defined(VK_EXT_validation_cache)
    if (extensions->hasExtension(VK_EXT_VALIDATION_CACHE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateValidationCacheEXT ||
            nullptr == fFunctions.fDestroyValidationCacheEXT ||
            nullptr == fFunctions.fGetValidationCacheDataEXT ||
            nullptr == fFunctions.fMergeValidationCachesEXT)
            return false;
    };
#endif /* defined(VK_EXT_validation_cache) */

#if defined(VK_EXT_vertex_input_dynamic_state)
    if (extensions->hasExtension(VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdSetVertexInputEXT)
            return false;
    };
#endif /* defined(VK_EXT_vertex_input_dynamic_state) */

#if defined(VK_FUCHSIA_external_memory)
    if (extensions->hasExtension(VK_FUCHSIA_EXTERNAL_MEMORY_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetMemoryZirconHandleFUCHSIA ||
            nullptr == fFunctions.fGetMemoryZirconHandlePropertiesFUCHSIA)
            return false;
    };
#endif /* defined(VK_FUCHSIA_external_memory) */

#if defined(VK_FUCHSIA_external_semaphore)
    if (extensions->hasExtension(VK_FUCHSIA_EXTERNAL_SEMAPHORE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetSemaphoreZirconHandleFUCHSIA ||
            nullptr == fFunctions.fImportSemaphoreZirconHandleFUCHSIA)
            return false;
    };
#endif /* defined(VK_FUCHSIA_external_semaphore) */

#if defined(VK_GOOGLE_display_timing)
    if (extensions->hasExtension(VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPastPresentationTimingGOOGLE ||
            nullptr == fFunctions.fGetRefreshCycleDurationGOOGLE)
            return false;
    };
#endif /* defined(VK_GOOGLE_display_timing) */

#if defined(VK_HUAWEI_invocation_mask)
    if (extensions->hasExtension(VK_HUAWEI_INVOCATION_MASK_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdBindInvocationMaskHUAWEI)
            return false;
    };
#endif /* defined(VK_HUAWEI_invocation_mask) */

#if defined(VK_HUAWEI_subpass_shading)
    if (extensions->hasExtension(VK_HUAWEI_SUBPASS_SHADING_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdSubpassShadingHUAWEI ||
            nullptr == fFunctions.fGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI)
            return false;
    };
#endif /* defined(VK_HUAWEI_subpass_shading) */

#if defined(VK_INTEL_performance_query)
    if (extensions->hasExtension(VK_INTEL_PERFORMANCE_QUERY_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fAcquirePerformanceConfigurationINTEL ||
            nullptr == fFunctions.fCmdSetPerformanceMarkerINTEL ||
            nullptr == fFunctions.fCmdSetPerformanceOverrideINTEL ||
            nullptr == fFunctions.fCmdSetPerformanceStreamMarkerINTEL ||
            nullptr == fFunctions.fGetPerformanceParameterINTEL ||
            nullptr == fFunctions.fGetPerformanceParameterINTEL ||
            nullptr == fFunctions.fInitializePerformanceApiINTEL ||
            nullptr == fFunctions.fQueueSetPerformanceConfigurationINTEL ||
            nullptr == fFunctions.fReleasePerformanceConfigurationINTEL ||
            nullptr == fFunctions.fUninitializePerformanceApiINTEL)
            return false;
    };
#endif /* defined(VK_INTEL_performance_query) */

#if defined(VK_KHR_acceleration_structure)
    if (extensions->hasExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fBuildAccelerationStructuresKHR ||
            nullptr == fFunctions.fCmdBuildAccelerationStructuresIndirectKHR ||
            nullptr == fFunctions.fCmdBuildAccelerationStructuresKHR ||
            nullptr == fFunctions.fCmdCopyAccelerationStructureKHR ||
            nullptr == fFunctions.fCmdCopyAccelerationStructureToMemoryKHR ||
            nullptr == fFunctions.fCmdCopyMemoryToAccelerationStructureKHR ||
            nullptr == fFunctions.fCmdWriteAccelerationStructuresPropertiesKHR ||
            nullptr == fFunctions.fCopyAccelerationStructureKHR ||

            nullptr == fFunctions.fCopyAccelerationStructureToMemoryKHR ||
            nullptr == fFunctions.fCopyMemoryToAccelerationStructureKHR ||
            nullptr == fFunctions.fCreateAccelerationStructureKHR ||
            nullptr == fFunctions.fDestroyAccelerationStructureKHR ||

            nullptr == fFunctions.fGetAccelerationStructureBuildSizesKHR ||
            nullptr == fFunctions.fGetAccelerationStructureDeviceAddressKHR ||
            nullptr == fFunctions.fGetDeviceAccelerationStructureCompatibilityKHR ||
            nullptr == fFunctions.fWriteAccelerationStructuresPropertiesKHR 
            )
            return false;
    };
#endif /* defined(VK_KHR_acceleration_structure) */

#if defined(VK_KHR_bind_memory2)
    if (extensions->hasExtension(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fBindBufferMemory2KHR ||
            nullptr == fFunctions.fBindImageMemory2KHR)
            return false;
    };
#endif /* defined(VK_KHR_bind_memory2) */

#if defined(VK_KHR_buffer_device_address)
    if (extensions->hasExtension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetBufferDeviceAddressKHR ||
            nullptr == fFunctions.fGetBufferOpaqueCaptureAddressKHR ||
            nullptr == fFunctions.fGetDeviceMemoryOpaqueCaptureAddressKHR)
            return false;
    };
#endif /* defined(VK_KHR_buffer_device_address) */

#if defined(VK_KHR_copy_commands2)
    if (extensions->hasExtension(VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdBlitImage2KHR ||
            nullptr == fFunctions.fCmdCopyBuffer2KHR ||
            nullptr == fFunctions.fCmdCopyBufferToImage2KHR ||
            nullptr == fFunctions.fCmdCopyImage2KHR ||
            nullptr == fFunctions.fCmdCopyImageToBuffer2KHR ||
            nullptr == fFunctions.fCmdResolveImage2KHR)
            return false;
    };
#endif /* defined(VK_KHR_copy_commands2) */

#if defined(VK_KHR_create_renderpass2)
    if (extensions->hasExtension(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdBeginRenderPass2KHR ||
            nullptr == fFunctions.fCmdEndRenderPass2KHR ||
            nullptr == fFunctions.fCmdNextSubpass2KHR ||
            nullptr == fFunctions.fCreateRenderPass2KHR)
            return false;
    };
#endif /* defined(VK_KHR_create_renderpass2) */

#if defined(VK_KHR_deferred_host_operations)
    if (extensions->hasExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateDeferredOperationKHR ||
            nullptr == fFunctions.fDeferredOperationJoinKHR ||
            nullptr == fFunctions.fDestroyDeferredOperationKHR ||
            nullptr == fFunctions.fGetDeferredOperationMaxConcurrencyKHR ||
            nullptr == fFunctions.fGetDeferredOperationResultKHR)
            return false;
    };
#endif /* defined(VK_KHR_deferred_host_operations) */

#if defined(VK_KHR_descriptor_update_template)
    if (extensions->hasExtension(VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateDescriptorUpdateTemplateKHR ||
            nullptr == fFunctions.fDestroyDescriptorUpdateTemplateKHR ||
            nullptr == fFunctions.fUpdateDescriptorSetWithTemplateKHR)
            return false;
    };
#endif /* defined(VK_KHR_descriptor_update_template) */

#if defined(VK_KHR_device_group)
    if (extensions->hasExtension(VK_KHR_DEVICE_GROUP_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdDispatchBaseKHR ||
            nullptr == fFunctions.fCmdSetDeviceMaskKHR ||
            nullptr == fFunctions.fGetDeviceGroupPeerMemoryFeaturesKHR )
            return false;
    };
#endif /* defined(VK_KHR_device_group) */

#if defined(VK_KHR_display_swapchain)
    if (extensions->hasExtension(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateSharedSwapchainsKHR)
            return false;
    };
#endif /* defined(VK_KHR_display_swapchain) */

#if defined(VK_KHR_draw_indirect_count)
    if (extensions->hasExtension(VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdDrawIndexedIndirectCountKHR ||
            nullptr == fFunctions.fCmdDrawIndirectCountKHR)
            return false;
    };
#endif /* defined(VK_KHR_draw_indirect_count) */

#if defined(VK_KHR_external_fence_fd)
    if (extensions->hasExtension(VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetFenceFdKHR ||
            nullptr == fFunctions.fImportFenceFdKHR)
            return false;
    };
#endif /* defined(VK_KHR_external_fence_fd) */

#if defined(VK_KHR_external_fence_win32)
    if (extensions->hasExtension(VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetFenceWin32HandleKHR ||
            nullptr == fFunctions.fImportFenceWin32HandleKHR)
            return false;
    };
#endif /* defined(VK_KHR_external_fence_win32) */

#if defined(VK_KHR_external_memory_fd)
    if (extensions->hasExtension(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetMemoryFdKHR ||
            nullptr == fFunctions.fGetMemoryFdPropertiesKHR)
            return false;
    };
#endif /* defined(VK_KHR_external_memory_fd) */

#if defined(VK_KHR_external_memory_win32)
    if (extensions->hasExtension(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetMemoryWin32HandleKHR ||
            nullptr == fFunctions.fGetMemoryWin32HandlePropertiesKHR)
            return false;
    };
#endif /* defined(VK_KHR_external_memory_win32) */

#if defined(VK_KHR_external_semaphore_fd)
    if (extensions->hasExtension(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetSemaphoreFdKHR ||
            nullptr == fFunctions.fImportSemaphoreFdKHR)
            return false;
    };
#endif /* defined(VK_KHR_external_semaphore_fd) */

#if defined(VK_KHR_external_semaphore_win32)
    if (extensions->hasExtension(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetSemaphoreWin32HandleKHR ||
            nullptr == fFunctions.fImportSemaphoreWin32HandleKHR)
            return false;
    };
#endif /* defined(VK_KHR_external_semaphore_win32) */

#if defined(VK_KHR_fragment_shading_rate)
    if (extensions->hasExtension(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdSetFragmentShadingRateKHR)
            return false;
    };
#endif /* defined(VK_KHR_fragment_shading_rate) */

#if defined(VK_KHR_get_memory_requirements2)
    if (extensions->hasExtension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetBufferMemoryRequirements2KHR ||
            nullptr == fFunctions.fGetImageMemoryRequirements2KHR ||
            nullptr == fFunctions.fGetImageSparseMemoryRequirements2KHR)
            return false;
    };
#endif /* defined(VK_KHR_get_memory_requirements2) */

#if defined(VK_KHR_maintenance1)
    if (extensions->hasExtension(VK_KHR_MAINTENANCE1_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fTrimCommandPoolKHR)
            return false;
    };
#endif /* defined(VK_KHR_maintenance1) */

#if defined(VK_KHR_maintenance3)
    if (extensions->hasExtension(VK_KHR_MAINTENANCE3_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetDescriptorSetLayoutSupportKHR)
            return false;
    };
#endif /* defined(VK_KHR_maintenance3) */

#if defined(VK_KHR_performance_query)
    if (extensions->hasExtension(VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fAcquireProfilingLockKHR ||
            nullptr == fFunctions.fReleaseProfilingLockKHR)
            return false;
    };
#endif /* defined(VK_KHR_performance_query) */

#if defined(VK_KHR_pipeline_executable_properties)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetPipelineExecutableInternalRepresentationsKHR ||
            nullptr == fFunctions.fGetPipelineExecutablePropertiesKHR ||
            nullptr == fFunctions.fGetPipelineExecutableStatisticsKHR)
            return false;
    };
#endif /* defined(VK_KHR_pipeline_executable_properties) */

#if defined(VK_KHR_present_wait)
    if (extensions->hasExtension(VK_KHR_PRESENT_WAIT_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fWaitForPresentKHR)
            return false;
    };
#endif /* defined(VK_KHR_present_wait) */

#if defined(VK_KHR_push_descriptor)
    if (extensions->hasExtension(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdPushDescriptorSetKHR)
            return false;
    };
#endif /* defined(VK_KHR_push_descriptor) */

#if defined(VK_KHR_ray_tracing_pipeline)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdSetRayTracingPipelineStackSizeKHR ||
            nullptr == fFunctions.fCmdTraceRaysIndirectKHR ||
            nullptr == fFunctions.fCmdTraceRaysKHR ||
            nullptr == fFunctions.fCreateRayTracingPipelinesKHR ||
            
            nullptr == fFunctions.fGetRayTracingCaptureReplayShaderGroupHandlesKHR ||
            nullptr == fFunctions.fGetRayTracingShaderGroupHandlesKHR ||
            nullptr == fFunctions.fGetRayTracingShaderGroupStackSizeKHR)
            return false;
    };
#endif /* defined(VK_KHR_ray_tracing_pipeline) */

#if defined(VK_KHR_sampler_ycbcr_conversion)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCreateSamplerYcbcrConversionKHR ||
            nullptr == fFunctions.fDestroySamplerYcbcrConversionKHR)
            return false;
    };
#endif /* defined(VK_KHR_sampler_ycbcr_conversion) */

#if defined(VK_KHR_shared_presentable_image)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetSwapchainStatusKHR)
            return false;
    };
#endif /* defined(VK_KHR_shared_presentable_image) */

#if defined(VK_KHR_swapchain)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fAcquireNextImageKHR ||
            nullptr == fFunctions.fCreateSwapchainKHR ||
            nullptr == fFunctions.fDestroySwapchainKHR ||
            nullptr == fFunctions.fGetSwapchainImagesKHR ||
            nullptr == fFunctions.fQueuePresentKHR)
            return false;
    };
#endif /* defined(VK_KHR_swapchain) */

#if defined(VK_KHR_synchronization2)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdPipelineBarrier2KHR ||
            nullptr == fFunctions.fCmdResetEvent2KHR ||
            nullptr == fFunctions.fCmdSetEvent2KHR ||
            nullptr == fFunctions.fCmdWaitEvents2KHR ||
            nullptr == fFunctions.fCmdWriteTimestamp2KHR ||
            nullptr == fFunctions.fQueueSubmit2KHR)
            return false;
    };
#endif /* defined(VK_KHR_synchronization2) */

#if defined(VK_KHR_synchronization2) && defined(VK_AMD_buffer_marker)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdWriteBufferMarker2AMD)
            return false;
    };
#endif /* defined(VK_KHR_synchronization2) && defined(VK_AMD_buffer_marker) */

#if defined(VK_KHR_synchronization2) && defined(VK_NV_device_diagnostic_checkpoints)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetQueueCheckpointData2NV)
            return false;
    };
#endif /* defined(VK_KHR_synchronization2) && defined(VK_NV_device_diagnostic_checkpoints) */

#if defined(VK_KHR_timeline_semaphore)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetSemaphoreCounterValueKHR ||
            nullptr == fFunctions.fSignalSemaphoreKHR ||
            nullptr == fFunctions.fWaitSemaphoresKHR)
            return false;
    };
#endif /* defined(VK_KHR_timeline_semaphore) */

#if defined(VK_KHR_video_decode_queue)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdDecodeVideoKHR)
            return false;
    };
#endif /* defined(VK_KHR_video_decode_queue) */

#if defined(VK_KHR_video_encode_queue)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdEncodeVideoKHR)
            return false;
    };
#endif /* defined(VK_KHR_video_encode_queue) */

#if defined(VK_KHR_video_queue)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fBindVideoSessionMemoryKHR ||
            nullptr == fFunctions.fCmdBeginVideoCodingKHR ||
            nullptr == fFunctions.fCmdControlVideoCodingKHR ||
            nullptr == fFunctions.fCmdEndVideoCodingKHR ||
            
            nullptr == fFunctions.fCreateVideoSessionKHR ||
            nullptr == fFunctions.fCreateVideoSessionParametersKHR ||
            nullptr == fFunctions.fDestroyVideoSessionKHR ||
            nullptr == fFunctions.fDestroyVideoSessionParametersKHR ||
            
            nullptr == fFunctions.fGetVideoSessionMemoryRequirementsKHR ||
            nullptr == fFunctions.fUpdateVideoSessionParametersKHR)
            return false;
    };
#endif /* defined(VK_KHR_video_queue) */

#if defined(VK_NVX_binary_import)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdCuLaunchKernelNVX ||
            nullptr == fFunctions.fCreateCuFunctionNVX ||
            nullptr == fFunctions.fCreateCuModuleNVX ||
            nullptr == fFunctions.fDestroyCuFunctionNVX ||
            nullptr == fFunctions.fDestroyCuModuleNVX)
            return false;
    };
#endif /* defined(VK_NVX_binary_import) */

#if defined(VK_NVX_image_view_handle)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetImageViewAddressNVX ||
            nullptr == fFunctions.fGetImageViewHandleNVX)
            return false;
    };
#endif /* defined(VK_NVX_image_view_handle) */

#if defined(VK_NV_clip_space_w_scaling)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdSetViewportWScalingNV)
            return false;
    };
#endif /* defined(VK_NV_clip_space_w_scaling) */

#if defined(VK_NV_device_diagnostic_checkpoints)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdSetCheckpointNV ||
            nullptr == fFunctions.fGetQueueCheckpointDataNV)
            return false;
    };
#endif /* defined(VK_NV_device_diagnostic_checkpoints) */

#if defined(VK_NV_device_generated_commands)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdBindPipelineShaderGroupNV ||
            nullptr == fFunctions.fCmdExecuteGeneratedCommandsNV ||
            nullptr == fFunctions.fCmdPreprocessGeneratedCommandsNV ||
            nullptr == fFunctions.fCreateIndirectCommandsLayoutNV ||
            nullptr == fFunctions.fDestroyIndirectCommandsLayoutNV ||
            nullptr == fFunctions.fGetGeneratedCommandsMemoryRequirementsNV)
            return false;
    };
#endif /* defined(VK_NV_device_generated_commands) */

#if defined(VK_NV_external_memory_rdma)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetMemoryRemoteAddressNV)
            return false;
    };
#endif /* defined(VK_NV_external_memory_rdma) */

#if defined(VK_NV_external_memory_win32)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetMemoryWin32HandleNV)
            return false;
    };
#endif /* defined(VK_NV_external_memory_win32) */

#if defined(VK_NV_fragment_shading_rate_enums)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdSetFragmentShadingRateEnumNV)
            return false;
    };
#endif /* defined(VK_NV_fragment_shading_rate_enums) */

#if defined(VK_NV_mesh_shader)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdDrawMeshTasksIndirectCountNV ||
            nullptr == fFunctions.fCmdDrawMeshTasksIndirectNV ||
            nullptr == fFunctions.fCmdDrawMeshTasksNV)
            return false;
    };
#endif /* defined(VK_NV_mesh_shader) */

#if defined(VK_NV_ray_tracing)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fBindAccelerationStructureMemoryNV ||
            nullptr == fFunctions.fCmdBuildAccelerationStructureNV ||
            nullptr == fFunctions.fCmdCopyAccelerationStructureNV ||
            nullptr == fFunctions.fCmdTraceRaysNV ||
            
            nullptr == fFunctions.fCmdWriteAccelerationStructuresPropertiesNV ||
            nullptr == fFunctions.fCompileDeferredNV ||
            nullptr == fFunctions.fCreateAccelerationStructureNV ||
            nullptr == fFunctions.fCreateRayTracingPipelinesNV ||
            
            nullptr == fFunctions.fDestroyAccelerationStructureNV ||
            nullptr == fFunctions.fGetAccelerationStructureHandleNV ||
            nullptr == fFunctions.fGetAccelerationStructureMemoryRequirementsNV ||
            nullptr == fFunctions.fGetRayTracingShaderGroupHandlesNV)
            return false;
    };
#endif /* defined(VK_NV_ray_tracing) */

#if defined(VK_NV_scissor_exclusive)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdSetExclusiveScissorNV)
            return false;
    };
#endif /* defined(VK_NV_scissor_exclusive) */

#if defined(VK_NV_shading_rate_image)
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdBindShadingRateImageNV ||
            nullptr == fFunctions.fCmdSetCoarseSampleOrderNV ||
            nullptr == fFunctions.fCmdSetViewportShadingRatePaletteNV)
            return false;
    };
#endif /* defined(VK_NV_shading_rate_image) */

#if (defined(VK_EXT_full_screen_exclusive) && defined(VK_KHR_device_group)) || (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1))
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetDeviceGroupSurfacePresentModes2EXT)
            return false;
    };
#endif /* (defined(VK_EXT_full_screen_exclusive) && defined(VK_KHR_device_group)) || (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1)) */

#if (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_push_descriptor) && defined(VK_KHR_descriptor_update_template))
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fCmdPushDescriptorSetWithTemplateKHR)
            return false;
    };
#endif /* (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_push_descriptor) && defined(VK_KHR_descriptor_update_template)) */

#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fGetDeviceGroupPresentCapabilitiesKHR ||
            nullptr == fFunctions.fGetDeviceGroupSurfacePresentModesKHR)
            return false;
    };
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */

#if (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
    if (extensions->hasExtension(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, physicalDeviceVersion))
    {
        if (nullptr == fFunctions.fAcquireNextImage2KHR)
            return false;
    };
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */

    return true;
};

VulkanInterface::VulkanInterface(DiligentGetProc getProc, VkInstance instance, VkDevice device, uint32_t instanceVersion, uint32_t physicalDeviceVersion, const VulkanExtensions* extensions)
{
    if (getProc == nullptr)
    {
        return;
    }
    // Global/Loader Procs.
    loadGlobalFunctions(getProc);

    // Instance Procs.
    loadInstanceFunctions(getProc, instance);

    // Device Procs.
    loadDeviceLevel(getProc, device);
};

VulkanInterface::VulkanInterface()
{

};

bool VulkanInterface::validate(uint32_t instanceVersion, uint32_t physicalDeviceVersion, const VulkanExtensions* extensions)
{
    bool correctly_setup = true;

    correctly_setup &= validateGlobalFunctions(instanceVersion);
    correctly_setup &= validateInstanceFunctions(instanceVersion, physicalDeviceVersion, extensions);
    correctly_setup &= validateDeviceFunctions(instanceVersion, physicalDeviceVersion, extensions);

    return correctly_setup;
};

bool loadVulkanDll(PFN_vkGetInstanceProcAddr& GetInstanceProcAddr)
{
#if defined(_WIN32)
    HMODULE module = LoadLibraryA("vulkan-1.dll");
    if (!module)
        return false;

    // note: function pointer is cast through void function pointer to silence cast-function-type warning on gcc8
    GetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)(void (*)(void))GetProcAddress(module, "vkGetInstanceProcAddr");
#elif defined(__APPLE__)
    void* module = dlopen("libvulkan.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!module)
        module = dlopen("libvulkan.1.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!module)
        module = dlopen("libMoltenVK.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!module)
        return false;

    GetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(module, "vkGetInstanceProcAddr");
#else
    void* module = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!module)
        module = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!module)
        return false;

    GetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(module, "vkGetInstanceProcAddr");
#endif
    return true;
}

} // namespace VulkanUtilities