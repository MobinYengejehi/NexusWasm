# Filesystem Service

The planned filesystem architecture preserves standard guest APIs while allowing host-controlled storage backends.

```text
std::ifstream / libc
      ↓
WASI filesystem frontend
      ↓
FileSystemService
      ↓
Policy
      ↓
IFileSystemProvider
```

## Virtual mounts

Example:

```text
/data   → host native directory
/assets → engine asset VFS
/cache  → in-memory filesystem
/config → read-only provider
```

## Planned security controls

- read/write/create/delete/rename rights
- path normalization
- `..` escape prevention
- symlink handling
- mount boundaries
- descriptor quotas
- file-size/storage quotas

Native file descriptors or OS handles should not be exposed directly to the guest.
