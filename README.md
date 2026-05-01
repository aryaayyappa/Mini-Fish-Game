# 🐟 Hungry Fish - Terminal Arcade Game

Welcome to **Hungry Fish**, a high-speed, dynamic, and visually immersive arcade game built entirely in C using the `ncurses` terminal graphics library. 

Dive into the depths of the ocean, catch as much valuable sea life as you can, dodge devastating aquatic hazards, and try to survive the perilous Abyssal Trench!

---

## 🌊 Game Features

### 🎮 Fluid Mechanics
- **Queue-based Input:** Experience zero input lag and buttery smooth movement across the terminal.
- **Dash Mechanic:** Press `Spacebar` to perform a massive lateral dash to save combos or dodge sharks (includes a cooldown timer).
- **Pause System:** Need a break? Press `P` to freeze the action.

### 🌎 Dynamic Biomes & Difficulty Scaling
As your score increases, you level up and your basket dives deeper into the ocean, changing the environment:
1. **Shallow Reef (Levels 1–3):** Bright cyan waters, slow speeds, standard hazards.
2. **Deep Ocean (Levels 4–7):** Dark blue waters, faster speeds, introduction of Sea Mines.
3. **Abyssal Trench (Level 8+):** Pitch black. You can only see hazards when they enter your basket's small radius of light. Look out for the Sharks!

### 💎 The Loot Table
Catch these items in your basket to increase your score and build your Combo Multiplier:
- `<><` **Fish**: Standard points.
- `*` **Starfish**: Moderate points. Animates and rotates as it falls.
- `$$$` **Golden Fish**: Rare. Massive point injection.

### ✨ Power-Ups
- `[+]` **Wide Basket**: Expands your basket (`\_________/`) to catch more items easily.
- `[S]` **Shield**: Absorbs a single hit from a Bomb or Mine.
- `[~]` **Slow Motion**: Temporarily cuts the frame rate in half, giving you time to think.
- `[M]` **Magnet**: Pulls nearby fish and starfish directly into your basket.
- `[x2]` **Score Multiplier**: Doubles all incoming score (including combo scaling) for a limited time.

### ☠️ Obstacles & Hazards
- `O` **Bomb**: Resets your combo and removes 1 Life.
- `[X]` **Sea Mine**: A heavier explosive. Destroys shields instantly, removes 2 Lives, and penalizes you -500 points.
- `~@~` **Jellyfish**: Wiggles horizontally. Getting zapped paralyzes your basket for several seconds, leaving you helpless!
- `==///>` **Shark**: Spawns only in the depths. Sharks do not fall; they swim rapidly across the screen horizontally. Touching a shark results in an instant Game Over.

### 🎨 Visual Polish
- **Ambient Bubbles:** A constant stream of bubbles rising from the ocean floor.
- **Floating Pop-ups:** Points, combo resets, and power-up activations spawn animated text that floats upward and fades away.
- **Flashy UI:** Bold flashing animations for Level Ups and Game Over sequences.

---

## 🕹️ Controls

| Key | Action |
| :--- | :--- |
| **A / Left Arrow** | Move Basket Left |
| **D / Right Arrow** | Move Basket Right |
| **Spacebar** | Dash (Cooldown displayed in UI) |
| **P** | Pause / Unpause Game |
| **Q** | Quit Game instantly |

---

## 🛠️ Compilation & Execution

Because the game uses standard Unix/Linux terminal rendering, it requires the `ncurses` development library. 

### Prerequisites (macOS/Linux)
You likely already have this installed, but if not:
- **Ubuntu/Debian:** `sudo apt-get install libncurses5-dev libncursesw5-dev`
- **macOS:** Comes pre-installed with Xcode Command Line Tools.

### Compiling
Run the following command in your terminal to compile the source file:
```bash
gcc -o fish_game /Users/aryaayyappahalambi/Downloads/gemini-code-1777621178639.c -lncurses
```

### Running
```bash
./fish_game
```

---

*Enjoy the dive, and beware the Sharks!*
