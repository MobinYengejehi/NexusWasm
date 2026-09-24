# Runtime Model

## Runtime

Global owner of the Wasmtime engine and runtime-wide infrastructure.

Planned responsibilities include:

- Wasmtime Engine/configuration
- global compiled-module caches
- default provider registry
- diagnostics
- platform services

## Realm

A lifecycle/security boundary that can restrict child Programs.

Typical use cases include isolating multiple tenants, applications, plugins, or host-defined sandboxes.

## Program

A logical executable environment.

A Program may own or reference:

- module graph
- memory model
- service authority
- resource accounts
- main execution domain
- worker execution domains

## ExecutionDomain

An execution lane centered around a Wasmtime Store and the objects that belong to that Store.

Direct Wasmtime objects such as functions and instances cannot be assumed to be portable across Stores.

This matters for:

- direct module linking
- threads
- async Store rules
- `NEXUS_WASM` function tables
- Store-local caches
