# Networking and Polling

NexusWasm plans to preserve standard WASI networking frontends while introducing host-controlled policy and replaceable backends.

## Policy examples

```text
TCP: allowed
UDP: denied
DNS: allowed
Bind: localhost only
Ports: 3000-3010
Maximum sockets: 5
```

## Provider virtualization

A host may map a logical network endpoint to something that is not an OS socket.

```text
guest connect("service.internal")
      ↓
NetworkService
      ↓
VirtualNetworkProvider
      ↓
internal host service
```

## Polling

Guest semantics remain portable.

Backend implementations may use platform mechanisms such as epoll, kqueue, or Windows-specific readiness mechanisms without exposing those APIs to guest code.

## Async

Network waits should integrate with Wasmtime-native async execution rather than blocking a Store or wrapping blocking work in `std::async`.
