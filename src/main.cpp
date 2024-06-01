#include <GLFW/glfw3.h>
#include <glm/mat2x2.hpp>
#include <webgpu/webgpu_cpp.h>
#include <iostream>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

wgpu::Instance instance;
wgpu::Device device;

void GetDevice(void (*callback)(wgpu::Device))
{
    instance.RequestAdapter(
        nullptr,
        // TODO(https://bugs.chromium.org/p/dawn/issues/detail?id=1892): Use
        // wgpu::RequestAdapterStatus, wgpu::Adapter, and wgpu::Device.
        [](WGPURequestAdapterStatus status, WGPUAdapter cAdapter,
           const char *message, void *userdata)
        {
            if (status != WGPURequestAdapterStatus_Success)
            {
                exit(0);
            }
            wgpu::Adapter adapter = wgpu::Adapter::Acquire(cAdapter);
            adapter.RequestDevice(
                nullptr,
                [](WGPURequestDeviceStatus status, WGPUDevice cDevice,
                   const char *message, void *userdata)
                {
                    wgpu::Device device = wgpu::Device::Acquire(cDevice);
                    device.SetUncapturedErrorCallback(
                        [](WGPUErrorType type, const char *message, void *userdata)
                        {
                            std::cout << "Error: " << type << " - message: " << message;
                        },
                        nullptr);
                    reinterpret_cast<void (*)(wgpu::Device)>(userdata)(device);
                },
                userdata);
        },
        reinterpret_cast<void *>(callback));
}

const char shaderCode[] = R"(
@group(0) @binding(0)
var<storage, read_write> output: array<i32>;

@compute @workgroup_size(32)
fn main(
  @builtin(global_invocation_id)
  global_id : vec3u,

  @builtin(local_invocation_id)
  local_id : vec3u,
) {
  // Avoid accessing the buffer out of bounds
  if (global_id.x >= 250) {
    return;
  }

  output[global_id.x] = output[global_id.x] + 1;

}
)";

wgpu::Buffer createBuffer(wgpu::BufferUsage usage)
{
    wgpu::BufferDescriptor descriptor{
        .usage = usage, .size = 1000};

    return device.CreateBuffer(&descriptor);
}

wgpu::BindGroupLayout createBindGroupLayout()
{
    wgpu::BindGroupLayoutDescriptor bglDesc{
        .entryCount = 1,
        .entries = new wgpu::BindGroupLayoutEntry[1]{
            {
                .binding = 0,
                .visibility = wgpu::ShaderStage::Compute,
                .buffer = {.type = wgpu::BufferBindingType::Storage},
            },
        },
    };
    return device.CreateBindGroupLayout(&bglDesc);
}

wgpu::Buffer output;
wgpu::Buffer stagingBuffer;
wgpu::BindGroupLayout bindGroupLayout;
wgpu::BindGroup bindGroup;

wgpu::BindGroup createBindGroup(wgpu::Buffer buffer)
{
    wgpu::BindGroupEntry bgEntry{
        .binding = 0,
        .buffer = buffer,
        .offset = 0,
        .size = 1000,
    };
    wgpu::BindGroupDescriptor bgDesc{
        .layout = bindGroupLayout,
        .entryCount = 1,
        .entries = &bgEntry,
    };
    return device.CreateBindGroup(&bgDesc);
}

wgpu::ComputePipeline createComputePipeline()
{
    wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
    wgslDesc.code = shaderCode;

    wgpu::ShaderModuleDescriptor shaderModuleDescriptor{
        .nextInChain = &wgslDesc};
    wgpu::ShaderModule shaderModule =
        device.CreateShaderModule(&shaderModuleDescriptor);

    wgpu::PipelineLayoutDescriptor plDesc{
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &bindGroupLayout,
    };

    wgpu::ComputePipelineDescriptor cpDesc{
        .layout = device.CreatePipelineLayout(&plDesc),
        .compute = {
            .module = shaderModule,
            .entryPoint = "main",
        },
    };
    return device.CreateComputePipeline(&cpDesc);
}

void runComputePass(wgpu::Buffer output, wgpu::BindGroup bindGroup)
{
    wgpu::ComputePipeline cp = createComputePipeline();
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
    pass.SetPipeline(cp);
    pass.SetBindGroup(0, bindGroup);
    pass.DispatchWorkgroups(16);
    pass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);
}

void copyBufferToStagingBuffer(wgpu::Buffer src, wgpu::Buffer dst)
{
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyBufferToBuffer(src, 0, dst, 0, 1000);
    wgpu::CommandBuffer commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);
}
struct Context
{
    wgpu::Buffer &buffer;
};

void print_stat()
{
    uint32_t status = 0;

    switch(stagingBuffer.GetMapState()) {
        case wgpu::BufferMapState::Unmapped:
            status = 1;
            break;
        case wgpu::BufferMapState::Pending:
            status = 2;
            break;
        case wgpu::BufferMapState::Mapped:
            status = 3;
            break;
    }

    std::cout << status << std::endl;
}

bool done = false;
std::vector<uint32_t> numbers(250);

void readStagingBuffer() {
    uint32_t *data = (uint32_t *)stagingBuffer.GetConstMappedRange(0, 1000);
    for (int i = 0; i < 250; i++)
    {
        std::cout << numbers[i] << "-" << data[i] << "\n";
    }
    std::cout << std::endl;
    stagingBuffer.Unmap();

    done = true;
}

auto onBuffer2Mapped = [](WGPUBufferMapAsyncStatus status, void *pUserData)
{
    wgpu::Buffer *buffer = reinterpret_cast<wgpu::Buffer *>(pUserData);
    std::cout << "Buffer 2 mapped with status " << status << std::endl;
    if (status != WGPUBufferMapAsyncStatus_Success)
        return;

    std::cout << "some serious shit goin on here" << std::endl;

    std::cout << "Context status is " << (uint32_t)buffer->GetMapState() << std::endl;
    print_stat();

    // Get a pointer to wherever the driver mapped the GPU memory to the RAM
    readStagingBuffer();
};

auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void * userdata) {
    std::cout << "[QUEUE] Work done with status: " << status << std::endl;
};

#include <complex>

void test() {
    using mat = glm::mat<2, 2, std::complex<int>>;

    mat test_mat = {
        {0, 0}, {0, 0}, 
        {1, 0}, {1, 0}
    };

    //print the matrix
    for(int i = 0; i < 2; i++) {
        for(int j = 0; j < 2; j++) {
            std::cout << test_mat[i][j] << " ";
        }
        std::cout << std::endl;
    }

    mat b = {
        {0, 0}, {-1, 0},
        {1, 0}, {0, 0}
    };

    mat c = test_mat * b;

    for(int i = 0; i < 2; i++) {
        for(int j = 0; j < 2; j++) {
            std::cout << c[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

                  

int main()
{
    instance = wgpu::CreateInstance();
    for (uint32_t i = 0; i < 250; ++i) numbers[i] = i;
    // i need to create an abstraction layer so bad
    GetDevice([](wgpu::Device dev)
              {
                device = dev;

                dev.GetQueue().OnSubmittedWorkDone(onQueueWorkDone, nullptr);

                  output = createBuffer(wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst);
                  stagingBuffer = createBuffer(wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst);

                  

                  device.GetQueue().WriteBuffer(output, 0, numbers.data(), numbers.size() * sizeof(uint32_t));

                  wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
                  encoder.CopyBufferToBuffer(output, 0, stagingBuffer, 0, 1000);

                  wgpu::CommandBuffer commands = encoder.Finish();
                  device.GetQueue().Submit(1, &commands);

                  bindGroupLayout = createBindGroupLayout();
                  bindGroup = createBindGroup(output);
                  std::cout << "Buffer created" << std::endl;
                  runComputePass(output, bindGroup);
                  copyBufferToStagingBuffer(output, stagingBuffer);

                print_stat();

                stagingBuffer.MapAsync(
                      wgpu::MapMode::Read, 0, 1000, onBuffer2Mapped, (void *)&stagingBuffer);
                });

        // while(!done) {
        //     emscripten_request_animation_frame(nullptr, nullptr);
        // }

    test();


    return 0;   

    
}
