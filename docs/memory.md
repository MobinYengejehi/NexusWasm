# Memory Model

## Isolated memory

```text
Module A → Memory A
Module B → Memory B
```

In wasm32, a C/C++ pointer is normally an offset into a particular linear memory.

Therefore an integer pointer value such as `0x1000` in Memory A does not automatically refer to the same bytes in Memory B.

## Shared address space

```text
Module A ─┐
Module B ─┼→ same Memory object
Module C ─┘
```

When modules import the same Wasmtime/WebAssembly Memory object as their normal address space, ordinary C/C++ pointer offsets can refer to the same bytes.

## Shared address space vs WebAssembly Shared Memory

These are intentionally different concepts.

- Same Memory object: multiple modules refer to one linear memory.
- WebAssembly Shared Memory: memory designed for concurrent agents/threads using atomics.

## Future advanced memory

Later phases add:

- custom linear-memory backends
- OS-backed shared regions
- file-backed mappings
- multi-memory
- memory64
