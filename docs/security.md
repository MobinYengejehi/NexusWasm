# Security Model

NexusWasm should treat guest WebAssembly as untrusted.

## Core principles

### No raw native-pointer authority

Guest code should not receive unrestricted host pointers to native objects or host memory.

### Provider does not imply permission

A globally installed provider does not make that service available to every guest.

### Restrictive capability inheritance

Child scopes may narrow parent authority but not widen it.

### Hierarchical quotas

Resources are accounted across Realm, Program, and Instance scopes.

### Typed opaque handles

Handles should carry enough identity/lifetime information to reject stale, cross-instance, or type-confused use.

### Validate untrusted command buffers

GPU/audio/input/command-transport packets should be treated as attacker-controlled guest memory.

Validate:

- opcode
- size
- offsets
- counts
- ownership
- resource type
- policy
- quotas

### Sensitive services default to deny

Examples include:

- microphone input
- input injection
- raw networking
- arbitrary mapped host resources

## Async lifetime safety

Pending operations must not keep dangling references to frontend stack objects or destroyed Instances.

## `NEXUS_WASM`

Inline Wasm remains sandboxed WebAssembly and may access only explicitly supplied imports/memories.

## Release hardening

Before release, the project roadmap includes fuzzing, sanitizers, stress testing, lifecycle testing, cache-collision testing, malformed-command testing, and quota-bypass testing.
