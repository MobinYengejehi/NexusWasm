# GPU Service

The planned GPU subsystem is a portable render/compute abstraction with replaceable providers.

## Goals

- portable shader/pipeline model
- portable resource binding model
- command batching
- compute + graphics queues
- multi-GPU support
- opaque typed resource handles
- resource quotas
- external resource import/export
- engine-owned GPU providers

## Architecture

```text
Guest
  ↓
Nexus GPU API
  ↓
GpuService
  ↓
IGpuProvider
  ├── Direct3D backend
  ├── Vulkan backend
  ├── OpenGL backend
  ├── Metal backend
  └── Host-engine backend
```

## Conceptual pipeline API

```cpp
// Conceptual pre-1.0 API.
auto vertexShader = device.CreateShader(vertexDesc);
auto fragmentShader = device.CreateShader(fragmentDesc);

auto pipeline = device.CreateGraphicsPipeline({
    .vertexShader = vertexShader,
    .fragmentShader = fragmentShader,
    // portable pipeline state
});
```

The guest should not receive a native `VkPipeline`, `GLuint`, or Direct3D pipeline pointer.

## Portable resource bindings

Instead of exposing backend-specific registers directly, NexusWasm should define its own logical binding model.

Conceptually:

```cpp
cmd.BindTexture(/*group=*/0, /*binding=*/0, texture);
cmd.BindUniformBuffer(0, 1, uniforms);
```

The provider maps that model to the native backend.

## Command recording

```cpp
// Conceptual pre-1.0 API.
auto cmd = device.CreateCommandList();

cmd.BindPipeline(pipeline);
cmd.BindVertexBuffer(vertices);
cmd.BindIndexBuffer(indices);
cmd.DrawIndexed(indexCount);

queue.Submit(cmd);
```

Individual recorded commands should not require a guest-to-host transition each time.

## Multi-GPU

NexusWasm should allow a host to expose multiple logical adapters.

Example:

```text
GPU A → compute
GPU B → rendering
```

Cross-adapter sharing is capability- and hardware-dependent. Fallback paths may require staging/copying.

## Existing host graphics device

One of the most important design goals is allowing a host to provide its existing graphics device/context as the Nexus GPU backend.

```text
Game / Engine Renderer
      ↓
Existing GPU Device
      ↓
EngineGpuProvider
      ↓
Nexus GPU Service
      ↓
Wasm guest
```

This makes host resources and Nexus-created resources part of the same graphics ecosystem where the underlying API permits it.

## Engine integration example

A future host-specific extension might expose an engine material while the actual texture/pipeline resource remains a Nexus GPU resource.

```cpp
// Host-specific conceptual API, not Nexus Core.
auto object = engine::graphics::GetObject(id);
auto material = object.GetMaterial(0);

material.SetTexture("diffuse", nexusTexture);
material.OverrideFragmentShader(nexusShader);
```

## RmlUi as an integration target

RmlUi can be integrated by implementing its renderer backend on top of Nexus GPU commands.

```text
RmlUi
  ↓
NexusRmlUiRenderer
  ↓
Nexus GPU API
  ↓
Host GPU Provider
  ↓
D3D / Vulkan / OpenGL / Metal / Engine Renderer
```

This allows the same RmlUi backend to work across different host graphics backends.

## Offscreen UI

A UI can render to a Nexus texture and then be attached to an engine object.

```text
RmlUi
 ↓
Offscreen Nexus texture
 ↓
Engine material
 ↓
In-world monitor / phone / dashboard
```

Where the host provider can share the same GPU resource, this can avoid a GPU→CPU→GPU round trip.

## Shader representation

The exact canonical shader source/IR strategy is intentionally not frozen yet.

Before implementation the GPU phase must decide:

1. supported shader source/IR formats
2. cross-backend translation strategy
3. reflection/binding metadata
4. offline vs runtime compilation
5. cache identity and backend compatibility
