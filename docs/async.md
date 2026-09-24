# Native asynchronous execution

NexusWasm uses Wasmtime's native asynchronous execution support.

It does not emulate asynchronous execution with `std::async`,
worker threads, Asyncify, JSPI, or custom WebAssembly stack
unwinding.

## Call and CallAsync

`Instance::Call<T>()` enters WebAssembly synchronously.

`Instance::CallAsync<T>()` creates a native
`wasmtime_call_future_t`.

Creating the asynchronous call does not busy-spin until the
WebAssembly function finishes. The host explicitly drives the
execution by calling `AsyncCall<T>::Poll()`.

A poll returns:

- `AsyncPollStatus::Pending`
- `AsyncPollStatus::Ready`

When Ready, `TakeResult()` returns the final Nexus `Result<T>`.

## Execution model

WebAssembly does not execute on an automatically managed
background thread.

Execution occurs when the Wasmtime future is polled.

Wasmtime uses its native asynchronous stack/fiber mechanism to
allow execution to suspend and later resume.

## Lifetime

Wasmtime requires all arguments passed to an asynchronous call
to remain alive and unmodified until the native future is
deleted.

NexusWasm therefore stores the following in the AsyncCall state:

- the ExecutionDomain/Store lifetime
- the Store execution lease
- the resolved Wasmtime function
- argument storage
- result storage
- trap storage
- error storage
- the Wasmtime call future

The Wasmtime future is always destroyed before the Store
execution lease is released.

Destroying a pending AsyncCall cancels/drops the native Wasmtime
future and releases the Store safely.

## Store execution state

Each ExecutionDomain has one canonical StoreExecutionState.

It tracks:

- whether the Store is idle or exclusively in use
- whether a native async future currently owns the Store
- whether the Store has become async-required

Only one active native future is permitted for one Store.

While a future is active, Nexus rejects:

- another asynchronous call
- a synchronous call
- synchronous instantiation
- ModuleGraph instantiation
- conflicting linker mutations

Different Stores remain independent.

## Async-required Stores

Some Wasmtime features make asynchronous entrypoints mandatory.

Native async host functions are one example.

Once such a function becomes part of an instantiated Store,
Nexus marks the Store as async-required.

That state is sticky.

The Store may return from `AsyncExecutionActive` to `Idle`, but
synchronous WebAssembly entrypoints remain unavailable.

Subsequent execution must use native async entrypoints.

## Async host functions

ExecutionDomain can define a low-level Wasmtime-native async host
function.

The guest still observes a normal synchronous WebAssembly call:

    call $host_operation

Internally the flow is:

    guest call
        -> host async callback
        -> Wasmtime continuation
        -> Pending
        -> control returned to the host
        -> future polled later
        -> continuation Ready
        -> Wasm resumes

The low-level Phase 07 host-function API currently supports only
i32, i64, f32, and f64 values.

It is not the Nexus Service/Capability/Provider API.

Future Service Frontends will use the same native mechanism while
providing service-specific RequestContext, policy, capability,
and provider resolution.

## ModuleGraph

The Phase 06 ModuleGraph remains a synchronous activation API.

Its Store operations participate in StoreExecutionState.

Therefore graph activation is rejected while a native future is
active.

The graph also recognizes registered async host imports and
reports that asynchronous activation is required rather than
misclassifying the import as an unresolved module dependency.

An asynchronous ModuleGraph activation state machine is not part
of Phase 07.

## Thread safety

Native async does not make a Wasmtime Store concurrently usable.

NexusWasm serializes Store-owning operations through
StoreExecutionState.

A Store must not be concurrently manipulated by multiple host
threads while an asynchronous future is active.

Different ExecutionDomains/Stores remain independent.

## Not implemented in Phase 07

This phase does not provide:

- guest C++ `co_await`
- a guest coroutine scheduler
- `std::async` emulation
- worker-thread Wasm execution
- Asyncify
- JSPI
- Emscripten async
- asynchronous ModuleGraph activation
- reference-type async argument/result support
