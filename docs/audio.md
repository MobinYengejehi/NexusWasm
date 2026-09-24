# Audio Service

The planned audio subsystem exposes controlled audio input and output.

## Input

- microphone
- optional loopback capture where supported

## Output

- speakers
- headphones
- virtual/engine audio sinks

## Provider model

```text
Guest
 ↓
Audio Service
 ↓
Audio Policy
 ↓
IAudioProvider
 ↓
Default OS backend OR host-defined backend
```

## Data plane

Bulk PCM data should use shared/ring-buffer transport where appropriate.

```text
Microphone
   ↓
Host audio thread
   ↓
Shared ring buffer
   ↓
Wasm
```

Output is the reverse direction.

## Control plane

Small commands such as start/stop/pause/gain/device-switch can use command transport.

## Realtime safety

The realtime audio callback path should avoid arbitrary blocking guest execution, heavy allocations, logging, filesystem I/O, or other unpredictable work.

## Security

Microphone access is intended to be denied by default and granted explicitly by host policy.
