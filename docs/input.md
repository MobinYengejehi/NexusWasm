# Input Service

The planned Input Service exposes only the devices and controls selected by the host.

## Input classes

- keyboard
- pointer / mouse
- gamepad
- future touch / pen

## Read vs control

Reading input and injecting/controling input are separate authorities.

A guest allowed to read `W`, `A`, `S`, and `D` should not automatically be allowed to synthesize keyboard events.

## Virtual devices

The host may expose virtual devices instead of physical OS devices.

Example:

```text
Virtual Keyboard
├── W
├── A
├── S
├── D
└── Space
```

The guest does not need to know whether events came from:

- a physical keyboard
- a game engine
- a replay
- a test harness
- a network source

## Event transport

Input events are good candidates for a queue/ring-buffer design.

```text
KeyDown
KeyUp
PointerMove
PointerButton
Wheel
GamepadAxis
```

## Security

Unrestricted OS-global input injection is not intended as a default capability.
