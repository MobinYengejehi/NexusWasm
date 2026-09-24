# NexusWasm

> A portable, embeddable WebAssembly host runtime for C++ powered by Wasmtime.

NexusWasm is being built as a **general-purpose WebAssembly host runtime**, not as a thin wrapper around Wasmtime and not as a runtime tied to any specific game, engine, or application.

The goal is to provide a reusable runtime layer that combines:

- Wasmtime-based WebAssembly execution
- a structured Runtime / Realm / Program / ExecutionDomain model
- module graphs and direct WebAssembly linking
- native Wasmtime asynchronous execution
- standard C/C++ + WASI guest support
- a strongly typed Service / Capability / Policy / Provider architecture
- replaceable host-service implementations
- sandboxed filesystem and networking
- standard C++ threading on WebAssembly shared memory
- high-throughput command transport and execution lanes
- portable GPU, audio, and input services
- custom linear-memory backends and OS-backed mappings
- advanced multi-memory / memory64 support
- `NEXUS_WASM`, an inline-WebAssembly escape hatch for C/C++ guests

NexusWasm is designed so that applications, game engines, plugin systems, servers, and future adapters such as VMP / CitizenFX can embed the runtime without NexusWasm depending on those hosts.

---

## Project status

Current milestone: **Phase 07 complete — Wasmtime-native async execution**.

| Phase | Status | Area |
|---|---:|---|
| 01 | ✅ | Development environment |
| 02 | ✅ | Repository + CMake infrastructure |
| 03 | ✅ | Wasmtime embedding |
| 04 | ✅ | Runtime / Module / Instance |
| 05 | ✅ | Realm / Program / ExecutionDomain |
| 06 | ✅ | Module Linking & Module Graph |
| 07 | ✅ | Wasmtime Native Async Execution |
| 08 | ⏳ | Memory Models & Standard Memory Linking |
| 09 | ⏳ | Service / Capability / Provider Foundation |
| 10 | ⏳ | Standard C++ Guest Toolchain + WASI Frontends |
| 11 | ⏳ | `NEXUS_WASM` inline WebAssembly |
| 12 | ⏳ | Filesystem Service + VFS |
| 13 | ⏳ | Network Service + Poll Service |
| 14 | ⏳ | Standard WebAssembly Shared Memory |
| 15 | ⏳ | Standard C++ Multithreading |
| 16 | ⏳ | Command Transport + Execution Lanes |
| 17 | ⏳ | GPU Service |
| 18 | ⏳ | Audio Service |
| 19 | ⏳ | Input Service |
| 20 | ⏳ | Resource Governance / Scheduler / Interruption |
| 21 | ⏳ | Compilation Cache + Performance |
| 22 | ⏳ | Custom Linear Memory Backend |
| 23 | ⏳ | OS Shared Memory Mapping |
| 24 | ⏳ | File-backed `mmap` |
| 25 | ⏳ | Advanced Multi-Memory + Memory64 |
| 26 | ⏳ | Cross-platform Hardening |
| 27 | ⏳ | Security / Fuzzing / Stress |
| 28 | ⏳ | API Stabilization + Packaging |
| 29 | ⏳ | Documentation |
| 30 | ⏳ | Examples / Tutorials |
| 31 | ⏳ | Release Candidate |

The roadmap is intentionally ambitious. A phase is considered complete only when its architecture, implementation, tests, ownership rules, error handling, and documentation are in a usable state.

See [ROADMAP.md](ROADMAP.md) for the complete plan.

---

## Design principles

### 1. Standard first

When C, C++, WebAssembly, or WASI already provide the right abstraction, NexusWasm should use it instead of replacing it with a proprietary API.

Examples:

```cpp
std::cout << "Hello from WebAssembly\n";
std::thread worker(...);
std::mutex mutex;
std::atomic<int> counter;
```

Nexus-specific guest APIs are reserved for features that do not have an appropriate standard equivalent, such as GPU access, audio devices, controlled input, mapped memory, host-engine extensions, and inline WebAssembly.

### 2. Wasmtime is the execution engine, NexusWasm is the host runtime

```text
Host Application
      │
      ▼
  NexusWasm
      │
      ▼
   Wasmtime
      │
      ▼
 WebAssembly
```

Wasmtime owns WebAssembly execution and compilation. NexusWasm owns the runtime model, host services, policies, provider resolution, resource accounting, host integration, and higher-level runtime facilities.

### 3. Host remains in control

A guest should only see resources explicitly exposed by the host.

This applies to:

- filesystem paths
- network access
- GPU adapters
- microphone/audio devices
- keyboard/mouse/gamepad input
- mapped memory
- host-engine objects

### 4. Providers are replaceable

A Nexus service can use a default implementation or a host-supplied implementation.

```text
Guest API / WASI Frontend
          │
          ▼
       Service
          │
     Policy Check
          │
          ▼
       Provider
      /        \
 Default      Host-defined
```

A game engine may, for example, provide its existing graphics device instead of allowing NexusWasm to create a second device.

### 5. No unsafe native-pointer leakage

Guests should receive typed opaque handles, WebAssembly pointers, or capability-scoped resources — not raw native pointers such as:

```text
ID3D12Resource*
VkDevice
HANDLE
int fd
```

---

## Runtime model

The intended runtime hierarchy is:

```text
Runtime
 └── Realm
      └── Program
           ├── Module Graph
           ├── Memory Model
           ├── Service Authority
           ├── Resource Accounts
           │
           ├── Main ExecutionDomain
           │    └── Wasmtime Store / Instances
           │
           └── Worker ExecutionDomains
                └── Wasmtime Stores / Instances
```

### Runtime

Owns global execution infrastructure such as the Wasmtime engine, global compiled-module caches, default providers, diagnostics, and platform-wide services.

### Realm

A logical isolation, security, and lifecycle boundary.

### Program

Represents one logical executable environment: module graph, memory model, service authority, resource accounting, and thread group.

### ExecutionDomain

Represents one Wasmtime execution lane, typically centered around one Store and the instances belonging to it.

See [docs/runtime-model.md](docs/runtime-model.md).

---

## Module linking

NexusWasm supports a real module graph rather than treating every Wasm module as an isolated black box.

Conceptually:

```text
math.wasm
  └── exports math.add

physics.wasm
  └── imports math.add

app.wasm
  ├── imports math.add
  └── imports physics.step
```

Direct WebAssembly linking is intended to remain a fast path inside a compatible ExecutionDomain / Store.

Cross-Store calls are a different problem and will use Nexus-level dynamic references/dispatch rather than pretending a Wasmtime `Func` can cross Store boundaries.

See [docs/module-linking.md](docs/module-linking.md).

---

## Native asynchronous execution

NexusWasm now supports **Wasmtime-native asynchronous execution**.

This is not `std::async`, not Asyncify, and not a worker-thread trick. It wraps Wasmtime's own suspend/resume model.

Conceptual host usage:

```cpp
// Conceptual API; exact names may evolve while the project is pre-1.0.
auto call = instance.CallAsync<int>("expensive_operation", 42);

while (call.Poll() == AsyncPollStatus::Pending)
{
    DoOtherHostWork();
}

auto result = call.TakeResult();
```

Async host imports are intended to support flows such as:

```text
Wasm executes
    ↓
async host import
    ↓
Wasmtime suspends Wasm
    ↓
control returns to host
    ↓
I/O remains pending
    ↓
host polls later
    ↓
continuation completes
    ↓
Wasmtime resumes the same Wasm execution
```

The Store execution state is explicitly tracked so unsafe operations are rejected while an incompatible async execution is active.

See [docs/async-execution.md](docs/async-execution.md).

---

## Planned Service / Capability / Provider architecture

One of the central pieces of NexusWasm is a strongly typed host-service system.

```text
                     Guest
                       │
                   Frontend
                       │
                    Service
                       │
        ┌──────────────┴──────────────┐
        ▼                             ▼
 Capability Resolver            Provider Resolver
        │                             │
        ▼                             ▼
 Effective Policy              Effective Provider
        │                             │
        └──────────────┬──────────────┘
                       ▼
                Authorization
                       │
                Resource Reserve
                       │
                       ▼
              Service RequestContext
                       │
                       ▼
                    Provider
```

The project intentionally separates:

```text
Service      != Provider
Provider     != Permission
Capability   != Service
Frontend     != Service
Policy       != Resource Usage
```

Examples of planned built-in services:

```text
Output
Filesystem
Network
Poll
Clock
Random
Environment
GPU
Audio
Input
```

A host will also be able to define custom services without modifying NexusWasm core.

See [docs/services-capabilities-providers.md](docs/services-capabilities-providers.md).

---

## Memory model

NexusWasm will distinguish at least two logical modes:

```text
Isolated
SharedAddressSpace
```

### Isolated

```text
Module A → Memory A
Module B → Memory B
```

A pointer value from A has no automatic meaning in B.

### Shared address space

```text
Module A ─┐
Module B ─┼── same WebAssembly Memory object
Module C ─┘
```

This allows normal C/C++ pointer offsets to refer to the same bytes across participating modules.

This is intentionally distinct from **WebAssembly Shared Memory**, which is required for concurrent threads and atomics.

See [docs/memory.md](docs/memory.md).

---

## `NEXUS_WASM` — planned inline WebAssembly escape hatch

`NEXUS_WASM` is planned as an advanced low-level feature for C/C++ guests that need direct access to WebAssembly features not conveniently exposed by the compiler.

Conceptual usage:

```cpp
using Add = int(*)(int, int);

Add add = NEXUS_WASM(
    Add,

    local.get 0
    local.get 1
    i32.add
);

int value = add(20, 22);
```

The important property is that resolution happens once:

```text
first resolution
    ↓
generate tiny auxiliary Wasm module
    ↓
compile/cache
    ↓
instantiate in the same Store
    ↓
insert function into the guest's function table
    ↓
return the guest ABI function reference
```

After that, calls should be ordinary WebAssembly indirect calls with **no Nexus host transition on the hot path**.

A major use case is multi-memory access from inline Wasm while normal C++ continues using memory 0.

See [docs/nexus-wasm-inline.md](docs/nexus-wasm-inline.md).

---

## GPU service — planned

The GPU subsystem is intended to be a portable graphics/compute abstraction rather than a DirectX/Vulkan/OpenGL wrapper exposed directly to the guest.

```text
Guest
  ↓
Nexus GPU API
  ↓
GpuService
  ↓
IGpuProvider
  ├── D3D provider
  ├── Vulkan provider
  ├── OpenGL provider
  ├── Metal provider
  └── Engine-owned GPU provider
```

Conceptual API:

```cpp
// Conceptual API.
auto vs = device.CreateShader(vertexShaderDesc);
auto fs = device.CreateShader(fragmentShaderDesc);

auto pipeline = device.CreateGraphicsPipeline({
    .vertexShader = vs,
    .fragmentShader = fs,
    // vertex layout, bindings, depth/blend/raster state...
});

auto cmd = device.CreateCommandList();
cmd.BindPipeline(pipeline);
cmd.BindVertexBuffer(vertices);
cmd.BindTexture(0, 0, texture);
cmd.DrawIndexed(indexCount);

queue.Submit(cmd);
```

The provider translates the portable Nexus model to the actual backend.

A host game engine may expose its **existing graphics device** instead of creating a new one. This enables future zero-copy or low-copy interoperability with engine textures, buffers, render targets, and pipelines where the host backend allows it.

This also makes libraries such as RmlUi a strong future integration target: a custom RmlUi renderer can produce Nexus GPU commands while the host decides whether those commands run on D3D, Vulkan, OpenGL, Metal, or an engine-owned renderer.

See [docs/gpu.md](docs/gpu.md).

---

## Audio service — planned

The audio subsystem will support controlled input and output:

```text
Input:
- microphone
- optional loopback capture

Output:
- speakers/headphones
```

Bulk PCM data is intended to use shared ring buffers rather than one host transition per sample or tiny block.

Conceptually:

```cpp
// Conceptual API.
auto mic = audio.OpenInput(inputDesc);
auto speaker = audio.OpenOutput(outputDesc);

auto input = mic.AcquireReadBuffer();
auto output = speaker.AcquireWriteBuffer();

ProcessAudio(input.samples, output.samples);

speaker.Submit(output);
```

Microphone access will be capability-controlled and denied by default.

See [docs/audio.md](docs/audio.md).

---

## Input service — planned

NexusWasm will expose controlled input rather than unrestricted raw desktop access.

Planned device classes:

```text
Keyboard
Pointer / Mouse
Gamepad
Future: Touch / Pen
```

Reading input and injecting/controling input are separate authorities.

A host may expose only a virtual subset:

```text
W
A
S
D
Space
Left Mouse Button
```

while hiding every other physical device or input.

Conceptual API:

```cpp
// Conceptual API.
if (keyboard.IsDown(Key::W))
{
    MoveForward();
}

while (auto event = input.PollEvent())
{
    HandleInput(*event);
}
```

See [docs/input.md](docs/input.md).

---

## Filesystem and networking — planned

NexusWasm will keep standard WASI frontends while allowing the host to replace the backend.

Filesystem example:

```text
Guest /data   → native directory
Guest /cache  → memory filesystem
Guest /assets → game-engine VFS
```

Network example:

```text
guest connect("service.internal")
        ↓
NetworkService
        ↓
custom host provider
        ↓
internal service transport
```

No guest code change is required simply because the backend changes.

See [docs/filesystem.md](docs/filesystem.md) and [docs/networking.md](docs/networking.md).

---

## Standard C++ threading — planned

The goal is for the guest to write normal C++:

```cpp
static std::atomic<int> counter = 0;

std::thread worker([]
{
    ++counter;
});

worker.join();
```

Internally, NexusWasm will map the selected current WebAssembly/WASI threading model onto separate execution domains/stores where required while sharing the same WebAssembly SharedMemory.

See [docs/threading.md](docs/threading.md).

---

## Command transport and execution lanes — planned

Some host operations must execute on a specific host thread or context.

Examples:

```text
GPU → render lane
Audio control → audio lane
Game natives → game/native lane
Physics → physics lane
```

NexusWasm will provide a generic internal transport, but guest-facing APIs remain domain-specific.

```text
Service != CommandTransport != ExecutionLane != Wasmtime Async
```

A future host adapter may expose something like:

```cpp
// Example of a host-specific extension, not Nexus Core.
cfx::ExecuteInNativeThread([]
{
    cfx::native::SomeNativeCall();
});
```

The underlying transport can safely route the operation to the required host execution lane.

See [docs/command-transport.md](docs/command-transport.md).

---

## Security model

NexusWasm assumes guest modules may be untrusted.

Core rules include:

- no unrestricted native-pointer exposure
- capability-based authority
- provider does not imply permission
- parent scopes cannot be widened by children
- hierarchical resource quotas
- typed opaque handles
- stale-handle protection
- validated command streams
- explicit memory-sharing rights
- default-denied sensitive capabilities
- fuzzing and adversarial testing before release

See [docs/security.md](docs/security.md).

---

## Long-term host integration

NexusWasm itself remains host-agnostic.

Future adapters can be thin layers:

```text
CitizenFX / VMP Adapter
        │
        ├── lifecycle integration
        ├── host-specific services
        ├── native invocation
        ├── render integration
        ├── input/audio bridges
        └── engine-specific resources
        │
        ▼
     NexusWasm
        │
        ▼
      Wasmtime
```

Formal VMP/CitizenFX integration is intentionally postponed until NexusWasm itself is release-ready.

---

## API stability

NexusWasm is currently pre-1.0.

Examples in this README that describe unfinished phases are explicitly **conceptual target APIs**. They document design intent and may change before stabilization.

Implemented APIs should always be treated as authoritative according to the repository source and current generated documentation.

---

## Documentation map

- [Architecture](docs/architecture.md)
- [Runtime model](docs/runtime-model.md)
- [Module linking](docs/module-linking.md)
- [Async execution](docs/async-execution.md)
- [Services / Capabilities / Providers](docs/services-capabilities-providers.md)
- [Memory](docs/memory.md)
- [WASI and guest toolchain](docs/wasi-and-guest-toolchain.md)
- [Filesystem](docs/filesystem.md)
- [Networking](docs/networking.md)
- [Threading](docs/threading.md)
- [Command transport](docs/command-transport.md)
- [NEXUS_WASM](docs/nexus-wasm-inline.md)
- [GPU](docs/gpu.md)
- [Audio](docs/audio.md)
- [Input](docs/input.md)
- [Resource governance](docs/resource-governance.md)
- [Security](docs/security.md)
- [Roadmap](ROADMAP.md)

---

## License

See the repository license file.
