# Architecture

NexusWasm is intentionally split into three broad layers:

```text
Guest
  │
  ▼
Nexus frontends and runtime services
  │
  ▼
Wasmtime + platform backends
```

## Execution layer

Wasmtime remains responsible for WebAssembly compilation and execution.

NexusWasm builds a higher-level runtime on top of it rather than reimplementing a WebAssembly engine.

## Runtime layer

```text
Runtime
 └ Realm
    └ Program
       ├ Module Graph
       ├ Memory Model
       ├ Service Authority
       └ ExecutionDomains
```

## Host-service layer

Planned services are represented through explicit service identity, policy, provider resolution, and service-scoped request contexts.

```text
Frontend
   ↓
Service
   ↓
Capability/Policy
   ↓
Provider
```

## Provider replacement

Provider resolution is planned to support scoped overrides:

```text
Runtime default
   ↓
Realm override
   ↓
Program override
   ↓
Instance override
```

The nearest explicit provider override wins.

Capability inheritance is different: child scopes may restrict parent authority but must not widen it.

## Async

Wasmtime-native async is treated as execution suspension/resumption, not as a generic thread pool.

```text
Wasm
 ↓
async host import
 ↓
Wasmtime suspends
 ↓
Host resumes later
```

## Command transport

Command transport is separate from async execution.

```text
Service          = what an operation means
CommandTransport = how work moves between threads
ExecutionLane    = where it must run
Wasmtime Async   = how Wasm suspends/resumes
```

This separation is important for GPU, audio, and future engine-specific integrations.
