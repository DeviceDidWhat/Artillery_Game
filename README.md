# Artillery Game

A classic 2D artillery game built with GTK4 and Cairo graphics. Battle against another player on dynamically generated terrain with various weapons and realistic physics!

## 🎮 Features

### Gameplay
- **Two-player turn-based combat** - Classic artillery gameplay
- **Dynamic terrain generation** - Procedurally generated landscapes with hills, valleys, and texture
- **Realistic physics** - Gravity, wind effects, and projectile trajectories
- **Multiple weapon types** with unique properties:
  - **Small Missile** - Basic projectile with moderate damage
  - **Big Missile** - High damage explosive with large blast radius
  - **Drill** - Penetrates through terrain before exploding
  - **Cluster Bomb** - Splits into multiple sub-projectiles
  - **Nuke** - Devastating weapon with massive explosion radius

### Visual Effects
- **Particle systems** - Realistic explosion debris and effects
- **Gradient explosions** - Beautiful radial explosion graphics
- **Terrain deformation** - Landscape changes based on weapon impacts
- **Visual feedback** - Health bars, weapon indicators, and status displays
- **Environmental details** - Grass, rocks, shadows, and terrain textures

### Game Mechanics
- **Wind system** - Dynamic wind affects projectile trajectories
- **Tank movement** - Limited moves per turn for strategic positioning
- **Health system** - Damage based on proximity to explosions
- **Scoring system** - Track wins across multiple rounds
- **Pause functionality** - Game can be paused and resumed

## 🎯 Controls

| Key | Action |
|-----|--------|
| `←/→` | Adjust cannon angle |
| `↑/↓` | Adjust firing power |
| `W/S` | Cycle through weapons |
| `A/D` | Move tank left/right (limited moves per turn) |
| `Space` | Fire weapon |
| `R` | Reset game (new round) |
| `P` | Pause/unpause game |

## 🛠️ Technical Details

### Built With
- **GTK4** - Modern GUI toolkit for Linux
- **Cairo** - 2D graphics library for rendering
- **C** - Core programming language
- **Mathematical physics** - Realistic projectile motion and collision detection

### Key Components
- **Terrain Generation** - Multi-layered sine wave algorithm with smoothing
- **Physics Engine** - Gravity, wind resistance, and collision detection
- **Particle System** - Dynamic particle effects for explosions
- **Game State Management** - Turn-based logic with multiple game states
- **Graphics Rendering** - Hardware-accelerated 2D graphics with Cairo

## 🚀 Installation

### Prerequisites
- GTK4 development libraries
- Cairo development libraries
- GCC compiler
- pkg-config

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install libgtk-4-dev libcairo2-dev build-essential pkg-config
```

### Fedora/RHEL
```bash
sudo dnf install gtk4-devel cairo-devel gcc pkg-config
```

### Arch Linux
```bash
sudo pacman -S gtk4 cairo gcc pkg-config
```

### Compilation
```bash
# Clone the repository
git clone https://github.com/DeviceDidWhat/Artillery_Game.git
cd artillery-game

# Compile the game
gcc Artillery.c -o Artillery.exe `pkg-config --cflags --libs gtk4 cairo` -mconsole

# Run the game
./Artillery
```

## 🎲 How to Play

1. **Setup**: Each player starts with a tank on opposite sides of the terrain
2. **Aim**: Use arrow keys to adjust your cannon angle and firing power
3. **Choose Weapon**: Press W/S to cycle through different weapon types
4. **Position**: Use A/D to move your tank (limited moves per turn)
5. **Fire**: Press Space to launch your projectile
6. **Wind**: Pay attention to wind direction and strength - it affects trajectory!
7. **Victory**: Reduce opponent's health to 0 to win the round
8. **New Round**: Press R to start a new round with fresh terrain

## 🎨 Screenshots

*Add screenshots of your game here showing:*
- Main gameplay with terrain
- Different weapon explosions
- UI elements and controls
- Victory screen

## 🔧 Configuration

### Game Constants
You can modify these values in the source code to customize gameplay:

```c
#define WINDOW_WIDTH 1920      // Game window width
#define WINDOW_HEIGHT 1080     // Game window height
#define TERRAIN_SEGMENTS 800   // Terrain detail level
#define GRAVITY 0.1            // Physics gravity strength
#define MAX_POWER 100          // Maximum firing power
#define MAX_PARTICLES 200      // Particle system limit
```

### Weapon Properties
Each weapon can be customized by modifying the `init_weapons()` function:
- Damage values
- Explosion radius
- Terrain deformation
- Special abilities (drilling, clustering)

## 🐛 Known Issues

- Wind indicator may need manual refresh in some cases
- Performance may vary on older hardware with complex particle effects
- Game requires X11 display server (common on Linux)

## 🤝 Contributing

Contributions are welcome! Here are some ideas for improvements:

- **New Weapons**: Add more weapon types (laser, teleporter, etc.)
- **Sound Effects**: Add audio for explosions and firing
- **AI Player**: Implement computer opponent with difficulty levels
- **Network Play**: Add multiplayer support over network
- **Terrain Types**: Different biomes (desert, snow, moon, etc.)
- **Power-ups**: Collectible items that affect gameplay
- **Animations**: Smooth tank movement and better visual effects

### Development Setup
1. Fork the repository
2. Create a feature branch: `git checkout -b new-feature`
3. Make your changes and test thoroughly
4. Commit your changes: `git commit -am 'Add new feature'`
5. Push to the branch: `git push origin new-feature`
6. Submit a pull request


## 🙏 Acknowledgments

- Inspired by classic artillery games like Worms and Scorched Earth
- GTK4 and Cairo documentation and community
- Mathematical references for realistic physics simulation
- Open source gaming community for inspiration and feedback

## 📞 Support

If you encounter any issues or have questions:
- Open an issue on GitHub
- Review the source code comments for implementation details

---

**Enjoy the game!** 🎯💥

*Built with ❤️ for gaming community*
