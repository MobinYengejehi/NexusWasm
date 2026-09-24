# Resource Governance

NexusWasm plans hierarchical resource limits rather than flat per-instance limits only.

```text
Realm
 └ Program
    └ Instance
```

Example:

```text
Realm socket limit    = 100
Program socket limit  = 10
Instance socket limit = 2
```

The system must enforce all three simultaneously.

Ten Instances must not bypass the Program limit simply because each Instance individually remains below two sockets.

## Resource accounts

Service-specific usage remains separate:

```text
NetworkResourceAccount
FileSystemResourceAccount
GpuResourceAccount
AudioResourceAccount
```

Common lower-level accounting primitives may be shared.

## Execution governance

Later work also includes:

- Wasmtime fuel
- epoch interruption
- timeouts
- cancellation
- stack limits
- memory/table limits
- pending async operation limits
