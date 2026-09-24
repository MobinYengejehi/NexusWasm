# Command Transport and Execution Lanes

Some operations must execute on a specific host thread or context.

Examples:

```text
GPU work     → render lane
Audio control → audio lane
Game natives → game/native lane
Physics      → physics lane
```

NexusWasm therefore plans a generic internal transport and host-defined execution lanes.

## Separation of concerns

```text
Service          = semantic operation
CommandTransport = efficient movement of work/data
ExecutionLane    = target execution context
Wasmtime Async   = guest suspension/resumption
```

A future async service may use both command transport and Wasmtime async:

```text
Wasm
 ↓
async host import
 ↓
enqueue command to required lane
 ↓
Wasm suspended
 ↓
host lane finishes work
 ↓
continuation ready
 ↓
Wasm resumes
```

Guest-facing APIs remain domain-specific; the user should not have to manually serialize arbitrary raw command packets for normal operations.
