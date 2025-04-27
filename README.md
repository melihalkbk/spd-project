# 🚀 spd-project

## 🎮 Avoidance Game (OpenGL)

This project is an **Avoidance Game** developed using OpenGL. The player must dodge obstacles in a dynamic gameplay environment.

## ✨ Features
- Developed with OpenGL using 2D graphics.
- Randomly generated obstacles.
- The player must avoid obstacles for a certain duration.
- Various sound effects included.
- Level and scoring system.
- Smooth transitions and particle effects.
- Collectible Power-Ups with special effects.

## 🛠 Requirements
To run this project, you need the following dependencies installed on your system:

- **C++ Compiler** (GCC, Clang, or MSVC)
- **OpenGL Libraries**
- **GLFW** (for window and input management)
- **GLEW** (for OpenGL extensions)
- **SFML** (only for audio system)

### 📌 Linux & macOS Dependencies:
```sh
sudo apt-get install libglfw3-dev libglew-dev libsfml-dev
```

### 📌 Windows:
You need to manually download and include the dependencies in your project.

## 🔧 Installation
Clone and build the project:

```sh
git clone https://github.com/melihalkbk/spd-project.git
cd spd-project
mkdir build
cd build
cmake ..
make
```

## 🎯 Usage
After building, you can start the game with:

```sh
./compile
```

---

## 🎮 Controls
| Key            | Action                          |
|----------------|----------------------------------|
| ← / → (Arrow Keys) | Move the player left or right |
| Enter          | Start or restart the game        |
| P              | Pause or resume the game         |
| M              | Mute or unmute the background music |

---

## ⚡ Power-Ups
| Power-Up | Effect |
|:---------|:-------|
| **Speed** | Temporarily increases player movement speed. |
| **Block Reset** | Clears all current obstacles from the screen. |
| **Invisibility** | Grants temporary invisibility and immunity to collisions. |
| **Time Slow** | Slows down the overall game speed temporarily. |
| **Shield** | Provides a temporary protective shield against collisions. |
| **Extra Life** | Grants an additional life to the player. |


---

## 📂 Project Structure
| Path | Description |
|------|-------------|
| `main.cpp` | Main application file. Contains game logic, rendering, sound handling, and input processing. |
| `sounds/` | Directory containing sound effects like collision, pickup, level-up, and game-over sounds. |
| `.vscode/` | Visual Studio Code settings for the project (build configurations, IntelliSense settings). |
| `.git/` | Git version control metadata. (Not necessary for running the project) |
| `README.md` | Project description file (this file). |

---

## 🤝 Contributing
Want to contribute? Follow these steps:

1. Fork the repository.
2. Create a new branch: `git checkout -b new-feature`
3. Make your changes and commit them.
4. Push to your branch: `git push origin new-feature`
5. Open a pull request.

---

## 📜 License
This project is licensed under the MIT License. See the `LICENSE` file for more details.

