# `NEXUS_WASM`

`NEXUS_WASM` is a planned advanced low-level facility for embedding raw WebAssembly instruction bodies in a C/C++ guest.

It is similar in purpose to inline assembly, but the embedded instruction language is WebAssembly and execution remains sandboxed.

## Conceptual API

```cpp
using Add = int(*)(int, int);

Add add = NEXUS_WASM(
    Add,

    local.get 0
    local.get 1
    i32.add
);

int answer = add(20, 22);
```

## Resolution model

```text
NEXUS_WASM
   ↓
canonical cache key
   ↓
per-Store cache?
   ├── hit → return existing function-table slot
   └── miss
         ↓
     global compiled-module cache?
         ↓
     generate tiny auxiliary Wasm module
         ↓
     compile if needed
         ↓
     instantiate in current Store
         ↓
     retrieve exported inline function
         ↓
     insert funcref into guest table
         ↓
     return guest function-pointer representation
```

## Hot path

After resolution:

```cpp
add(1, 2);
```

should be an ordinary WebAssembly indirect call.

The hot call path must not perform:

- host callback
- hashing
- cache lookup
- compilation
- module lookup

## Multi-memory

A major use case is accessing additional memories from inline Wasm.

```text
memory 0 = normal C/C++ memory
memory 1 = Nexus secondary/shared memory
memory 2 = another authorized memory
```

Inline modules receive only memories explicitly authorized for the current environment.

## Security

`NEXUS_WASM` does not provide arbitrary host/native memory access.

It only accesses WebAssembly memories/imports deliberately supplied to the generated auxiliary module.
