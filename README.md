# Chess Game

A fully-featured chess game implementation using SDL2 with graphical interface, timed gameplay, and complete chess rule validation.

## Features

* **Complete Chess Implementation**: All standard chess pieces with proper movement validation
* **Special Moves**: Castling, pawn promotion, en passant (partial)
* **Game Logic**: Check, checkmate, and stalemate detection
* **Timed Gameplay**: 10-minute timer for each player
* **Visual Interface**:
    * Highlighted squares for selected pieces and valid moves
    * Coordinate labels (a-h, 1-8)
    * Visual check indicators
    * Move history highlighting
* **Multiple Screens**: Home screen, game screen, and pause menu
* **Game Controls**: Pause/resume functionality and restart option

## Requirements

* SDL2 development libraries
* SDL2\_image (PNG support)
* SDL2\_ttf (font rendering)
* Windows OS (uses Windows-specific file paths)
* Visual C++ compiler or compatible

### Required SDL2 Libraries

* SDL2.lib
* SDL2main.lib
* SDL2\_image.lib
* SDL2\_ttf.lib

## Installation

1. **Install SDL2 Development Libraries**
    * Download SDL2, SDL2\_image, and SDL2\_ttf development libraries
    * Extract to a folder (e.g., `D:\SDL2`)
2. **Set up Development Environment**
    * Configure include paths to SDL2 headers
    * Configure library paths to SDL2 libraries
    * Link against required libraries
3. **Prepare Assets**

    ```
    chess/
    ├── chess.exe
    └── assets/
        ├── wp.png (white pawn)
        ├── wr.png (white rook)
        ├── wn.png (white knight)
        ├── wb.png (white bishop)
        ├── wq.png (white queen)
        ├── wk.png (white king)
        ├── bp.png (black pawn)
        ├── br.png (black rook)
        ├── bn.png (black knight)
        ├── bb.png (black bishop)
        ├── bq.png (black queen)
        ├── bk.png (black king)
        └── home_bg.png (background image)
    ```

## Compilation

```bash
gcc main.c -o chess.exe -ISDL/include/SDL2 -ISDL_image/include/SDL2 -ISDL_ttf/include/SDL2 -LSDL/lib -LSDL_image/lib -LSDL_ttf/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf
```

Run the command from the repository root. The executable is generated locally;
keep the bundled DLLs and assets beside it when running.

Or using Visual Studio, create a new project and:

1. Add the source file
2. Configure SDL2 library paths
3. Link against SDL2, SDL2\_image, and SDL2\_ttf libraries

## Usage

### Starting the Game

1. Run the executable
2. Click "Play" from the home screen to start a new game
3. Click "Exit" to quit the application

### Playing Chess

* **Select Piece**: Click on any piece to select it
* **Make Move**: Click on a highlighted destination square
* **Valid Moves**: Green dots show available moves, rings show capture moves
* **Turn System**: White (Player One) moves first, then alternates

### Game Controls

* **P Key**: Pause/unpause the game
* **ESC Key**: Open pause menu during game
* **Mouse**: All game interactions are mouse-driven

### Game States

* **Check**: King square highlights red when in check
* **Checkmate**: Game ends with winner announcement
* **Stalemate**: Game ends in draw
* **Time Out**: Game ends when a player runs out of time

### Special Chess Rules Implemented

* **Castling**: King and rook move simultaneously (kingside/queenside)
* **Pawn Promotion**: Pawns reaching end ranks become queens
* **Check Validation**: Illegal moves that leave king in check are prevented

## Game Configuration

### Customizable Constants

```c
#define TILE 90           // Size of each board square in pixels
#define BOARD_SIZE 8      // Standard 8x8 chess board
int playerTimers[2] = {600, 600}; // 10 minutes per player in seconds
```

### Piece Encoding

The game uses numeric codes for pieces:

* 0: Empty square
* 1-6: White pieces (pawn, rook, knight, bishop, queen, king)
* 7-12: Black pieces (pawn, rook, knight, bishop, queen, king)

## File Structure

* **Main Loop**: SDL event handling and game state management
* **Chess Logic**: Move validation, check detection, game end conditions
* **Rendering**: Board drawing, piece rendering, UI elements
* **Asset Loading**: Dynamic loading of piece images and fonts

## Known Limitations

* En passant capture not fully implemented
* Font path hardcoded to Windows system fonts
* Asset paths use Windows-style backslashes
* No AI opponent (human vs human only)
* No move notation or game saving

## Troubleshooting

**Missing DLLs**: Ensure SDL2.dll, SDL2\_image.dll, and SDL2\_ttf.dll are in the executable directory or system PATH.

**Missing Assets**: The game will run without piece images but pieces won't be visible. Ensure all PNG files are in the `assets` folder.

**Font Loading Issues**: The game attempts to load Arial from Windows fonts directory. On non-Windows systems, modify the font path in the code.

## License

This is a educational/demonstration chess implementation. Ensure you have proper licenses for SDL2 libraries if distributing.
