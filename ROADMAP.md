# NexusWasm Roadmap

This roadmap describes the current planned development order.

Legend:

- ✅ Complete
- 🚧 Current / active foundation
- ⏳ Planned

## Completed foundation

### ✅ 01 — Development Environment

Establish the supported compiler, CMake, platform SDK, Wasmtime, WASI SDK, and tooling environment.

### ✅ 02 — Repository + CMake

Create the standalone NexusWasm repository, library target, tests, install/export support, and clean package structure.

### ✅ 03 — Wasmtime Embedding

Embed Wasmtime through its C API and prove minimal compile / instantiate / call functionality without leaking Wasmtime types into the public architecture.

### ✅ 04 — Runtime / Module / Instance

Introduce the first public runtime objects and ownership model.

### ✅ 05 — Realm / Program / ExecutionDomain

Introduce logical isolation and execution boundaries.

### ✅ 06 — Module Linking & Module Graph

Support direct standard WebAssembly imports/exports between modules in a compatible execution domain and manage dependencies as a graph.

### ✅ 07 — Wasmtime Native Async Execution

Wrap Wasmtime's native async execution model, including asynchronous exported calls, polling, Store execution-state protection, and native async host imports.

---

## Runtime semantics

### ⏳ 08 — Memory Models & Standard Memory Linking

Formalize isolated memories vs a shared address-space model using standard WebAssembly Memory objects.

Target:

```text
Isolated:
A → Memory A
B → Memory B

SharedAddressSpace:
A ─┐
B ─┼→ same Memory
C ─┘
```

### ⏳ 09 — Service / Capability / Provider Foundation

Build the central host-service architecture.

Target separation:

```text
Service      = semantic identity
Capability   = authority
Policy       = restrictions
Provider     = mechanism/backend
Frontend     = guest ABI/API entry
RequestContext = service-scoped request state
ResourceAccount = current usage
```

### ⏳ 10 — Standard C++ Guest Toolchain + WASI Frontends

A normal C/C++ WASI program should run without Nexus-specific source changes.

Target:

```cpp
std::cout << "hello\n";
```

should flow through a Nexus OutputService while remaining standard C++ guest code.

### ⏳ 11 — `NEXUS_WASM`

Add a low-level inline-WebAssembly escape hatch for C/C++ guests.

Primary goals:

- lazy compilation
- global compiled-module cache
- per-Store instantiated cache
- function-table integration
- zero host transition on resolved hot calls
- multi-memory access

---

## Host services

### ⏳ 12 — Filesystem Service + VFS

Support WASI filesystem frontends with replaceable providers and virtual mounts.

### ⏳ 13 — Network Service + Poll Service

Support standard networking with granular host policy and portable readiness/polling semantics.

### ⏳ 14 — Standard WebAssembly Shared Memory

Support true WebAssembly shared memory for concurrent worker execution and atomics.

### ⏳ 15 — Standard C++ Multithreading

Target ordinary C++ guest code:

```cpp
std::thread
std::mutex
std::condition_variable
std::atomic
thread_local
```

### ⏳ 16 — Command Transport + Execution Lanes

Create high-throughput internal queues and host-defined execution lanes for thread-affine services.

### ⏳ 17 — GPU Service

Portable GPU compute/rendering abstraction with replaceable providers, command batching, resource validation, multi-GPU support, and external resource sharing.

### ⏳ 18 — Audio Service

Microphone capture, output, device virtualization, ring-buffer transport, and optional mixing/resampling.

### ⏳ 19 — Input Service

Controlled keyboard, pointer, gamepad, and virtual input exposure with separate read/control authority.

---

## Governance and performance

### ⏳ 20 — Resource Governance / Scheduler / Interruption

Hierarchical quotas, execution budgets, cancellation, timeouts, fuel/epoch integration, and lifecycle cleanup.

### ⏳ 21 — Compilation Cache + Performance

Consolidate compiled-module caching and build reproducible benchmark suites.

---

## Advanced memory

### ⏳ 22 — Custom Linear Memory Backend

Use official Wasmtime custom-memory hooks to provide Nexus-controlled virtual memory behavior.

### ⏳ 23 — OS Shared Memory Mapping

Map explicitly authorized shared backing objects into Nexus-managed guest address spaces.

### ⏳ 24 — File-backed `mmap`

Provide portable emulated and, where safe, real OS-backed file mappings.

### ⏳ 25 — Advanced Multi-Memory + Memory64

Generalize multi-memory beyond the minimum required by `NEXUS_WASM` and investigate production-ready memory64 support.

---

## Release hardening

### ⏳ 26 — Cross-platform Hardening

Initial production targets:

- Windows x86_64
- Linux x86_64
- macOS arm64

Later targets may include Linux arm64 and Windows arm64.

### ⏳ 27 — Security / Fuzzing / Stress

Treat guest Wasm as malicious and fuzz parsers, frontends, command streams, mappings, service registries, and lifecycle edges.

### ⏳ 28 — API Stabilization + Packaging

Finalize public API shape and support normal CMake consumption:

```text
add_subdirectory
FetchContent
find_package(NexusWasm)
prebuilt packages
```

### ⏳ 29 — Documentation

Complete and source-review documentation for every public subsystem.

### ⏳ 30 — Examples / Tutorials

Provide small, buildable examples for all major use cases.

### ⏳ 31 — Release Candidate

Final CI, sanitizer, fuzzing, documentation, packaging, benchmark, license, and public-API review.

---

## After NexusWasm release readiness

Only after the standalone runtime is ready:

```text
NexusWasm release
      ↓
VMP adapter
      ↓
VMP PR
      ↓
CitizenFX / FiveM work later
```

The NexusWasm core must remain independent from GTA, RAGE, VMP, and CitizenFX.
