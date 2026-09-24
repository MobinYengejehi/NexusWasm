# Standard C++ Guest Toolchain and WASI

The goal is to let ordinary C/C++ code run on NexusWasm without proprietary wrappers for standard functionality.

Example:

```cpp
#include <iostream>
#include <vector>

int main()
{
    std::vector<int> values{1, 2, 3};
    std::cout << values.size() << '\n';
}
```

Target flow:

```text
C++
 ↓
libc++ / libc
 ↓
WASI ABI
 ↓
Nexus WASI Frontend
 ↓
Nexus Service
 ↓
Provider
```

## Why keep WASI as the frontend?

It allows standard toolchains and libraries to work without learning Nexus-specific replacements.

## Why keep Nexus between WASI and the final backend?

Because the host still needs control over:

- policy
- virtualization
- provider replacement
- resource accounting
- custom engine integration

Where upstream Wasmtime/wasmtime-wasi implementations already solve a problem well, NexusWasm should reuse them behind Nexus abstractions rather than reimplementing them without reason.
