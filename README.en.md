[日本語](README.md) | [English](README.en.md)

# Medaka — A palm-sized aquarium

A quiet aquarium with nine medaka for M5Stack StopWatch C152. Accelerometer and three-axis gyroscope input make the water and plants respond to shaking and twisting the device.

![Aquarium preview](docs/preview.png)

<details>
<summary>View animated preview</summary>

![Aquarium preview](docs/preview.gif)

</details>

*Rendered using the simulation and scene code shared with the firmware. This is not a photograph or recording of the device.*

## Features

- Large waves and gradually decaying motion after the device stops.
- Tail motion, burst-and-glide swimming, schooling, separation and a startle response.
- Swaying plants, depth shading and subtle lighting.
- No clock or menu overlay during viewing, and no sound or vibration.

## Controls

| Action | Response |
| --- | --- |
| Tilt sideways | Tilt the free surface |
| Shake or twist | Excite waves using acceleration and angular velocity |
| Tap the screen | Create a ripple at that horizontal position |
| Click yellow A | Toggle normal / dim viewing |
| Click blue B | Create a central ripple |

Settings are not persisted: restarting returns to normal viewing. If IMU initialization fails, a message appears while autonomous swimming and touch ripples remain available.

## Build and flash

For **StopWatch C152 only**. Install PlatformIO and run these commands from the repository folder:

```sh
pio run
pio device list
pio run -t upload --upload-port <your-confirmed-StopWatch-port>
```

Flashing replaces the current application. If download mode is needed, connect USB, hold the power button for about two seconds and release it when the green LED lights up. If the device remains in download mode after flashing, briefly press the power button.

M5Unified 0.2.21 and M5GFX 0.2.28 are pinned to commits. The first build needs network access to download dependencies. The application itself implements no Wi-Fi connection, external transmission, cloud integration or credential configuration.

## Validation

```sh
c++ -std=c++17 -O2 test/physics.cpp -o /tmp/medaka-physics
/tmp/medaka-physics
c++ -std=c++17 -O2 test/preview.cpp -o /tmp/medaka-preview
/tmp/medaka-preview 180
```

Preview frames are written to `/tmp/medaka-frame-000.ppm` and subsequent numbered files.

Verified:

- Target build, USB flashing and all transferred hashes.
- Normal boot, IMU and PSRAM initialization, advancing frame counts and gyroscope responses.
- A ten-minute simulated acceleration, three-axis rotation and tap stress test; fish containment; wave mean displacement and decay.
- Synthetic gyro-only input produces a peak bulk-motion component of 38.8px, a 16.5px maximum during the first two seconds after stopping and less than 1px at ten seconds.

Physical shake direction and appearance, touch, buttons and battery life remain unverified. Rendering is scheduled for up to approximately 30fps; actual 30fps performance has not been established. CI is not configured; the results above are local checks.

Send `?` over USB serial at 115200bps for IMU state, frame count, tilt, activity, gyro Z, bulk water displacement and free heap.

## Implementation

| File | Purpose |
| --- | --- |
| `src/World.h` | Fixed 120Hz water and fish simulation; accelerometer and gyro input |
| `src/Scene.h` | Shared aquarium renderer |
| `src/main.cpp` | Device display, sensors and controls |
| `test/physics.cpp` | Numerical simulation checks |
| `test/preview.cpp` | Desktop reference renderer |

This is a lightweight 2D approximation, not a full 3D fluid solver. The visible surface is limited to 12–240px. Extreme tilts and upside-down rotation are not reproduced.

## Official references

- [StopWatch specifications and controls](https://docs.m5stack.com/en/core/StopWatch)
- [M5Unified](https://github.com/m5stack/M5Unified)
- [M5GFX](https://github.com/m5stack/M5GFX)
