# Native Async Execution

NexusWasm supports Wasmtime's native asynchronous execution model.

## What it is

A Wasm execution can suspend while an async host operation remains pending, return control to the embedding host, and later resume the same execution.

```text
Wasm execution
    ↓
async host import
    ↓
Wasmtime suspension
    ↓
Host regains control
    ↓
Operation pending
    ↓
Poll later
    ↓
Continuation ready
    ↓
Wasm resumes
```

## What it is not

It is not:

- `std::async`
- a worker thread pretending to be async
- Asyncify
- JSPI
- guest C++ coroutines

## Conceptual API

```cpp
// Conceptual; exact pre-1.0 public names may evolve.
auto future = instance.CallAsync<int>("foo", 10);

while (future.Poll() == AsyncPollStatus::Pending)
{
    DoOtherHostWork();
}

auto result = future.TakeResult();
```

## Lifetime

The async call object must own every argument/result/error/trap structure Wasmtime requires to remain valid while the future exists.

## Store execution state

A Store with an active Wasmtime async call may be restricted from other operations depending on the current Wasmtime API contract.

NexusWasm therefore tracks one canonical Store execution state and rejects incompatible operations rather than relying on caller discipline.

Different Stores remain independent.
