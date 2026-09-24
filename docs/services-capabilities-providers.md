# Services, Capabilities, Policies, Providers, and Frontends

This document describes the planned host-service model.

## Service

A Service is the semantic identity of a host capability domain.

Examples:

```text
OutputService
FileSystemService
NetworkService
GpuService
AudioService
InputService
```

## Capability

A Capability answers:

> Is this scope allowed to use this Service?

## Policy

A Policy answers:

> If allowed, what operations and limits apply?

Example:

```text
NetworkPolicy
- TCP allowed
- UDP denied
- bind only to localhost
- ports 3000-3010
- maximum 5 sockets
```

## Provider

A Provider is the mechanism used to perform the operation.

Example:

```text
NetworkService
   ├── NativeSocketProvider
   ├── VirtualNetworkProvider
   └── EngineNetworkProvider
```

A Provider does not grant authority by existing.

## Frontend

A Frontend is the guest ABI/API through which an operation arrives.

Multiple frontends may target one Service:

```text
WASI Preview 1 ─┐
WASI Preview 2 ─┼→ FileSystemService
Nexus extension ┘
```

## RequestContext

Providers should receive only service-specific context.

Conceptual example:

```cpp
struct NetworkRequestContext
{
    RequestOrigin origin;
    const EffectiveNetworkPolicy& policy;
    NetworkResourceAccount& resources;
};
```

A Network provider should not receive unrelated GPU or filesystem authority.

## Provider resolution

Planned provider lookup:

```text
Runtime
 ↓
Realm
 ↓
Program
 ↓
Instance
```

Nearest explicit override wins.

## Capability composition

Capability inheritance is restrictive.

```text
Realm:   UDP denied
Program: UDP allowed

Effective: UDP denied
```

Children cannot widen parent authority.

## Resource accounting

Policy and usage are separate.

```text
Policy: max sockets = 10
Usage:  currently open = 4
```

Quota accounting is hierarchical so many children cannot bypass a parent limit.

## Custom services

A major design goal is allowing a host to define a new strongly typed Service without modifying NexusWasm core.
