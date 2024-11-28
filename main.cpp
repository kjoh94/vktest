#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <cassert>
#include <cstring>
#include <stdio.h>

const uint32_t N = 16;       // 데이터 크기
//const uint32_t LOCAL_SIZE = 256; // 워크그룹 크기

// 푸시 상수 구조체
struct PushConstants {
    VkDeviceAddress src;
    VkDeviceAddress dst;
};

// 파일에서 SPIR-V 바이너리를 읽는 함수
std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    assert(file.is_open());
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}


void checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    //VkLayerProperties availableLayers[layerCount];
    VkLayerProperties availableLayers[128];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

    int layerFound = 0;
    for (uint32_t i = 0; i < layerCount; i++) {
        if (strcmp(validationLayerName, availableLayers[i].layerName) == 0) {
            layerFound = 1;
            break;
        }
    }

    if (layerFound) {
        printf("Validation layer supported: %s\n", validationLayerName);
    } else {
        printf("Validation layer not found: %s\n", validationLayerName);
    }

    printf("Available Vulkan layers:\n");
    for (uint32_t i = 0; i < layerCount; i++) {
        printf("  %s: %s\n", availableLayers[i].layerName, availableLayers[i].description);
    }
}


int main() {
    checkValidationLayerSupport();
    // Vulkan 인스턴스 생성
    VkInstance instance;

    // 애플리케이션 정보 설정
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;


    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

#if 1
    // 검증 레이어 활성화 (선택 사항)
    const char* validationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    // 활성화할 인스턴스 확장 (예: 디버그 메시지를 위한 확장)
    std::vector<const char*> instanceExtensions;

    // 디버그 유틸 확장을 활성화하여 디버그 메시지 콜백을 설정할 수 있습니다.
    instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    instanceCreateInfo.enabledLayerCount = sizeof(validationLayers) / sizeof(validationLayers[0]);
    instanceCreateInfo.ppEnabledLayerNames = validationLayers;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
#endif

    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
    assert(result == VK_SUCCESS);


    // 물리 디바이스 선택
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    assert(physicalDeviceCount > 0);
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
    VkPhysicalDevice physicalDevice = physicalDevices[0];

    // 디바이스 큐 생성
    uint32_t queueFamilyIndex = 0; // 실제로는 적절한 큐 패밀리를 선택해야 함
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    // 디바이스 확장 활성화
    const char* deviceExtensions[] = {
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
    };

    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
    bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &bufferDeviceAddressFeatures;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    VkDevice device;
    result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
    assert(result == VK_SUCCESS);

    // 큐 가져오기
    VkQueue queue;
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

    // 버퍼 생성
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = sizeof(float) * N;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer srcBuffer, dstBuffer;
    vkCreateBuffer(device, &bufferCreateInfo, nullptr, &srcBuffer);
    vkCreateBuffer(device, &bufferCreateInfo, nullptr, &dstBuffer);

    // 메모리 요구사항 확인
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, srcBuffer, &memRequirements);

    // 메모리 타입 선택
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    uint32_t memoryTypeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
            memoryTypeIndex = i;
            break;
        }
    }
    assert(memoryTypeIndex != UINT32_MAX);

    // 메모리 할당 플래그 설정
    VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
    memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
    memoryAllocateInfo.allocationSize = memRequirements.size * 2;
    memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

    VkDeviceMemory memory;
    vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &memory);

    // 메모리 바인딩
    vkBindBufferMemory(device, srcBuffer, memory, 0);
    vkBindBufferMemory(device, dstBuffer, memory, memRequirements.size);

    // 메모리 매핑 및 초기화
    void* data;
    vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &data);
    float* srcData = reinterpret_cast<float*>(data);
    for (uint32_t i = 0; i < N; ++i) {
        srcData[i] = static_cast<float>(i);
    }
    float* dstData = srcData + N;
    std::memset(dstData, 0, sizeof(float) * N);
    vkUnmapMemory(device, memory);

    // 버퍼 디바이스 주소 가져오기
    VkBufferDeviceAddressInfo bufferDeviceAddressInfo{};
    bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;

    bufferDeviceAddressInfo.buffer = srcBuffer;
    VkDeviceAddress srcAddress = vkGetBufferDeviceAddress(device, &bufferDeviceAddressInfo);

    bufferDeviceAddressInfo.buffer = dstBuffer;
    VkDeviceAddress dstAddress = vkGetBufferDeviceAddress(device, &bufferDeviceAddressInfo);

    // 셰이더 모듈 생성
    std::vector<char> shaderCode = readFile("shader.spv");
    VkShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = shaderCode.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    VkShaderModule shaderModule;
    vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);

    // 파이프라인 레이아웃 생성
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

    VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);

    // 컴퓨트 파이프라인 생성
    VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageCreateInfo.module = shaderModule;
    shaderStageCreateInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stage = shaderStageCreateInfo;
    pipelineCreateInfo.layout = pipelineLayout;

    VkPipeline computePipeline;
    vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &computePipeline);

    // 커맨드 풀 및 커맨드 버퍼 생성
    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
    VkCommandPool commandPool;
    vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);

    // 커맨드 버퍼 레코딩
    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

    // 푸시 상수 설정
    PushConstants pushConstants{};
    pushConstants.src = srcAddress;
    pushConstants.dst = dstAddress;
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &pushConstants);

    // 파이프라인 바인딩
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

    // 디스패치 호출
    vkCmdDispatch(commandBuffer, N, 1, 1);

    vkEndCommandBuffer(commandBuffer);

    // 커맨드 버퍼 제출
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    // 결과 읽기
    vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &data);
    dstData = reinterpret_cast<float*>(data) + N;
    for (uint32_t i = 0; i < N; ++i) {
        std::cout << dstData[i] << std::endl;
    }
    vkUnmapMemory(device, memory);

    // 리소스 해제
    vkDestroyPipeline(device, computePipeline, nullptr);
    vkDestroyShaderModule(device, shaderModule, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyBuffer(device, srcBuffer, nullptr);
    vkDestroyBuffer(device, dstBuffer, nullptr);
    vkFreeMemory(device, memory, nullptr);
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);

    return 0;
}
