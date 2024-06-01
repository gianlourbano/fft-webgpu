#include <webgpu/webgpu_cpp.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

using namespace wgpu;

// finally my beautiful abstraction layer
// #NOTTODO: create another layer
class Application
{
public:
    Application();
    ~Application();
    void run();
    void init();

private:
    Instance instance;
    Device device;

    Buffer inputBuffer;
    Buffer outputBuffer;
    Buffer stagingBuffer;

    BindGroup bindGroup;
    ComputePipeline pipeline;
    Queue queue;
    CommandEncoder encoder;
};