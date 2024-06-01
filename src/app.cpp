#include "app.hpp"

using namespace wgpu;

Application::Application()
{
    init();
}

Application::~Application()
{
}

void Application::init()
{
    instance = CreateInstance();
    instance.RequestAdapter(
        nullptr,
        [](RequestAdapterStatus status, Adapter adapter, void *userdata)
        {
            if (status != RequestAdapterStatus::Success)
            {
                exit(0);
            }
            adapter.RequestDevice(
                nullptr,
                [](RequestDeviceStatus status, Device device, void *userdata)
                {
                    if (status != RequestDeviceStatus::Success)
                    {
                        exit(0);
                    }
                    reinterpret_cast<Application *>(userdata)->device = device;
                },
                userdata);
        },
        this);
}