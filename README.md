# VulkanLab

VulkanLab is the disposable learning project. Use it to prove one small idea at a time before deciding whether that idea belongs in Reimagine.

The starter milestone is deliberately small: it creates a native Win32 window, links the Vulkan loader, queries the loader's supported API version, and displays that version in the window title. It does not create a Vulkan instance or renderer yet.

## Build

From this directory:

```powershell
cmake --preset vs2022-x64
cmake --build --preset debug
```

Run:

```powershell
.\build\vs2022-x64\Debug\VulkanLab.exe
```

## Suggested first experiments

Do these one at a time and keep the program buildable:

1. Print or display the available instance layers.
2. Create and destroy a `VkInstance`.
3. Enable the validation layer in Debug builds.
4. Create a Win32 Vulkan surface.
5. Choose a physical device and queue family.
6. Create the logical device and queues.
7. Add the swapchain only after the earlier lifetime rules make sense.

VulkanLab and Reimagine are separate projects. Do not add one with `add_subdirectory`, and do not copy every experiment into Reimagine automatically.

