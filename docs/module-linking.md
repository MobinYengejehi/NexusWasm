# Module Linking and Module Graph

NexusWasm models multiple Wasm modules as a dependency graph.

Example:

```text
math.wasm
    exports add

physics.wasm
    imports math.add
    exports step

app.wasm
    imports math.add
    imports physics.step
```

Direct linking uses normal WebAssembly imports and exports.

## Same-Store fast path

Within a compatible ExecutionDomain, direct function linking should remain normal Wasmtime/WebAssembly linking rather than an unnecessary host RPC layer.

## Directly linkable object classes

WebAssembly imports/exports may include:

- functions
- memories
- tables
- globals

The project initially focuses on building safe graph semantics and progressively extends supported object classes.

## Cross-Store behavior

A Wasmtime function belonging to Store A must not be passed directly into Store B.

Future cross-domain references will therefore be Nexus-level references/dispatchers, not fake cross-Store Wasmtime functions.
