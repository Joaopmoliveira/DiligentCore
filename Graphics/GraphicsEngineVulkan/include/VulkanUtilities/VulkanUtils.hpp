#ifndef EngineVkUtils_DEFINED
#define EngineVkUtils_DEFINED

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <functional>
#include <string>
#include <vector>

// Diligent interface to the Vulkan API

#if defined DILIGENT_VULKAN_STATIC
#    define DILIGENT_VK_CALL(X) vk##X;
#else
#    define DILIGENT_VK_CALL(X) VulkanUtilities::diligent_vk_interface.fFunctions.f##X;
#endif



namespace VulkanUtilities
{

/*
Function used to obtain pointers to the vulkan functions throught 
the PFN_vkGetProcAdress pointer. Usefull to append the engine to 
a user supplied instance and device.
*/
using DiligentGetProc = std::function<PFN_vkVoidFunction(
    const char*, // function name
    VkInstance,  // instance or VK_NULL_HANDLE
    VkDevice     // device or VK_NULL_HANDLE
    )>;


/**
 * Helper class that eats in an array of extensions strings for instance and device and allows for
 * quicker querying if an extension is present.
 */
class VulkanExtensions
{
public:
    VulkanExtensions() {}

    /*
    Initialize the extensions which are available for the Diligent engine.
    */
    void init(DiligentGetProc, VkInstance, VkPhysicalDevice, uint32_t instanceExtensionCount, const char* const* instanceExtensions, uint32_t deviceExtensionCount, const char* const* deviceExtensions);

    /*
    If a given algorithm requires an extension with a given version
    then the class can query if the extensions provided are compatible 
    with the requested extensions.
    */
    bool hasExtension(const char* , uint32_t minVersion) const;

    struct Info
    {
        Info() {}
        Info(const char* name) :
            fName(name), fSpecVersion(0) {}

        std::string fName;
        uint32_t fSpecVersion;
    };

private:
    void getSpecVersions(DiligentGetProc getProc, VkInstance, VkPhysicalDevice);

    std::vector<Info> fExtensions;
};

/**
 * Diligent uses the following interface to make all calls into Vulkan. When a
 * the user creates a diligent engine with his own vulkan device and instance they must provide a VulkanInterface. All functions that should be
 * available based on the Vulkan's version must be non-NULL or VulkanInterface creation
 * will fail. This can be tested with the validate() method.
 */
struct VulkanInterface
{
private:

    // simple wrapper class that exists only to initialize a pointer to NULL
    template <typename FNPTR_TYPE> class VkPtr
    {
    public:
        VkPtr() :
            fPtr(NULL) {}
        VkPtr operator=(FNPTR_TYPE ptr)
        {
            fPtr = ptr;
            return *this;
        }
        operator FNPTR_TYPE() const { return fPtr; }

    private:
        FNPTR_TYPE fPtr;
    };

    /*
    Load from the getProc functions all the pointers related with global vulkan functions
    */
    void loadGlobalFunctions(DiligentGetProc getProc);

    /*
    Validade Global Functions
    */
    bool validateGlobalFunctions(uint32_t instanceVersion);

    /*
    Load from the getProc functions all the pointers related with instance level vulkan functions 
    */
    void loadInstanceFunctions(DiligentGetProc getProc, VkInstance instance);

    /*
    Validade Global Functions
    */
    bool validateInstanceFunctions(uint32_t instanceVersion, uint32_t physicalDeviceVersion, const VulkanExtensions* extensions);

    /*
    Load from the getProc function all the pointers related with device level vulkan functions
    */
    void loadDeviceLevel(DiligentGetProc getProc, VkDevice device);

    /*
    Validade Global Functions
    */
    bool validateDeviceFunctions(uint32_t instanceVersion, uint32_t physicalDeviceVersion, const VulkanExtensions* extensions);

public:
    VulkanInterface(DiligentGetProc       getProc,
                  VkInstance            instance,
                  VkDevice              device,
                  uint32_t              instanceVersion,
                  uint32_t              physicalDeviceVersion,
                  const VulkanExtensions* extensions);


    VulkanInterface();




    // Validates that the GrVkInterface supports its advertised standard. This means the necessary
    // function pointers have been initialized for Vulkan version.
    bool validate(uint32_t instanceVersion, uint32_t physicalDeviceVersion, const VulkanExtensions* extensions);

    /**
     * The function pointers are in a struct so that we can have a compiler generated assignment
     * operator.
     */
    struct Functions
    {
    ////////////////// Global Level funtions //////////////////
#if defined(VK_VERSION_1_0)
    VkPtr<PFN_vkCreateInstance> fCreateInstance;
    VkPtr<PFN_vkEnumerateInstanceExtensionProperties> fEnumerateInstanceExtensionProperties;
    VkPtr<PFN_vkEnumerateInstanceLayerProperties>     fEnumerateInstanceLayerProperties;
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
        VkPtr<PFN_vkEnumerateInstanceVersion> fEnumerateInstanceVersion;
#endif /* defined(VK_VERSION_1_1) */

    ////////////////// Instance level functions //////////////////
#if defined(VK_VERSION_1_0)
        VkPtr<PFN_vkCreateDevice> fCreateDevice;
        VkPtr<PFN_vkDestroyInstance> fDestroyInstance;
        VkPtr<PFN_vkEnumerateDeviceExtensionProperties>           fEnumerateDeviceExtensionProperties;
        VkPtr<PFN_vkEnumerateDeviceLayerProperties>               fEnumerateDeviceLayerProperties;
        VkPtr<PFN_vkEnumeratePhysicalDevices>                     fEnumeratePhysicalDevices;
        VkPtr<PFN_vkGetDeviceProcAddr>                            fGetDeviceProcAddr;
        VkPtr<PFN_vkGetPhysicalDeviceFeatures>                    fGetPhysicalDeviceFeatures;
        VkPtr<PFN_vkGetPhysicalDeviceFormatProperties>            fGetPhysicalDeviceFormatProperties;
        VkPtr<PFN_vkGetPhysicalDeviceImageFormatProperties>       fGetPhysicalDeviceImageFormatProperties;
        VkPtr<PFN_vkGetPhysicalDeviceQueueFamilyProperties>       fGetPhysicalDeviceQueueFamilyProperties;
        VkPtr<PFN_vkGetPhysicalDeviceSparseImageFormatProperties> fGetPhysicalDeviceSparseImageFormatProperties;
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
        VkPtr<PFN_vkEnumeratePhysicalDeviceGroups>                fEnumeratePhysicalDeviceGroups;
        VkPtr<PFN_vkGetPhysicalDeviceExternalBufferProperties>    fGetPhysicalDeviceExternalBufferProperties;
        VkPtr<PFN_vkGetPhysicalDeviceExternalFenceProperties>     fGetPhysicalDeviceExternalFenceProperties;
        VkPtr<PFN_vkGetPhysicalDeviceExternalSemaphoreProperties> fGetPhysicalDeviceExternalSemaphoreProperties;
        VkPtr<PFN_vkGetPhysicalDeviceFeatures2>                   fGetPhysicalDeviceFeatures2;

        VkPtr<PFN_vkGetPhysicalDeviceFormatProperties2>      fGetPhysicalDeviceFormatProperties2;
        VkPtr<PFN_vkGetPhysicalDeviceImageFormatProperties2> fGetPhysicalDeviceImageFormatProperties2;
        VkPtr<PFN_vkGetPhysicalDeviceMemoryProperties2>      fGetPhysicalDeviceMemoryProperties2;
        VkPtr<PFN_vkGetPhysicalDeviceProperties2>            fGetPhysicalDeviceProperties2;
        VkPtr<PFN_vkGetPhysicalDeviceQueueFamilyProperties2> fGetPhysicalDeviceQueueFamilyProperties2;

        VkPtr<PFN_vkGetPhysicalDeviceSparseImageFormatProperties2> fGetPhysicalDeviceSparseImageFormatProperties2;
#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_EXT_acquire_drm_display)
        VkPtr<PFN_vkAcquireDrmDisplayEXT> fAcquireDrmDisplayEXT;
        VkPtr<PFN_vkGetDrmDisplayEXT>     fGetDrmDisplayEXT;
#endif /* defined(VK_EXT_acquire_drm_display) */
#if defined(VK_EXT_acquire_xlib_display)
        VkPtr<PFN_vkAcquireXlibDisplayEXT>    fAcquireXlibDisplayEXT;
        VkPtr<PFN_vkGetRandROutputDisplayEXT> fGetRandROutputDisplayEXT;
#endif /* defined(VK_EXT_acquire_xlib_display) */
#if defined(VK_EXT_calibrated_timestamps)
        VkPtr<PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT> fGetPhysicalDeviceCalibrateableTimeDomainsEXT;
#endif /* defined(VK_EXT_calibrated_timestamps) */
#if defined(VK_EXT_debug_report)
        VkPtr<PFN_vkCreateDebugReportCallbackEXT>  fCreateDebugReportCallbackEXT;
        VkPtr<PFN_vkDebugReportMessageEXT>         fDebugReportMessageEXT;
        VkPtr<PFN_vkDestroyDebugReportCallbackEXT> fDestroyDebugReportCallbackEXT;
#endif /* defined(VK_EXT_debug_report) */
#if defined(VK_EXT_debug_utils)
        VkPtr<PFN_vkCmdBeginDebugUtilsLabelEXT>  fCmdBeginDebugUtilsLabelEXT;
        VkPtr<PFN_vkCmdEndDebugUtilsLabelEXT>    fCmdEndDebugUtilsLabelEXT;
        VkPtr<PFN_vkCmdInsertDebugUtilsLabelEXT> fCmdInsertDebugUtilsLabelEXT;

        VkPtr<PFN_vkCreateDebugUtilsMessengerEXT>  fCreateDebugUtilsMessengerEXT;
        VkPtr<PFN_vkDestroyDebugUtilsMessengerEXT> fDestroyDebugUtilsMessengerEXT;
        VkPtr<PFN_vkQueueBeginDebugUtilsLabelEXT>  fQueueBeginDebugUtilsLabelEXT;

        VkPtr<PFN_vkQueueEndDebugUtilsLabelEXT>    fQueueEndDebugUtilsLabelEXT;
        VkPtr<PFN_vkQueueInsertDebugUtilsLabelEXT> fQueueInsertDebugUtilsLabelEXT;
        VkPtr<PFN_vkSetDebugUtilsObjectNameEXT>    fSetDebugUtilsObjectNameEXT;

        VkPtr<PFN_vkSetDebugUtilsObjectTagEXT>  fSetDebugUtilsObjectTagEXT;
        VkPtr<PFN_vkSubmitDebugUtilsMessageEXT> fSubmitDebugUtilsMessageEXT;

#endif /* defined(VK_EXT_debug_utils) */
#if defined(VK_EXT_direct_mode_display)
        VkPtr<PFN_vkReleaseDisplayEXT> fReleaseDisplayEXT;
#endif /* defined(VK_EXT_direct_mode_display) */
#if defined(VK_EXT_directfb_surface)
        VkPtr<PFN_vkCreateDirectFBSurfaceEXT>                        fCreateDirectFBSurfaceEXT;
        VkPtr<PFN_vkGetPhysicalDeviceDirectFBPresentationSupportEXT> fGetPhysicalDeviceDirectFBPresentationSupportEXT;
#endif /* defined(VK_EXT_directfb_surface) */
#if defined(VK_EXT_display_surface_counter)
        VkPtr<PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT> fGetPhysicalDeviceSurfaceCapabilities2EXT;
#endif /* defined(VK_EXT_display_surface_counter) */
#if defined(VK_EXT_full_screen_exclusive)
        VkPtr<PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT> fGetPhysicalDeviceSurfacePresentModes2EXT;
#endif /* defined(VK_EXT_full_screen_exclusive) */
#if defined(VK_EXT_headless_surface)
        VkPtr<PFN_vkCreateHeadlessSurfaceEXT> fCreateHeadlessSurfaceEXT;
#endif /* defined(VK_EXT_headless_surface) */
#if defined(VK_EXT_metal_surface)
        VkPtr<PFN_vkCreateMetalSurfaceEXT> fCreateMetalSurfaceEXT;
#endif /* defined(VK_EXT_metal_surface) */
#if defined(VK_EXT_sample_locations)
        VkPtr<PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT> fGetPhysicalDeviceMultisamplePropertiesEXT;
#endif /* defined(VK_EXT_sample_locations) */
#if defined(VK_EXT_tooling_info)
        VkPtr<PFN_vkGetPhysicalDeviceToolPropertiesEXT> fGetPhysicalDeviceToolPropertiesEXT;
#endif /* defined(VK_EXT_tooling_info) */
#if defined(VK_FUCHSIA_imagepipe_surface)
        VkPtr<PFN_vkCreateImagePipeSurfaceFUCHSIA> fCreateImagePipeSurfaceFUCHSIA;
#endif /* defined(VK_FUCHSIA_imagepipe_surface) */
#if defined(VK_GGP_stream_descriptor_surface)
        VkPtr<PFN_vkCreateStreamDescriptorSurfaceGGP> fCreateStreamDescriptorSurfaceGGP;
#endif /* defined(VK_GGP_stream_descriptor_surface) */
#if defined(VK_KHR_android_surface)
        VkPtr<PFN_vkCreateAndroidSurfaceKHR> fCreateAndroidSurfaceKHR;
#endif /* defined(VK_KHR_android_surface) */
#if defined(VK_KHR_device_group_creation)
        VkPtr<PFN_vkEnumeratePhysicalDeviceGroupsKHR> fEnumeratePhysicalDeviceGroupsKHR;
#endif /* defined(VK_KHR_device_group_creation) */
#if defined(VK_KHR_display)
        VkPtr<PFN_vkCreateDisplayModeKHR>                       fCreateDisplayModeKHR;
        VkPtr<PFN_vkCreateDisplayPlaneSurfaceKHR>               fCreateDisplayPlaneSurfaceKHR;
        VkPtr<PFN_vkGetDisplayModePropertiesKHR>                fGetDisplayModePropertiesKHR;
        VkPtr<PFN_vkGetDisplayPlaneCapabilitiesKHR>             fGetDisplayPlaneCapabilitiesKHR;
        VkPtr<PFN_vkGetDisplayPlaneSupportedDisplaysKHR>        fGetDisplayPlaneSupportedDisplaysKHR;
        VkPtr<PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR> fGetPhysicalDeviceDisplayPlanePropertiesKHR;
        VkPtr<PFN_vkGetPhysicalDeviceDisplayPropertiesKHR>      fGetPhysicalDeviceDisplayPropertiesKHR;
#endif /* defined(VK_KHR_display) */
#if defined(VK_KHR_external_fence_capabilities)
        VkPtr<PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR> fGetPhysicalDeviceExternalFencePropertiesKHR;
#endif /* defined(VK_KHR_external_fence_capabilities) */
#if defined(VK_KHR_external_memory_capabilities)
        VkPtr<PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR> fGetPhysicalDeviceExternalBufferPropertiesKHR;
#endif /* defined(VK_KHR_external_memory_capabilities) */
#if defined(VK_KHR_external_semaphore_capabilities)
        VkPtr<PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR> fGetPhysicalDeviceExternalSemaphorePropertiesKHR;
#endif /* defined(VK_KHR_external_semaphore_capabilities) */
#if defined(VK_KHR_fragment_shading_rate)
        VkPtr<PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR> fGetPhysicalDeviceFragmentShadingRatesKHR;
#endif /* defined(VK_KHR_fragment_shading_rate) */
#if defined(VK_KHR_get_display_properties2)
        VkPtr<PFN_vkGetDisplayModeProperties2KHR>                fGetDisplayModeProperties2KHR;
        VkPtr<PFN_vkGetDisplayPlaneCapabilities2KHR>             fGetDisplayPlaneCapabilities2KHR;
        VkPtr<PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR> fGetPhysicalDeviceDisplayPlaneProperties2KHR;
        VkPtr<PFN_vkGetPhysicalDeviceDisplayProperties2KHR>      fGetPhysicalDeviceDisplayProperties2KHR;
#endif /* defined(VK_KHR_get_display_properties2) */
#if defined(VK_KHR_get_physical_device_properties2)
        VkPtr<PFN_vkGetPhysicalDeviceFeatures2KHR>                    fGetPhysicalDeviceFeatures2KHR;
        VkPtr<PFN_vkGetPhysicalDeviceFormatProperties2KHR>            fGetPhysicalDeviceFormatProperties2KHR;
        VkPtr<PFN_vkGetPhysicalDeviceImageFormatProperties2KHR>       fGetPhysicalDeviceImageFormatProperties2KHR;
        VkPtr<PFN_vkGetPhysicalDeviceMemoryProperties2KHR>            fGetPhysicalDeviceMemoryProperties2KHR;
        VkPtr<PFN_vkGetPhysicalDeviceProperties2KHR>                  fGetPhysicalDeviceProperties2KHR;
        VkPtr<PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR>       fGetPhysicalDeviceQueueFamilyProperties2KHR;
        VkPtr<PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR> fGetPhysicalDeviceSparseImageFormatProperties2KHR;
#endif /* defined(VK_KHR_get_physical_device_properties2) */
#if defined(VK_KHR_get_surface_capabilities2)
        VkPtr<PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR> fGetPhysicalDeviceSurfaceCapabilities2KHR;
        VkPtr<PFN_vkGetPhysicalDeviceSurfaceFormats2KHR>      fGetPhysicalDeviceSurfaceFormats2KHR;
#endif /* defined(VK_KHR_get_surface_capabilities2) */
#if defined(VK_KHR_performance_query)
        VkPtr<PFN_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR> fEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR;
        VkPtr<PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR>         fGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR;
#endif /* defined(VK_KHR_performance_query) */
#if defined(VK_KHR_surface)
        VkPtr<PFN_vkDestroySurfaceKHR>                       fDestroySurfaceKHR;
        VkPtr<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR> fGetPhysicalDeviceSurfaceCapabilitiesKHR;
        VkPtr<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR> fGetPhysicalDeviceSurfacePresentModesKHR;
        VkPtr<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>      fGetPhysicalDeviceSurfaceFormatsKHR;
        VkPtr<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>      fGetPhysicalDeviceSurfaceSupportKHR;
#endif /* defined(VK_KHR_surface) */
#if defined(VK_KHR_video_queue)
        VkPtr<PFN_vkGetPhysicalDeviceVideoCapabilitiesKHR>     fGetPhysicalDeviceVideoCapabilitiesKHR;
        VkPtr<PFN_vkGetPhysicalDeviceVideoFormatPropertiesKHR> fGetPhysicalDeviceVideoFormatPropertiesKHR;
#endif /* defined(VK_KHR_video_queue) */
#if defined(VK_KHR_wayland_surface)
        VkPtr<PFN_vkCreateWaylandSurfaceKHR>                        fCreateWaylandSurfaceKHR;
        VkPtr<PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR> fGetPhysicalDeviceWaylandPresentationSupportKHR;
#endif /* defined(VK_KHR_wayland_surface) */
#if defined(VK_KHR_win32_surface)
        VkPtr<PFN_vkCreateWin32SurfaceKHR>                        fCreateWin32SurfaceKHR;
        VkPtr<PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR> fGetPhysicalDeviceWin32PresentationSupportKHR;
#endif /* defined(VK_KHR_win32_surface) */
#if defined(VK_KHR_xcb_surface)
        VkPtr<PFN_vkCreateXcbSurfaceKHR>                        fCreateXcbSurfaceKHR;
        VkPtr<PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR> fGetPhysicalDeviceXcbPresentationSupportKHR;
#endif /* defined(VK_KHR_xcb_surface) */
#if defined(VK_KHR_xlib_surface)
        VkPtr<PFN_vkCreateXlibSurfaceKHR>                        fCreateXlibSurfaceKHR;
        VkPtr<PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR> fGetPhysicalDeviceXlibPresentationSupportKHR;
#endif /* defined(VK_KHR_xlib_surface) */
#if defined(VK_MVK_ios_surface)
        VkPtr<PFN_vkCreateIOSSurfaceMVK> fCreateIOSSurfaceMVK;
#endif /* defined(VK_MVK_ios_surface) */
#if defined(VK_MVK_macos_surface)
        VkPtr<PFN_vkCreateMacOSSurfaceMVK> fCreateMacOSSurfaceMVK;
#endif /* defined(VK_MVK_macos_surface) */
#if defined(VK_NN_vi_surface)
        VkPtr<PFN_vkCreateViSurfaceNN> fCreateViSurfaceNN;
#endif /* defined(VK_NN_vi_surface) */
#if defined(VK_NV_acquire_winrt_display)
        VkPtr<PFN_vkAcquireWinrtDisplayNV> fAcquireWinrtDisplayNV;
        VkPtr<PFN_vkGetWinrtDisplayNV>     fGetWinrtDisplayNV;
#endif /* defined(VK_NV_acquire_winrt_display) */
#if defined(VK_NV_cooperative_matrix)
        VkPtr<PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV> fGetPhysicalDeviceCooperativeMatrixPropertiesNV;
#endif /* defined(VK_NV_cooperative_matrix) */
#if defined(VK_NV_coverage_reduction_mode)
        VkPtr<PFN_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV> fGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV;
#endif /* defined(VK_NV_coverage_reduction_mode) */
#if defined(VK_NV_external_memory_capabilities)
        VkPtr<PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV> fGetPhysicalDeviceExternalImageFormatPropertiesNV;
#endif /* defined(VK_NV_external_memory_capabilities) */
#if defined(VK_QNX_screen_surface)
        VkPtr<PFN_vkCreateScreenSurfaceQNX>                        fCreateScreenSurfaceQNX;
        VkPtr<PFN_vkGetPhysicalDeviceScreenPresentationSupportQNX> fGetPhysicalDeviceScreenPresentationSupportQNX;
#endif /* defined(VK_QNX_screen_surface) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
        VkPtr<PFN_vkGetPhysicalDevicePresentRectanglesKHR> fGetPhysicalDevicePresentRectanglesKHR;
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */

        ////////////////// Device level functions //////////////////
#if defined(VK_VERSION_1_0)
        VkPtr<PFN_vkAllocateCommandBuffers>           fAllocateCommandBuffers;
        VkPtr<PFN_vkAllocateDescriptorSets>           fAllocateDescriptorSets;
        VkPtr<PFN_vkAllocateMemory>                   fAllocateMemory;
        VkPtr<PFN_vkBeginCommandBuffer>               fBeginCommandBuffer;
        VkPtr<PFN_vkBindBufferMemory>                 fBindBufferMemory;
        VkPtr<PFN_vkBindImageMemory>                  fBindImageMemory;
        VkPtr<PFN_vkCmdBeginQuery>                    fCmdBeginQuery;
        VkPtr<PFN_vkCmdBeginRenderPass>               fCmdBeginRenderPass;
        VkPtr<PFN_vkCmdBindDescriptorSets>            fCmdBindDescriptorSets;
        VkPtr<PFN_vkCmdBindIndexBuffer>               fCmdBindIndexBuffer;
        VkPtr<PFN_vkCmdBindPipeline>                  fCmdBindPipeline;
        VkPtr<PFN_vkCmdBindVertexBuffers>             fCmdBindVertexBuffers;
        VkPtr<PFN_vkCmdBlitImage>                     fCmdBlitImage;
        VkPtr<PFN_vkCmdClearAttachments>              fCmdClearAttachments;
        VkPtr<PFN_vkCmdClearColorImage>               fCmdClearColorImage;
        VkPtr<PFN_vkCmdClearDepthStencilImage>        fCmdClearDepthStencilImage;
        VkPtr<PFN_vkCmdCopyBuffer>                    fCmdCopyBuffer;
        VkPtr<PFN_vkCmdCopyBufferToImage>             fCmdCopyBufferToImage;
        VkPtr<PFN_vkCmdCopyImage>                     fCmdCopyImage;
        VkPtr<PFN_vkCmdCopyImageToBuffer>             fCmdCopyImageToBuffer;
        VkPtr<PFN_vkCmdCopyQueryPoolResults>          fCmdCopyQueryPoolResults;
        VkPtr<PFN_vkCmdDispatch>                      fCmdDispatch;
        VkPtr<PFN_vkCmdDispatchIndirect>              fCmdDispatchIndirect;
        VkPtr<PFN_vkCmdDraw>                          fCmdDraw;
        VkPtr<PFN_vkCmdDrawIndexed>                   fCmdDrawIndexed;
        VkPtr<PFN_vkCmdDrawIndexedIndirect>           fCmdDrawIndexedIndirect;
        VkPtr<PFN_vkCmdDrawIndirect>                  fCmdDrawIndirect;
        VkPtr<PFN_vkCmdEndQuery>                      fCmdEndQuery;
        VkPtr<PFN_vkCmdEndRenderPass>                 fCmdEndRenderPass;
        VkPtr<PFN_vkCmdExecuteCommands>               fCmdExecuteCommands;
        VkPtr<PFN_vkCmdFillBuffer>                    fCmdFillBuffer;
        VkPtr<PFN_vkCmdNextSubpass>                   fCmdNextSubpass;
        VkPtr<PFN_vkCmdPipelineBarrier>               fCmdPipelineBarrier;
        VkPtr<PFN_vkCmdPushConstants>                 fCmdPushConstants;
        VkPtr<PFN_vkCmdResetEvent>                    fCmdResetEvent;
        VkPtr<PFN_vkCmdResetQueryPool>                fCmdResetQueryPool;
        VkPtr<PFN_vkCmdResolveImage>                  fCmdResolveImage;
        VkPtr<PFN_vkCmdSetBlendConstants>             fCmdSetBlendConstants;
        VkPtr<PFN_vkCmdSetDepthBias>                  fCmdSetDepthBias;
        VkPtr<PFN_vkCmdSetDepthBounds>                fCmdSetDepthBounds;
        VkPtr<PFN_vkCmdSetEvent>                      fCmdSetEvent;
        VkPtr<PFN_vkCmdSetLineWidth>                  fCmdSetLineWidth;
        VkPtr<PFN_vkCmdSetScissor>                    fCmdSetScissor;
        VkPtr<PFN_vkCmdSetStencilCompareMask>         fCmdSetStencilCompareMask;
        VkPtr<PFN_vkCmdSetStencilReference>           fCmdSetStencilReference;
        VkPtr<PFN_vkCmdSetStencilWriteMask>           fCmdSetStencilWriteMask;
        VkPtr<PFN_vkCmdSetViewport>                   fCmdSetViewport;
        VkPtr<PFN_vkCmdUpdateBuffer>                  fCmdUpdateBuffer;
        VkPtr<PFN_vkCmdWaitEvents>                    fCmdWaitEvents;
        VkPtr<PFN_vkCmdWriteTimestamp>                fCmdWriteTimestamp;
        VkPtr<PFN_vkCreateBuffer>                     fCreateBuffer;
        VkPtr<PFN_vkCreateBufferView>                 fCreateBufferView;
        VkPtr<PFN_vkCreateCommandPool>                fCreateCommandPool;
        VkPtr<PFN_vkCreateComputePipelines>           fCreateComputePipelines;
        VkPtr<PFN_vkCreateDescriptorPool>             fCreateDescriptorPool;
        VkPtr<PFN_vkCreateDescriptorSetLayout>        fCreateDescriptorSetLayout;
        VkPtr<PFN_vkCreateEvent>                      fCreateEvent;
        VkPtr<PFN_vkCreateFence>                      fCreateFence;
        VkPtr<PFN_vkCreateFramebuffer>                fCreateFramebuffer;
        VkPtr<PFN_vkCreateGraphicsPipelines>          fCreateGraphicsPipelines;
        VkPtr<PFN_vkCreateImage>                      fCreateImage;
        VkPtr<PFN_vkCreateImageView>                  fCreateImageView;
        VkPtr<PFN_vkCreatePipelineCache>              fCreatePipelineCache;
        VkPtr<PFN_vkCreatePipelineLayout>             fCreatePipelineLayout;
        VkPtr<PFN_vkCreateQueryPool>                  fCreateQueryPool;
        VkPtr<PFN_vkCreateRenderPass>                 fCreateRenderPass;
        VkPtr<PFN_vkCreateSampler>                    fCreateSampler;
        VkPtr<PFN_vkCreateSemaphore>                  fCreateSemaphore;
        VkPtr<PFN_vkCreateShaderModule>               fCreateShaderModule;
        VkPtr<PFN_vkDestroyBuffer>                    fDestroyBuffer;
        VkPtr<PFN_vkDestroyBufferView>                fDestroyBufferView;
        VkPtr<PFN_vkDestroyCommandPool>               fDestroyCommandPool;
        VkPtr<PFN_vkDestroyDescriptorPool>            fDestroyDescriptorPool;
        VkPtr<PFN_vkDestroyDescriptorSetLayout>       fDestroyDescriptorSetLayout;
        VkPtr<PFN_vkDestroyDevice>                    fDestroyDevice;
        VkPtr<PFN_vkDestroyEvent>                     fDestroyEvent;
        VkPtr<PFN_vkDestroyFence>                     fDestroyFence;
        VkPtr<PFN_vkDestroyFramebuffer>               fDestroyFramebuffer;
        VkPtr<PFN_vkDestroyImage>                     fDestroyImage;
        VkPtr<PFN_vkDestroyImageView>                 fDestroyImageView;
        VkPtr<PFN_vkDestroyPipeline>                  fDestroyPipeline;
        VkPtr<PFN_vkDestroyPipelineCache>             fDestroyPipelineCache;
        VkPtr<PFN_vkDestroyPipelineLayout>            fDestroyPipelineLayout;
        VkPtr<PFN_vkDestroyQueryPool>                 fDestroyQueryPool;
        VkPtr<PFN_vkDestroyRenderPass>                fDestroyRenderPass;
        VkPtr<PFN_vkDestroySampler>                   fDestroySampler;
        VkPtr<PFN_vkDestroySemaphore>                 fDestroySemaphore;
        VkPtr<PFN_vkDestroyShaderModule>              fDestroyShaderModule;
        VkPtr<PFN_vkDeviceWaitIdle>                   fDeviceWaitIdle;
        VkPtr<PFN_vkEndCommandBuffer>                 fEndCommandBuffer;
        VkPtr<PFN_vkFlushMappedMemoryRanges>          fFlushMappedMemoryRanges;
        VkPtr<PFN_vkFreeCommandBuffers>               fFreeCommandBuffers;
        VkPtr<PFN_vkFreeDescriptorSets>               fFreeDescriptorSets;
        VkPtr<PFN_vkFreeMemory>                       fFreeMemory;
        VkPtr<PFN_vkGetBufferMemoryRequirements>      fGetBufferMemoryRequirements;
        VkPtr<PFN_vkGetDeviceMemoryCommitment>        fGetDeviceMemoryCommitment;
        VkPtr<PFN_vkGetDeviceQueue>                   fGetDeviceQueue;
        VkPtr<PFN_vkGetEventStatus>                   fGetEventStatus;
        VkPtr<PFN_vkGetFenceStatus>                   fGetFenceStatus;
        VkPtr<PFN_vkGetImageMemoryRequirements>       fGetImageMemoryRequirements;
        VkPtr<PFN_vkGetImageSparseMemoryRequirements> fGetImageSparseMemoryRequirements;
        VkPtr<PFN_vkGetImageSubresourceLayout>        fGetImageSubresourceLayout;
        VkPtr<PFN_vkGetPipelineCacheData>             fGetPipelineCacheData;
        VkPtr<PFN_vkGetQueryPoolResults>              fGetQueryPoolResults;
        VkPtr<PFN_vkGetRenderAreaGranularity>         fGetRenderAreaGranularity;
        VkPtr<PFN_vkInvalidateMappedMemoryRanges>     fInvalidateMappedMemoryRanges;
        VkPtr<PFN_vkMapMemory>                        fMapMemory;
        VkPtr<PFN_vkMergePipelineCaches>              fMergePipelineCaches;
        VkPtr<PFN_vkQueueBindSparse>                  fQueueBindSparse;
        VkPtr<PFN_vkQueueSubmit>                      fQueueSubmit;
        VkPtr<PFN_vkQueueWaitIdle>                    fQueueWaitIdle;
        VkPtr<PFN_vkResetCommandBuffer>               fResetCommandBuffer;
        VkPtr<PFN_vkResetCommandPool>                 fResetCommandPool;
        VkPtr<PFN_vkResetDescriptorPool>              fResetDescriptorPool;
        VkPtr<PFN_vkResetEvent>                       fResetEvent;
        VkPtr<PFN_vkResetFences>                      fResetFences;
        VkPtr<PFN_vkSetEvent>                         fSetEvent;
        VkPtr<PFN_vkUnmapMemory>                      fUnmapMemory;
        VkPtr<PFN_vkUpdateDescriptorSets>             fUpdateDescriptorSets;
        VkPtr<PFN_vkWaitForFences>                    fWaitForFences;
#endif /* defined(VK_VERSION_1_0) */ f;
#if defined(VK_VERSION_1_1)
        VkPtr<PFN_vkBindBufferMemory2>                 fBindBufferMemory2;
        VkPtr<PFN_vkBindImageMemory2>                  fBindImageMemory2;
        VkPtr<PFN_vkCmdDispatchBase>                   fCmdDispatchBase;
        VkPtr<PFN_vkCmdSetDeviceMask>                  fCmdSetDeviceMask;
        VkPtr<PFN_vkCreateDescriptorUpdateTemplate>    fCreateDescriptorUpdateTemplate;
        VkPtr<PFN_vkCreateSamplerYcbcrConversion>      fCreateSamplerYcbcrConversion;
        VkPtr<PFN_vkDestroyDescriptorUpdateTemplate>   fDestroyDescriptorUpdateTemplate;
        VkPtr<PFN_vkDestroySamplerYcbcrConversion>     fDestroySamplerYcbcrConversion;
        VkPtr<PFN_vkGetBufferMemoryRequirements2>      fGetBufferMemoryRequirements2;
        VkPtr<PFN_vkGetDescriptorSetLayoutSupport>     fGetDescriptorSetLayoutSupport;
        VkPtr<PFN_vkGetDeviceGroupPeerMemoryFeatures>  fGetDeviceGroupPeerMemoryFeatures;
        VkPtr<PFN_vkGetDeviceQueue2>                   fGetDeviceQueue2;
        VkPtr<PFN_vkGetImageMemoryRequirements2>       fGetImageMemoryRequirements2;
        VkPtr<PFN_vkGetImageSparseMemoryRequirements2> fGetImageSparseMemoryRequirements2;
        VkPtr<PFN_vkTrimCommandPool>                   fTrimCommandPool;
        VkPtr<PFN_vkUpdateDescriptorSetWithTemplate>   fUpdateDescriptorSetWithTemplate;
#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_VERSION_1_2)
        VkPtr<PFN_vkCmdBeginRenderPass2>                 fCmdBeginRenderPass2;
        VkPtr<PFN_vkCmdDrawIndexedIndirectCount>         fCmdDrawIndexedIndirectCount;
        VkPtr<PFN_vkCmdDrawIndirectCount>                fCmdDrawIndirectCount;
        VkPtr<PFN_vkCmdEndRenderPass2>                   fCmdEndRenderPass2;
        VkPtr<PFN_vkCmdNextSubpass2>                     fCmdNextSubpass2;
        VkPtr<PFN_vkCreateRenderPass2>                   fCreateRenderPass2;
        VkPtr<PFN_vkGetBufferDeviceAddress>              fGetBufferDeviceAddress;
        VkPtr<PFN_vkGetBufferOpaqueCaptureAddress>       fGetBufferOpaqueCaptureAddress;
        VkPtr<PFN_vkGetDeviceMemoryOpaqueCaptureAddress> fGetDeviceMemoryOpaqueCaptureAddress;
        VkPtr<PFN_vkGetSemaphoreCounterValue>            fGetSemaphoreCounterValue;
        VkPtr<PFN_vkResetQueryPool>                      fResetQueryPool;
        VkPtr<PFN_vkSignalSemaphore>                     fSignalSemaphore;
        VkPtr<PFN_vkWaitSemaphores>                      fWaitSemaphores;
#endif /* defined(VK_VERSION_1_2) */ 
#if defined(VK_AMD_buffer_marker)
        VkPtr<PFN_vkCmdWriteBufferMarkerAMD> fCmdWriteBufferMarkerAMD;
#endif /* defined(VK_AMD_buffer_marker) */
#if defined(VK_AMD_display_native_hdr)
        VkPtr<PFN_vkSetLocalDimmingAMD> fSetLocalDimmingAMD;
#endif /* defined(VK_AMD_display_native_hdr) */
#if defined(VK_AMD_draw_indirect_count)
        VkPtr<PFN_vkCmdDrawIndexedIndirectCountAMD> fCmdDrawIndexedIndirectCountAMD;
        VkPtr<PFN_vkCmdDrawIndirectCountAMD>        fCmdDrawIndirectCountAMD;
#endif /* defined(VK_AMD_draw_indirect_count) */
#if defined(VK_AMD_shader_info)
        VkPtr<PFN_vkGetShaderInfoAMD> fGetShaderInfoAMD;
#endif /* defined(VK_AMD_shader_info) */
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
        VkPtr<PFN_vkGetAndroidHardwareBufferPropertiesANDROID> fGetAndroidHardwareBufferPropertiesANDROID;
        VkPtr<PFN_vkGetAndroidHardwareBufferPropertiesANDROID> fGetAndroidHardwareBufferPropertiesANDROID;
#endif /* defined(VK_ANDROID_external_memory_android_hardware_buffer) */
#if defined(VK_EXT_buffer_device_address)
        VkPtr<PFN_vkGetBufferDeviceAddressEXT> fGetBufferDeviceAddressEXT;
#endif /* defined(VK_EXT_buffer_device_address) */
#if defined(VK_EXT_calibrated_timestamps)
        VkPtr<PFN_vkGetCalibratedTimestampsEXT> fGetCalibratedTimestampsEXT;
#endif /* defined(VK_EXT_calibrated_timestamps) */
#if defined(VK_EXT_color_write_enable)
        VkPtr<PFN_vkCmdSetColorWriteEnableEXT> fCmdSetColorWriteEnableEXT;
#endif /* defined(VK_EXT_color_write_enable) */
#if defined(VK_EXT_conditional_rendering)
        VkPtr<PFN_vkCmdBeginConditionalRenderingEXT> fCmdBeginConditionalRenderingEXT;
        VkPtr<PFN_vkCmdEndConditionalRenderingEXT>   fCmdEndConditionalRenderingEXT;
#endif /* defined(VK_EXT_conditional_rendering) */
#if defined(VK_EXT_debug_marker)
        VkPtr<PFN_vkCmdDebugMarkerBeginEXT>      fCmdDebugMarkerBeginEXT;
        VkPtr<PFN_vkCmdDebugMarkerEndEXT>        fCmdDebugMarkerEndEXT;
        VkPtr<PFN_vkCmdDebugMarkerInsertEXT>     fCmdDebugMarkerInsertEXT;
        VkPtr<PFN_vkDebugMarkerSetObjectNameEXT> fDebugMarkerSetObjectNameEXT;
        VkPtr<PFN_vkDebugMarkerSetObjectTagEXT>  fDebugMarkerSetObjectTagEXT;
#endif /* defined(VK_EXT_debug_marker) */
#if defined(VK_EXT_discard_rectangles)
        VkPtr<PFN_vkCmdSetDiscardRectangleEXT> fCmdSetDiscardRectangleEXT;
#endif /* defined(VK_EXT_discard_rectangles) */
#if defined(VK_EXT_display_control)
        VkPtr<PFN_vkDisplayPowerControlEXT>  fDisplayPowerControlEXT;
        VkPtr<PFN_vkGetSwapchainCounterEXT>  fGetSwapchainCounterEXT;
        VkPtr<PFN_vkRegisterDeviceEventEXT>  fRegisterDeviceEventEXT;
        VkPtr<PFN_vkRegisterDisplayEventEXT> fRegisterDisplayEventEXT;
#endif /* defined(VK_EXT_display_control) */
#if defined(VK_EXT_extended_dynamic_state)
        VkPtr<PFN_vkCmdBindVertexBuffers2EXT>       fCmdBindVertexBuffers2EXT;
        VkPtr<PFN_vkCmdSetCullModeEXT>              fCmdSetCullModeEXT;
        VkPtr<PFN_vkCmdSetDepthBoundsTestEnableEXT> fCmdSetDepthBoundsTestEnableEXT;
        VkPtr<PFN_vkCmdSetDepthCompareOpEXT>        fCmdSetDepthCompareOpEXT;

        VkPtr<PFN_vkCmdSetDepthTestEnableEXT>           fCmdSetDepthTestEnableEXT;
        VkPtr<PFN_vkCmdSetDepthWriteEnableEXT>  fCmdSetDepthWriteEnableEXT;
        VkPtr<PFN_vkCmdSetFrontFaceEXT>                 fCmdSetFrontFaceEXT;
        VkPtr<PFN_vkCmdSetPrimitiveTopologyEXT>         fCmdSetPrimitiveTopologyEXT;

        VkPtr<PFN_vkCmdSetScissorWithCountEXT>          fCmdSetScissorWithCountEXT;
        VkPtr<PFN_vkCmdSetStencilOpEXT>         fCmdSetStencilOpEXT;
        VkPtr<PFN_vkCmdSetStencilTestEnableEXT>         fCmdSetStencilTestEnableEXT;
        VkPtr<PFN_vkCmdSetViewportWithCountEXT>         fCmdSetViewportWithCountEXT;
#endif /* defined(VK_EXT_extended_dynamic_state) */
#if defined(VK_EXT_extended_dynamic_state2)
        VkPtr<PFN_vkCmdSetDepthBiasEnableEXT>         fCmdSetDepthBiasEnableEXT;
        VkPtr<PFN_vkCmdSetLogicOpEXT>                 fCmdSetLogicOpEXT;
        VkPtr<PFN_vkCmdSetPatchControlPointsEXT>      fCmdSetPatchControlPointsEXT;
        VkPtr<PFN_vkCmdSetPrimitiveRestartEnableEXT>  fCmdSetPrimitiveRestartEnableEXT;
        VkPtr<PFN_vkCmdSetRasterizerDiscardEnableEXT> fCmdSetRasterizerDiscardEnableEXT;
#endif /* defined(VK_EXT_extended_dynamic_state2) */
#if defined(VK_EXT_external_memory_host)
        VkPtr<PFN_vkGetMemoryHostPointerPropertiesEXT> fGetMemoryHostPointerPropertiesEXT;
#endif /* defined(VK_EXT_external_memory_host) */
#if defined(VK_EXT_full_screen_exclusive)
        VkPtr<PFN_vkAcquireFullScreenExclusiveModeEXT> fAcquireFullScreenExclusiveModeEXT;
        VkPtr<PFN_vkReleaseFullScreenExclusiveModeEXT> fReleaseFullScreenExclusiveModeEXT;
#endif /* defined(VK_EXT_full_screen_exclusive) */
#if defined(VK_EXT_hdr_metadata)
        VkPtr<PFN_vkSetHdrMetadataEXT> fSetHdrMetadataEXT;
#endif /* defined(VK_EXT_hdr_metadata) */
#if defined(VK_EXT_host_query_reset)
        VkPtr<PFN_vkResetQueryPoolEXT> fResetQueryPoolEXT;
#endif /* defined(VK_EXT_host_query_reset) */
#if defined(VK_EXT_image_drm_format_modifier)
        VkPtr<PFN_vkGetImageDrmFormatModifierPropertiesEXT> fGetImageDrmFormatModifierPropertiesEXT;
#endif /* defined(VK_EXT_image_drm_format_modifier) */
#if defined(VK_EXT_line_rasterization)
        VkPtr<PFN_vkCmdSetLineStippleEXT> fCmdSetLineStippleEXT;
#endif /* defined(VK_EXT_line_rasterization) */
#if defined(VK_EXT_multi_draw)
        VkPtr<PFN_vkCmdDrawMultiEXT>        fCmdDrawMultiEXT;
        VkPtr<PFN_vkCmdDrawMultiIndexedEXT> fCmdDrawMultiIndexedEXT;
#endif /* defined(VK_EXT_multi_draw) */
#if defined(VK_EXT_private_data)
        VkPtr<PFN_vkCreatePrivateDataSlotEXT>  fCreatePrivateDataSlotEXT;
        VkPtr<PFN_vkDestroyPrivateDataSlotEXT> fDestroyPrivateDataSlotEXT;
        VkPtr<PFN_vkGetPrivateDataEXT>         fGetPrivateDataEXT;
        VkPtr<PFN_vkSetPrivateDataEXT>         fSetPrivateDataEXT;
#endif /* defined(VK_EXT_private_data) */
#if defined(VK_EXT_sample_locations)
        VkPtr<PFN_vkCmdSetSampleLocationsEXT> fCmdSetSampleLocationsEXT;
#endif /* defined(VK_EXT_sample_locations) */
#if defined(VK_EXT_transform_feedback)
        VkPtr<PFN_vkCmdBeginQueryIndexedEXT>            fCmdBeginQueryIndexedEXT;
        VkPtr<PFN_vkCmdBeginTransformFeedbackEXT>       fCmdBeginTransformFeedbackEXT;
        VkPtr<PFN_vkCmdBindTransformFeedbackBuffersEXT> fCmdBindTransformFeedbackBuffersEXT;
        VkPtr<PFN_vkCmdDrawIndirectByteCountEXT>        fCmdDrawIndirectByteCountEXT;
        VkPtr<PFN_vkCmdEndQueryIndexedEXT>              fCmdEndQueryIndexedEXT;
        VkPtr<PFN_vkCmdEndTransformFeedbackEXT>         fCmdEndTransformFeedbackEXT;
#endif /* defined(VK_EXT_transform_feedback) */
#if defined(VK_EXT_validation_cache)
        VkPtr<PFN_vkCreateValidationCacheEXT>  fCreateValidationCacheEXT;
        VkPtr<PFN_vkDestroyValidationCacheEXT> fDestroyValidationCacheEXT;
        VkPtr<PFN_vkGetValidationCacheDataEXT> fGetValidationCacheDataEXT;
        VkPtr<PFN_vkMergeValidationCachesEXT>  fMergeValidationCachesEXT;
#endif /* defined(VK_EXT_validation_cache) */
#if defined(VK_EXT_vertex_input_dynamic_state)
        VkPtr<PFN_vkCmdSetVertexInputEXT> fCmdSetVertexInputEXT;
#endif /* defined(VK_EXT_vertex_input_dynamic_state) */
#if defined(VK_FUCHSIA_external_memory)
        VkPtr<PFN_vkGetMemoryZirconHandleFUCHSIA>           fGetMemoryZirconHandleFUCHSIA;
        VkPtr<PFN_vkGetMemoryZirconHandlePropertiesFUCHSIA> fGetMemoryZirconHandlePropertiesFUCHSIA;
#endif /* defined(VK_FUCHSIA_external_memory) */
#if defined(VK_FUCHSIA_external_semaphore)
        VkPtr<PFN_vkGetSemaphoreZirconHandleFUCHSIA>    fGetSemaphoreZirconHandleFUCHSIA;
        VkPtr<PFN_vkImportSemaphoreZirconHandleFUCHSIA> fImportSemaphoreZirconHandleFUCHSIA;
#endif /* defined(VK_FUCHSIA_external_semaphore) */
#if defined(VK_GOOGLE_display_timing)
        VkPtr<PFN_vkGetPastPresentationTimingGOOGLE> fGetPastPresentationTimingGOOGLE;
        VkPtr<PFN_vkGetRefreshCycleDurationGOOGLE>   fGetRefreshCycleDurationGOOGLE;
#endif /* defined(VK_GOOGLE_display_timing) */
#if defined(VK_HUAWEI_invocation_mask)
        VkPtr<PFN_vkCmdBindInvocationMaskHUAWEI> fCmdBindInvocationMaskHUAWEI;
#endif /* defined(VK_HUAWEI_invocation_mask) */
#if defined(VK_HUAWEI_subpass_shading)
        VkPtr<PFN_vkCmdSubpassShadingHUAWEI>                       fCmdSubpassShadingHUAWEI;
        VkPtr<PFN_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI> fGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI;
#endif /* defined(VK_HUAWEI_subpass_shading) */
#if defined(VK_INTEL_performance_query)
        VkPtr<PFN_vkAcquirePerformanceConfigurationINTEL>  fAcquirePerformanceConfigurationINTEL;
        VkPtr<PFN_vkCmdSetPerformanceMarkerINTEL>          fCmdSetPerformanceMarkerINTEL;
        VkPtr<PFN_vkCmdSetPerformanceOverrideINTEL>        fCmdSetPerformanceOverrideINTEL;
        VkPtr<PFN_vkCmdSetPerformanceStreamMarkerINTEL>    fCmdSetPerformanceStreamMarkerINTEL;
        VkPtr<PFN_vkGetPerformanceParameterINTEL>          fGetPerformanceParameterINTEL;
        VkPtr<PFN_vkInitializePerformanceApiINTEL>         fInitializePerformanceApiINTEL;
        VkPtr<PFN_vkQueueSetPerformanceConfigurationINTEL> fQueueSetPerformanceConfigurationINTEL;
        VkPtr<PFN_vkReleasePerformanceConfigurationINTEL>  fReleasePerformanceConfigurationINTEL;
        VkPtr<PFN_vkUninitializePerformanceApiINTEL>       fUninitializePerformanceApiINTEL;
#endif /* defined(VK_INTEL_performance_query) */
#if defined(VK_KHR_acceleration_structure)
        VkPtr<PFN_vkBuildAccelerationStructuresKHR>              fBuildAccelerationStructuresKHR;
        VkPtr<PFN_vkCmdBuildAccelerationStructuresIndirectKHR>   fCmdBuildAccelerationStructuresIndirectKHR;
        VkPtr<PFN_vkCmdBuildAccelerationStructuresKHR>           fCmdBuildAccelerationStructuresKHR;
        VkPtr<PFN_vkCmdCopyAccelerationStructureKHR>             fCmdCopyAccelerationStructureKHR;
        VkPtr<PFN_vkCmdCopyAccelerationStructureToMemoryKHR>     fCmdCopyAccelerationStructureToMemoryKHR;
        VkPtr<PFN_vkCmdCopyMemoryToAccelerationStructureKHR>     fCmdCopyMemoryToAccelerationStructureKHR;
        VkPtr<PFN_vkCmdWriteAccelerationStructuresPropertiesKHR> fCmdWriteAccelerationStructuresPropertiesKHR;
        VkPtr<PFN_vkCopyAccelerationStructureKHR>                fCopyAccelerationStructureKHR;

        VkPtr<PFN_vkCopyAccelerationStructureToMemoryKHR>         fCopyAccelerationStructureToMemoryKHR;
        VkPtr<PFN_vkCopyMemoryToAccelerationStructureKHR> fCopyMemoryToAccelerationStructureKHR;
        VkPtr<PFN_vkCreateAccelerationStructureKHR>               fCreateAccelerationStructureKHR;
        VkPtr<PFN_vkDestroyAccelerationStructureKHR>              fDestroyAccelerationStructureKHR;

        VkPtr<PFN_vkGetAccelerationStructureBuildSizesKHR>                  fGetAccelerationStructureBuildSizesKHR;
        VkPtr<PFN_vkGetAccelerationStructureDeviceAddressKHR>       fGetAccelerationStructureDeviceAddressKHR;
        VkPtr<PFN_vkGetDeviceAccelerationStructureCompatibilityKHR>         fGetDeviceAccelerationStructureCompatibilityKHR;
        VkPtr<PFN_vkWriteAccelerationStructuresPropertiesKHR>               fWriteAccelerationStructuresPropertiesKHR;

#endif /* defined(VK_KHR_acceleration_structure) */
#if defined(VK_KHR_bind_memory2)
        VkPtr<PFN_vkBindBufferMemory2KHR> fBindBufferMemory2KHR;
        VkPtr<PFN_vkBindImageMemory2KHR>  fBindImageMemory2KHR;
#endif /* defined(VK_KHR_bind_memory2) */
#if defined(VK_KHR_buffer_device_address)
        VkPtr<PFN_vkGetBufferDeviceAddressKHR>              fGetBufferDeviceAddressKHR;
        VkPtr<PFN_vkGetBufferOpaqueCaptureAddressKHR>       fGetBufferOpaqueCaptureAddressKHR;
        VkPtr<PFN_vkGetDeviceMemoryOpaqueCaptureAddressKHR> fGetDeviceMemoryOpaqueCaptureAddressKHR;

#endif /* defined(VK_KHR_buffer_device_address) */
#if defined(VK_KHR_copy_commands2)
        VkPtr<PFN_vkCmdBlitImage2KHR>         fCmdBlitImage2KHR;
        VkPtr<PFN_vkCmdCopyBuffer2KHR>        fCmdCopyBuffer2KHR;
        VkPtr<PFN_vkCmdCopyBufferToImage2KHR> fCmdCopyBufferToImage2KHR;
        VkPtr<PFN_vkCmdCopyImage2KHR>         fCmdCopyImage2KHR;
        VkPtr<PFN_vkCmdCopyImageToBuffer2KHR> fCmdCopyImageToBuffer2KHR;
        VkPtr<PFN_vkCmdResolveImage2KHR>      fCmdResolveImage2KHR;
#endif /* defined(VK_KHR_copy_commands2) */
#if defined(VK_KHR_create_renderpass2)
        VkPtr<PFN_vkCmdBeginRenderPass2KHR> fCmdBeginRenderPass2KHR;
        VkPtr<PFN_vkCmdEndRenderPass2KHR>   fCmdEndRenderPass2KHR;
        VkPtr<PFN_vkCmdNextSubpass2KHR>     fCmdNextSubpass2KHR;
        VkPtr<PFN_vkCreateRenderPass2KHR>   fCreateRenderPass2KHR;
#endif /* defined(VK_KHR_create_renderpass2) */
#if defined(VK_KHR_deferred_host_operations)
        VkPtr<PFN_vkCreateDeferredOperationKHR>            fCreateDeferredOperationKHR;
        VkPtr<PFN_vkDeferredOperationJoinKHR>              fDeferredOperationJoinKHR;
        VkPtr<PFN_vkDestroyDeferredOperationKHR>           fDestroyDeferredOperationKHR;
        VkPtr<PFN_vkGetDeferredOperationMaxConcurrencyKHR> fGetDeferredOperationMaxConcurrencyKHR;
        VkPtr<PFN_vkGetDeferredOperationResultKHR>         fGetDeferredOperationResultKHR;
#endif /* defined(VK_KHR_deferred_host_operations) */
#if defined(VK_KHR_descriptor_update_template)
        VkPtr<PFN_vkCreateDescriptorUpdateTemplateKHR>  fCreateDescriptorUpdateTemplateKHR;
        VkPtr<PFN_vkDestroyDescriptorUpdateTemplateKHR> fDestroyDescriptorUpdateTemplateKHR;
        VkPtr<PFN_vkUpdateDescriptorSetWithTemplateKHR> fUpdateDescriptorSetWithTemplateKHR;
#endif /* defined(VK_KHR_descriptor_update_template) */
#if defined(VK_KHR_device_group)
        VkPtr<PFN_vkCmdDispatchBaseKHR>                  fCmdDispatchBaseKHR;
        VkPtr<PFN_vkCmdSetDeviceMaskKHR>                 fCmdSetDeviceMaskKHR;
        VkPtr<PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR> fGetDeviceGroupPeerMemoryFeaturesKHR;
#endif /* defined(VK_KHR_device_group) */
#if defined(VK_KHR_display_swapchain)
        VkPtr<PFN_vkCreateSharedSwapchainsKHR> fCreateSharedSwapchainsKHR;
#endif /* defined(VK_KHR_display_swapchain) */
#if defined(VK_KHR_draw_indirect_count)
        VkPtr<PFN_vkCmdDrawIndexedIndirectCountKHR> fCmdDrawIndexedIndirectCountKHR;
        VkPtr<PFN_vkCmdDrawIndirectCountKHR>        fCmdDrawIndirectCountKHR;
#endif /* defined(VK_KHR_draw_indirect_count) */
#if defined(VK_KHR_external_fence_fd)
        VkPtr<PFN_vkGetFenceFdKHR>    fGetFenceFdKHR;
        VkPtr<PFN_vkImportFenceFdKHR> fImportFenceFdKHR;
#endif /* defined(VK_KHR_external_fence_fd) */
#if defined(VK_KHR_external_fence_win32)
        VkPtr<PFN_vkGetFenceWin32HandleKHR>    fGetFenceWin32HandleKHR;
        VkPtr<PFN_vkImportFenceWin32HandleKHR> fImportFenceWin32HandleKHR;
#endif /* defined(VK_KHR_external_fence_win32) */
#if defined(VK_KHR_external_memory_fd)
        VkPtr<PFN_vkGetMemoryFdKHR>           fGetMemoryFdKHR;
        VkPtr<PFN_vkGetMemoryFdPropertiesKHR> fGetMemoryFdPropertiesKHR;
#endif /* defined(VK_KHR_external_memory_fd) */
#if defined(VK_KHR_external_memory_win32)
        VkPtr<PFN_vkGetMemoryWin32HandleKHR>           fGetMemoryWin32HandleKHR;
        VkPtr<PFN_vkGetMemoryWin32HandlePropertiesKHR> fGetMemoryWin32HandlePropertiesKHR;
#endif /* defined(VK_KHR_external_memory_win32) */
#if defined(VK_KHR_external_semaphore_fd)
        VkPtr<PFN_vkGetSemaphoreFdKHR>    fGetSemaphoreFdKHR;
        VkPtr<PFN_vkImportSemaphoreFdKHR> fImportSemaphoreFdKHR;
#endif /* defined(VK_KHR_external_semaphore_fd) */
#if defined(VK_KHR_external_semaphore_win32)
        VkPtr<PFN_vkGetSemaphoreWin32HandleKHR>    fGetSemaphoreWin32HandleKHR;
        VkPtr<PFN_vkImportSemaphoreWin32HandleKHR> fImportSemaphoreWin32HandleKHR;
#endif /* defined(VK_KHR_external_semaphore_win32) */
#if defined(VK_KHR_fragment_shading_rate)
        VkPtr<PFN_vkCmdSetFragmentShadingRateKHR> fCmdSetFragmentShadingRateKHR;
#endif /* defined(VK_KHR_fragment_shading_rate) */
#if defined(VK_KHR_get_memory_requirements2)
        VkPtr<PFN_vkGetBufferMemoryRequirements2KHR>      fGetBufferMemoryRequirements2KHR;
        VkPtr<PFN_vkGetImageMemoryRequirements2KHR>       fGetImageMemoryRequirements2KHR;
        VkPtr<PFN_vkGetImageSparseMemoryRequirements2KHR> fGetImageSparseMemoryRequirements2KHR;
#endif /* defined(VK_KHR_get_memory_requirements2) */
#if defined(VK_KHR_maintenance1)
        VkPtr<PFN_vkTrimCommandPoolKHR> fTrimCommandPoolKHR;
#endif /* defined(VK_KHR_maintenance1) */
#if defined(VK_KHR_maintenance3)
        VkPtr<PFN_vkGetDescriptorSetLayoutSupportKHR> fGetDescriptorSetLayoutSupportKHR;
#endif /* defined(VK_KHR_maintenance3) */
#if defined(VK_KHR_performance_query)
        VkPtr<PFN_vkAcquireProfilingLockKHR> fAcquireProfilingLockKHR;
        VkPtr<PFN_vkReleaseProfilingLockKHR> fReleaseProfilingLockKHR;
#endif /* defined(VK_KHR_performance_query) */
#if defined(VK_KHR_pipeline_executable_properties)
        VkPtr<PFN_vkGetPipelineExecutableInternalRepresentationsKHR> fGetPipelineExecutableInternalRepresentationsKHR;
        VkPtr<PFN_vkGetPipelineExecutablePropertiesKHR>              fGetPipelineExecutablePropertiesKHR;
        VkPtr<PFN_vkGetPipelineExecutableStatisticsKHR>              fGetPipelineExecutableStatisticsKHR;
#endif /* defined(VK_KHR_pipeline_executable_properties) */
#if defined(VK_KHR_present_wait)
        VkPtr<PFN_vkWaitForPresentKHR> fWaitForPresentKHR;
#endif /* defined(VK_KHR_present_wait) */
#if defined(VK_KHR_push_descriptor)
        VkPtr<PFN_vkCmdPushDescriptorSetKHR> fCmdPushDescriptorSetKHR;
#endif /* defined(VK_KHR_push_descriptor) */
#if defined(VK_KHR_ray_tracing_pipeline)
        VkPtr<PFN_vkCmdSetRayTracingPipelineStackSizeKHR> fCmdSetRayTracingPipelineStackSizeKHR;
        VkPtr<PFN_vkCmdTraceRaysIndirectKHR>              fCmdTraceRaysIndirectKHR;
        VkPtr<PFN_vkCmdTraceRaysKHR>                      fCmdTraceRaysKHR;
        VkPtr<PFN_vkCreateRayTracingPipelinesKHR>         fCreateRayTracingPipelinesKHR;

        VkPtr<PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR> fGetRayTracingCaptureReplayShaderGroupHandlesKHR;
        VkPtr<PFN_vkGetRayTracingShaderGroupHandlesKHR>              fGetRayTracingShaderGroupHandlesKHR;
        VkPtr<PFN_vkGetRayTracingShaderGroupStackSizeKHR>            fGetRayTracingShaderGroupStackSizeKHR;
#endif /* defined(VK_KHR_ray_tracing_pipeline) */ 
#if defined(VK_KHR_sampler_ycbcr_conversion)
        VkPtr<PFN_vkCreateSamplerYcbcrConversionKHR>  fCreateSamplerYcbcrConversionKHR;
        VkPtr<PFN_vkDestroySamplerYcbcrConversionKHR> fDestroySamplerYcbcrConversionKHR;
#endif /* defined(VK_KHR_sampler_ycbcr_conversion) */
#if defined(VK_KHR_shared_presentable_image)
        VkPtr<PFN_vkGetSwapchainStatusKHR> fGetSwapchainStatusKHR;
#endif /* defined(VK_KHR_shared_presentable_image) */
#if defined(VK_KHR_swapchain)
        VkPtr<PFN_vkAcquireNextImageKHR>   fAcquireNextImageKHR;
        VkPtr<PFN_vkCreateSwapchainKHR>    fCreateSwapchainKHR;
        VkPtr<PFN_vkDestroySwapchainKHR>   fDestroySwapchainKHR;
        VkPtr<PFN_vkGetSwapchainImagesKHR> fGetSwapchainImagesKHR;
        VkPtr<PFN_vkQueuePresentKHR>       fQueuePresentKHR;
#endif /* defined(VK_KHR_swapchain) */
#if defined(VK_KHR_synchronization2)
        VkPtr<PFN_vkCmdPipelineBarrier2KHR> fCmdPipelineBarrier2KHR;
        VkPtr<PFN_vkCmdResetEvent2KHR>      fCmdResetEvent2KHR;
        VkPtr<PFN_vkCmdSetEvent2KHR>        fCmdSetEvent2KHR;
        VkPtr<PFN_vkCmdWaitEvents2KHR>      fCmdWaitEvents2KHR;
        VkPtr<PFN_vkCmdWriteTimestamp2KHR>  fCmdWriteTimestamp2KHR;
        VkPtr<PFN_vkQueueSubmit2KHR>        fQueueSubmit2KHR;
#endif /* defined(VK_KHR_synchronization2) */
#if defined(VK_KHR_synchronization2) && defined(VK_AMD_buffer_marker)
        VkPtr<PFN_vkCmdWriteBufferMarker2AMD> fCmdWriteBufferMarker2AMD;
#endif /* defined(VK_KHR_synchronization2) && defined(VK_AMD_buffer_marker) */
#if defined(VK_KHR_synchronization2) && defined(VK_NV_device_diagnostic_checkpoints)
        VkPtr<PFN_vkGetQueueCheckpointData2NV> fGetQueueCheckpointData2NV;
#endif /* defined(VK_KHR_synchronization2) && defined(VK_NV_device_diagnostic_checkpoints) */
#if defined(VK_KHR_timeline_semaphore)
        VkPtr<PFN_vkGetSemaphoreCounterValueKHR> fGetSemaphoreCounterValueKHR;
        VkPtr<PFN_vkSignalSemaphoreKHR>          fSignalSemaphoreKHR;
        VkPtr<PFN_vkWaitSemaphoresKHR>           fWaitSemaphoresKHR;
#endif /* defined(VK_KHR_timeline_semaphore) */
#if defined(VK_KHR_video_decode_queue) 
        VkPtr<PFN_vkCmdDecodeVideoKHR> fCmdDecodeVideoKHR;
#endif /* defined(VK_KHR_video_decode_queue) */
#if defined(VK_KHR_video_encode_queue)
        VkPtr<PFN_vkCmdEncodeVideoKHR> fCmdEncodeVideoKHR;
#endif /* defined(VK_KHR_video_encode_queue) */
#if defined(VK_KHR_video_queue)
        VkPtr<PFN_vkBindVideoSessionMemoryKHR> fBindVideoSessionMemoryKHR;
        VkPtr<PFN_vkCmdBeginVideoCodingKHR>    fCmdBeginVideoCodingKHR;
        VkPtr<PFN_vkCmdControlVideoCodingKHR>  fCmdControlVideoCodingKHR;
        VkPtr<PFN_vkCmdEndVideoCodingKHR>      fCmdEndVideoCodingKHR;
        VkPtr<PFN_vkCreateVideoSessionKHR>            fCreateVideoSessionKHR;
        VkPtr<PFN_vkCreateVideoSessionParametersKHR>  fCreateVideoSessionParametersKHR;
        VkPtr<PFN_vkDestroyVideoSessionKHR>           fDestroyVideoSessionKHR;
        VkPtr<PFN_vkDestroyVideoSessionParametersKHR> fDestroyVideoSessionParametersKHR;
        VkPtr<PFN_vkGetVideoSessionMemoryRequirementsKHR> fGetVideoSessionMemoryRequirementsKHR;
        VkPtr<PFN_vkUpdateVideoSessionParametersKHR>      fUpdateVideoSessionParametersKHR;

#endif /* defined(VK_KHR_video_queue) */
#if defined(VK_NVX_binary_import)
        VkPtr<PFN_vkCmdCuLaunchKernelNVX> fCmdCuLaunchKernelNVX;
        VkPtr<PFN_vkCreateCuFunctionNVX>  fCreateCuFunctionNVX;
        VkPtr<PFN_vkCreateCuModuleNVX>    fCreateCuModuleNVX;
        VkPtr<PFN_vkDestroyCuFunctionNVX> fDestroyCuFunctionNVX;
        VkPtr<PFN_vkDestroyCuModuleNVX>   fDestroyCuModuleNVX;
#endif /* defined(VK_NVX_binary_import) */
#if defined(VK_NVX_image_view_handle)
        VkPtr<PFN_vkGetImageViewAddressNVX> fGetImageViewAddressNVX;
        VkPtr<PFN_vkGetImageViewHandleNVX>  fGetImageViewHandleNVX;
#endif /* defined(VK_NVX_image_view_handle) */
#if defined(VK_NV_clip_space_w_scaling)
        VkPtr<PFN_vkCmdSetViewportWScalingNV> fCmdSetViewportWScalingNV;
#endif /* defined(VK_NV_clip_space_w_scaling) */
#if defined(VK_NV_device_diagnostic_checkpoints)
        VkPtr<PFN_vkCmdSetCheckpointNV>       fCmdSetCheckpointNV;
        VkPtr<PFN_vkGetQueueCheckpointDataNV> fGetQueueCheckpointDataNV;
#endif /* defined(VK_NV_device_diagnostic_checkpoints) */
#if defined(VK_NV_device_generated_commands)
        VkPtr<PFN_vkCmdBindPipelineShaderGroupNV>             fCmdBindPipelineShaderGroupNV;
        VkPtr<PFN_vkCmdExecuteGeneratedCommandsNV>            fCmdExecuteGeneratedCommandsNV;
        VkPtr<PFN_vkCmdPreprocessGeneratedCommandsNV>         fCmdPreprocessGeneratedCommandsNV;
        VkPtr<PFN_vkCreateIndirectCommandsLayoutNV>           fCreateIndirectCommandsLayoutNV;
        VkPtr<PFN_vkDestroyIndirectCommandsLayoutNV>          fDestroyIndirectCommandsLayoutNV;
        VkPtr<PFN_vkGetGeneratedCommandsMemoryRequirementsNV> fGetGeneratedCommandsMemoryRequirementsNV;
#endif /* defined(VK_NV_device_generated_commands) */
#if defined(VK_NV_external_memory_rdma)
        VkPtr<PFN_vkGetMemoryRemoteAddressNV> fGetMemoryRemoteAddressNV;
#endif /* defined(VK_NV_external_memory_rdma) */
#if defined(VK_NV_external_memory_win32)
        VkPtr<PFN_vkGetMemoryWin32HandleNV> fGetMemoryWin32HandleNV;
#endif /* defined(VK_NV_external_memory_win32) */
#if defined(VK_NV_fragment_shading_rate_enums)
        VkPtr<PFN_vkCmdSetFragmentShadingRateEnumNV> fCmdSetFragmentShadingRateEnumNV;
#endif /* defined(VK_NV_fragment_shading_rate_enums) */
#if defined(VK_NV_mesh_shader)
        VkPtr<PFN_vkCmdDrawMeshTasksIndirectCountNV> fCmdDrawMeshTasksIndirectCountNV;
        VkPtr<PFN_vkCmdDrawMeshTasksIndirectNV>      fCmdDrawMeshTasksIndirectNV;
        VkPtr<PFN_vkCmdDrawMeshTasksNV>              fCmdDrawMeshTasksNV;
#endif /* defined(VK_NV_mesh_shader) */
#if defined(VK_NV_ray_tracing)
        VkPtr<PFN_vkBindAccelerationStructureMemoryNV> fBindAccelerationStructureMemoryNV;
        VkPtr<PFN_vkCmdBuildAccelerationStructureNV>   fCmdBuildAccelerationStructureNV;
        VkPtr<PFN_vkCmdCopyAccelerationStructureNV>    fCmdCopyAccelerationStructureNV;
        VkPtr<PFN_vkCmdTraceRaysNV>                    fCmdTraceRaysNV;

        VkPtr<PFN_vkCmdWriteAccelerationStructuresPropertiesNV> fCmdWriteAccelerationStructuresPropertiesNV;
        VkPtr<PFN_vkCompileDeferredNV>                          fCompileDeferredNV;
        VkPtr<PFN_vkCreateAccelerationStructureNV>              fCreateAccelerationStructureNV;
        VkPtr<PFN_vkCreateRayTracingPipelinesNV>                fCreateRayTracingPipelinesNV;

        VkPtr<PFN_vkDestroyAccelerationStructureNV>               fDestroyAccelerationStructureNV;
        VkPtr<PFN_vkGetAccelerationStructureHandleNV>             fGetAccelerationStructureHandleNV;
        VkPtr<PFN_vkGetAccelerationStructureMemoryRequirementsNV> fGetAccelerationStructureMemoryRequirementsNV;
        VkPtr<PFN_vkGetRayTracingShaderGroupHandlesNV>            fGetRayTracingShaderGroupHandlesNV;
#endif /* defined(VK_NV_ray_tracing) */
#if defined(VK_NV_scissor_exclusive)
        VkPtr<PFN_vkCmdSetExclusiveScissorNV> fCmdSetExclusiveScissorNV;
#endif /* defined(VK_NV_scissor_exclusive) */
#if defined(VK_NV_shading_rate_image)
        VkPtr<PFN_vkCmdBindShadingRateImageNV>          fCmdBindShadingRateImageNV;
        VkPtr<PFN_vkCmdSetCoarseSampleOrderNV>          fCmdSetCoarseSampleOrderNV;
        VkPtr<PFN_vkCmdSetViewportShadingRatePaletteNV> fCmdSetViewportShadingRatePaletteNV;
#endif /* defined(VK_NV_shading_rate_image) */
#if (defined(VK_EXT_full_screen_exclusive) && defined(VK_KHR_device_group)) || (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1))
        VkPtr<PFN_vkGetDeviceGroupSurfacePresentModes2EXT> fGetDeviceGroupSurfacePresentModes2EXT;
#endif /* (defined(VK_EXT_full_screen_exclusive) && defined(VK_KHR_device_group)) || (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_push_descriptor) && defined(VK_KHR_descriptor_update_template))
        VkPtr<PFN_vkCmdPushDescriptorSetWithTemplateKHR> fCmdPushDescriptorSetWithTemplateKHR;
#endif /* (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_push_descriptor) && defined(VK_KHR_descriptor_update_template)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
        VkPtr<PFN_vkGetDeviceGroupPresentCapabilitiesKHR> fGetDeviceGroupPresentCapabilitiesKHR;
        VkPtr<PFN_vkGetDeviceGroupSurfacePresentModesKHR> fGetDeviceGroupSurfacePresentModesKHR;
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
        VkPtr<PFN_vkAcquireNextImage2KHR> fAcquireNextImage2KHR;
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
    } fFunctions;
};

/*
This variable will be global and all diligent calls must go through it
*/ 
    VulkanInterface diligent_vk_interface;

}

#endif EngineVkUtils_DEFINED