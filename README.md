# gameprototype

A real-time, top-down survival prototype written in C++17, with a raylib desktop interface.

Explore a mostly hidden map, use scans to reveal your surroundings, and use sound to draw enemies away from supplies or into a fire trap. Ammunition is limited, so clearing the level depends on stealth, positioning and crafted items.

## Gameplay

The goal is to survive and defeat every enemy in the level.

- **Scan and explore.** Scans reveal terrain within line of sight and briefly show enemy positions. Discovered terrain stays visible. In combat, scans run automatically and become more frequent as health falls.
- **Move quietly.** Stealth reduces enemy sight range but slows movement. Walls block vision.
- **Use sound.** Pistol shots and thrown decoys create noise events that nearby enemies investigate. Noise travels through walls; enemies use walkable paths to reach its source. Enemies already chasing the player ignore distractions.
- **Manage limited supplies.** Use the pistol against isolated enemies, lure groups together, and use Molotovs to deal area damage. Fire also damages the player.
- **Collect and craft.** Pick up ammunition and materials, then craft Molotovs and decoys outside combat.
- **Restart a run.** Victory and defeat screens offer a fresh session or exit.

The prototype currently has one configurable level and simple geometric graphics. Sound is a gameplay simulation; there is no audio playback.

## Controls

| Input | Action |
| --- | --- |
| W / A / S / D | Move |
| Left Shift | Toggle stealth |
| Space | Scan outside combat |
| Mouse | Aim |
| Left click | Fire once, or throw the selected item |
| R | Reload |
| V / B | Select or cancel Molotov / decoy |
| 1 / 2 | Craft Molotov / decoy |
| Esc | Exit |

After a successful throw, the next click fires the pistol again. See [HOW_TO_PLAY.txt](HOW_TO_PLAY.txt) for the full play guide.

## Code organization

| Location | Responsibility |
| --- | --- |
| `game_core/World` | Map data, coordinates, line of sight and shared ray queries |
| `game_core/Actors` | Player movement and health; enemy states, attacks and pathfinding |
| `game_core/Perception` | Scanning, temporary enemy observations and sound events |
| `game_core/Weapons` | Pistol, Molotovs, decoys and shared throwing rules |
| `game_core/Resources` | Supply pickup, materials and crafting |
| `game_core/Sessions` | Session ownership, frame updates and victory/defeat rules |
| `game_desktop` | raylib input/rendering, interface and level configuration loading |
| `game_tests` | Core behavior tests registered with CTest |

The `game_core` static library is separate from the raylib desktop target. `GameSession` receives input and elapsed time for each frame; the desktop layer reads session state to draw the game. The core tests run without opening a window.

Some implementation choices:

- Enemies use explicit states for wandering, chasing, waiting after losing sight, and investigating sound. Breadth-first search finds paths around walls.
- Map ray queries share grid traversal for visibility, throw landing positions and the first enemy hit by a shot.
- Throw previews use discovered terrain, while actual throws use the real map. An unknown wall can stop an item before its intended target.
- Movement, reloads, scanning and burning areas advance using elapsed seconds.
- Core coordinates use `(row, column)`; conversion to screen coordinates happens in the desktop layer.

## Build and run

Run the commands below from the repository root, which contains `CMakeLists.txt`.

Requirements:

- A C++17 compiler, CMake and Ninja.
- Windows: Visual Studio with the C++ desktop workload and CMake tools.
- Linux: GCC and the raylib development dependencies, including OpenGL, ALSA and X11/Wayland headers.

### Windows desktop

In Visual Studio, open the repository using **File > Open > Folder**, select an x64 configuration, build the project and choose `gameprototype.exe` as the startup target.

Alternatively, run these commands in a Visual Studio Developer PowerShell with the x64 C++ tools and Ninja available:

```powershell
cmake -S . -B build/windows -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/windows
ctest --test-dir build/windows --output-on-failure
.\build\windows\gameprototype.exe
```

CMake copies `level.txt` beside the executable. Keep that file with the executable when moving the game to another folder.

### Linux core tests

```bash
cmake -S . -B ~/build/gameprototype -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build ~/build/gameprototype --target game_tests enemy_tests gun_tests -j 2
ctest --test-dir ~/build/gameprototype --output-on-failure
```

## Tests and validation

The project uses assertion-based C++ tests, registered as three CTest entries. Assertions remain enabled for the test targets in both Debug and Release builds.

| CTest entry | Examples of checked behavior |
| --- | --- |
| `map_rules` | Map boundaries, movement cooldowns, scanning and throw previews |
| `enemy_behavior` | Detection, pathfinding, sound investigation and damage |
| `gun_behavior` | Shot range, nearest-enemy hits and ammunition handling |

Verified configurations:

- Windows/MSVC Debug and Release: all 3 CTest entries passed in each configuration.
- Ubuntu 24.04 (WSL 2), GCC 13 Debug core tests: all 3 CTest entries passed.

## Level configuration

`game_desktop/level.txt` defines terrain, spawn positions, supplies, gameplay parameters and display settings. Coordinates are zero-based `(row, column)`. The loader validates required sections and values and reports configuration errors at startup.

The application reads the copy beside the executable. After editing the source configuration, rebuild the desktop target to copy it to the output directory. Restarting from the result screen reloads that configuration and creates a new session.
