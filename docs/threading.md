# Standard C++ Multithreading

The target guest experience is ordinary C++.

```cpp
#include <atomic>
#include <thread>

static std::atomic<int> value{0};

int main()
{
    std::thread worker([]
    {
        ++value;
    });

    worker.join();
}
```

## Intended runtime model

```text
Program
 ├── Main ExecutionDomain / Store / Instance
 └── Worker ExecutionDomains / Stores / Instances
          │
          └── shared WebAssembly SharedMemory
```

The exact implementation will follow the current supported WebAssembly/WASI threading model at the time of implementation.

## Goals

- `std::thread`
- `std::mutex`
- `std::condition_variable`
- `std::atomic`
- `thread_local`
- shared C/C++ globals and heap where the selected ABI requires it

Threading is intentionally distinct from Wasmtime async execution.
