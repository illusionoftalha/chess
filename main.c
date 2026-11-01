#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#define TILE 90
#define BOARD_SIZE 8
#define MAX_MOVES 200
char moveHistory[MAX_MOVES][16];  
int moveCount = 0;
SDL_Texture* homeBackground = NULL;
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* tex[12];
void drawButton(const char* text, int x, int y, int w, int h);
void drawCoordinates(int winW, int winH);
int board[BOARD_SIZE][BOARD_SIZE];
int turn = 0;
bool isKingInCheck(int side);
bool isCheckmate(int side);
bool hasLegalMoves(int side);
bool isSquareAttacked(int x, int y, int byBlack);
bool running = true;
bool paused = false;
bool gameOver = false;
int winner = -1;

int selX = -1, selY = -1;
int lastMoveSX = -1, lastMoveSY = -1, lastMoveDX = -1, lastMoveDY = -1;
int boardOffX = 0, boardOffY = 0;
bool whiteKingMoved = false, blackKingMoved = false;
bool whiteRookLeftMoved = false, whiteRookRightMoved = false;
bool blackRookLeftMoved = false, blackRookRightMoved = false;

TTF_Font* font = NULL;
TTF_Font* gameOverFont = NULL;

int playerTimers[2] = {600, 600};
Uint32 lastTick = 0;
typedef enum { SCREEN_HOME, SCREEN_GAME, SCREEN_PAUSE } Screen;
Screen currentScreen = SCREEN_HOME;

SDL_Texture* loadTexture(const char* path) {
   SDL_Surface* s = IMG_Load(path);
   if (!s) {
      printf("IMG load error: %s -> %s\n", path, IMG_GetError());
      return NULL;
   }
   SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, s);
   SDL_FreeSurface(s);
   return t;
}
const char* pieceToChar(int p) {
   switch (p) {
      case 1:
         return "";  

      case 2:
         return "R";
      case 3:
         return "N";
      case 4:
         return "B";
      case 5:
         return "Q";
      case 6:
         return "K";
      case 7:
         return "";  

      case 8:
         return "R";
      case 9:
         return "N";
      case 10:
         return "B";
      case 11:
         return "Q";
      case 12:
         return "K";
   }
   return "";
}

char fileChar(int x) { return 'a' + x; }
char rankChar(int y) { return '8' - y; }

void addMoveToHistory(int sx, int sy, int dx, int dy) {
   int movingPiece = board[sy][sx];
   int capturedPiece = board[dy][dx];
   char notation[16] = "";

   if ((movingPiece == 6 || movingPiece == 12) && abs(dx - sx) == 2) {
      strcpy(notation, dx - sx == 2 ? "O-O" : "O-O-O");
   } else {
      const char* pc = pieceToChar(movingPiece);

      if (pc[0] == '\0') {  

         if (capturedPiece != 0)
            snprintf(notation, sizeof(notation), "%cx%c%c", fileChar(sx),
                     fileChar(dx), rankChar(dy));
         else
            snprintf(notation, sizeof(notation), "%c%c", fileChar(dx),
                     rankChar(dy));
      } else {  

         if (capturedPiece != 0)
            snprintf(notation, sizeof(notation), "%sx%c%c", pc, fileChar(dx),
                     rankChar(dy));
         else
            snprintf(notation, sizeof(notation), "%s%c%c", pc, fileChar(dx),
                     rankChar(dy));
      }
   }

   int tmpSrc = board[sy][sx];
   int tmpDest = board[dy][dx];

   board[dy][dx] = board[sy][sx];
   board[sy][sx] = 0;

   int opponent = (turn == 0) ? 1 : 0;

   int originalTurn = turn;
   turn = opponent;

   bool kingInCheck = isKingInCheck(opponent);
   bool mate = kingInCheck && !hasLegalMoves(opponent);

   turn = originalTurn;

   if (mate)
      strcat(notation, "#");
   else if (kingInCheck)
      strcat(notation, "+");

   board[sy][sx] = tmpSrc;
   board[dy][dx] = tmpDest;

   if (moveCount < MAX_MOVES) {
      strcpy(moveHistory[moveCount++], notation);
   }
}

void getExePath(char* buffer, int size) {
   GetModuleFileName(NULL, buffer, size);
   for (int i = (int)strlen(buffer) - 1; i >= 0; i--) {
      if (buffer[i] == '\\' || buffer[i] == '/') {
         buffer[i] = '\0';
         break;
      }
   }
}

void loadPieces() {
   char basePath[512];
   getExePath(basePath, sizeof(basePath));
   const char* files[12] = {"wp.png", "wr.png", "wn.png", "wb.png",
                            "wq.png", "wk.png", "bp.png", "br.png",
                            "bn.png", "bb.png", "bq.png", "bk.png"};
   for (int i = 0; i < 12; i++) {
      char full[512];
      snprintf(full, sizeof(full), "%s\\assets\\%s", basePath, files[i]);
      tex[i] = loadTexture(full);
      if (!tex[i]) printf("Failed to load: %s\n", full);
   }

   char homePath[512];
   getExePath(homePath, sizeof(homePath));
   snprintf(homePath, sizeof(homePath), "%s\\assets\\home_bg.png", homePath);
   homeBackground = loadTexture(homePath);
   if (!homeBackground) printf("Failed to load home background\n");
}

void initBoard() {
   int tmp[BOARD_SIZE][BOARD_SIZE] = {
       {8, 9, 10, 11, 12, 10, 9, 8}, {7, 7, 7, 7, 7, 7, 7, 7},
       {0, 0, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 0, 0},
       {0, 0, 0, 0, 0, 0, 0, 0},     {0, 0, 0, 0, 0, 0, 0, 0},
       {1, 1, 1, 1, 1, 1, 1, 1},     {2, 3, 4, 5, 6, 4, 3, 2}};
   for (int y = 0; y < BOARD_SIZE; y++)
      for (int x = 0; x < BOARD_SIZE; x++) board[y][x] = tmp[y][x];
}

bool isWhite(int p) { return p >= 1 && p <= 6; }
bool isBlack(int p) { return p >= 7; }
bool inside(int x, int y) {
   return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
}
bool isPathClear(int sx, int sy, int dx, int dy) {
   int stepX = (dx > sx) ? 1 : (dx < sx ? -1 : 0);
   int stepY = (dy > sy) ? 1 : (dy < sy ? -1 : 0);

   int x = sx + stepX;
   int y = sy + stepY;

   while (x != dx || y != dy) {
      if (board[y][x] != 0) return false;
      x += stepX;
      y += stepY;
   }
   return true;
}

bool validMove(int sx, int sy, int dx, int dy) {
   if (!inside(dx, dy)) return false;

   int p = board[sy][sx];
   int target = board[dy][dx];

   if (p == 0) return false;
   if (turn == 0 && !isWhite(p)) return false;
   if (turn == 1 && !isBlack(p)) return false;

   if (target != 0) {
      if (isWhite(p) && isWhite(target)) return false;
      if (isBlack(p) && isBlack(target)) return false;
   }

   int dxv = dx - sx;
   int dyv = dy - sy;

   if (p == 1) {
      if (dxv == 0 && dyv == -1 && target == 0) return true;
      if (dxv == 0 && dyv == -2 && sy == 6 && board[5][sx] == 0 && target == 0)
         return true;
      if (abs(dxv) == 1 && dyv == -1 && target != 0) return true;
   }

   if (p == 7) {
      if (dxv == 0 && dyv == 1 && target == 0) return true;
      if (dxv == 0 && dyv == 2 && sy == 1 && board[2][sx] == 0 && target == 0)
         return true;
      if (abs(dxv) == 1 && dyv == 1 && target != 0) return true;
   }

   if (p == 2 || p == 8) {
      if (dx == sx || dy == sy) return isPathClear(sx, sy, dx, dy);
   }

   if (p == 3 || p == 9) {
      if ((abs(dxv) == 2 && abs(dyv) == 1) || (abs(dxv) == 1 && abs(dyv) == 2))
         return true;
   }

   if (p == 4 || p == 10) {
      if (abs(dxv) == abs(dyv)) return isPathClear(sx, sy, dx, dy);
   }

   if (p == 5 || p == 11) {
      if (dx == sx || dy == sy || abs(dxv) == abs(dyv))
         return isPathClear(sx, sy, dx, dy);
   }

   if (p == 6 || p == 12) {
      if (abs(dxv) <= 1 && abs(dyv) <= 1) return true;
   }

   if (p == 6 && sy == 7 && dy == 7) {
      if (dx - sx == 2 && !whiteKingMoved && !whiteRookRightMoved &&
          board[7][5] == 0 && board[7][6] == 0 && !isSquareAttacked(4, 7, 1) &&
          !isSquareAttacked(5, 7, 1) && !isSquareAttacked(6, 7, 1))
         return true;

      if (dx - sx == -2 && !whiteKingMoved && !whiteRookLeftMoved &&
          board[7][1] == 0 && board[7][2] == 0 && board[7][3] == 0 &&
          !isSquareAttacked(4, 7, 1) && !isSquareAttacked(3, 7, 1) &&
          !isSquareAttacked(2, 7, 1))
         return true;
   }

   if (p == 12 && sy == 0 && dy == 0) {
      if (dx - sx == 2 && !blackKingMoved && !blackRookRightMoved &&
          board[0][5] == 0 && board[0][6] == 0 && !isSquareAttacked(4, 0, 0) &&
          !isSquareAttacked(5, 0, 0) && !isSquareAttacked(6, 0, 0))
         return true;

      if (dx - sx == -2 && !blackKingMoved && !blackRookLeftMoved &&
          board[0][1] == 0 && board[0][2] == 0 && board[0][3] == 0 &&
          !isSquareAttacked(4, 0, 0) && !isSquareAttacked(3, 0, 0) &&
          !isSquareAttacked(2, 0, 0))
         return true;
   }
   return false;
}

void movePiece(int sx, int sy, int dx, int dy) {
   int p = board[sy][sx];
   if (p == 6) whiteKingMoved = true;
   if (p == 12) blackKingMoved = true;
   if (p == 2 && sy == 7 && sx == 0) whiteRookLeftMoved = true;
   if (p == 2 && sy == 7 && sx == 7) whiteRookRightMoved = true;
   if (p == 8 && sy == 0 && sx == 0) blackRookLeftMoved = true;
   if (p == 8 && sy == 0 && sx == 7) blackRookRightMoved = true;

   if (p == 6 && dx - sx == 2) {
      board[7][6] = 6;
      board[7][5] = 2;
      board[7][4] = 0;
      board[7][7] = 0;
      return;
   }
   if (p == 6 && dx - sx == -2) {
      board[7][2] = 6;
      board[7][3] = 2;
      board[7][4] = 0;
      board[7][0] = 0;
      return;
   }
   if (p == 12 && dx - sx == 2) {
      board[0][6] = 12;
      board[0][5] = 8;
      board[0][4] = 0;
      board[0][7] = 0;
      return;
   }
   if (p == 12 && dx - sx == -2) {
      board[0][2] = 12;
      board[0][3] = 8;
      board[0][4] = 0;
      board[0][0] = 0;
      return;
   }

   board[dy][dx] = board[sy][sx];
   board[sy][sx] = 0;

   if (p == 1 && dy == 0) board[dy][dx] = 5;
   if (p == 7 && dy == BOARD_SIZE - 1) board[dy][dx] = 11;
}
bool canAttack(int sx, int sy, int dx, int dy) {
   if (!inside(dx, dy)) return false;

   int p = board[sy][sx];
   int target = board[dy][dx];

   if (p == 0) return false;

   int dxv = dx - sx;
   int dyv = dy - sy;

   if (p == 1) {
      if (abs(dxv) == 1 && dyv == -1) return true;
   }
   if (p == 7) {
      if (abs(dxv) == 1 && dyv == 1) return true;
   }

   if (p == 2 || p == 8) {
      if (dx == sx || dy == sy) return isPathClear(sx, sy, dx, dy);
   }

   if (p == 3 || p == 9) {
      if ((abs(dxv) == 2 && abs(dyv) == 1) || (abs(dxv) == 1 && abs(dyv) == 2))
         return true;
   }

   if (p == 4 || p == 10) {
      if (abs(dxv) == abs(dyv)) return isPathClear(sx, sy, dx, dy);
   }

   if (p == 5 || p == 11) {
      if (dx == sx || dy == sy || abs(dxv) == abs(dyv))
         return isPathClear(sx, sy, dx, dy);
   }

   if (p == 6 || p == 12) {
      if (abs(dxv) <= 1 && abs(dyv) <= 1) return true;
   }

   return false;
}
bool isSquareAttacked(int x, int y, int byBlack) {
   for (int iy = 0; iy < BOARD_SIZE; iy++) {
      for (int ix = 0; ix < BOARD_SIZE; ix++) {
         int p = board[iy][ix];
         if (p == 0) continue;

         if (byBlack && isBlack(p)) {
            if (canAttack(ix, iy, x, y)) return true;
         }
         if (!byBlack && isWhite(p)) {
            if (canAttack(ix, iy, x, y)) return true;
         }
      }
   }
   return false;
}

bool wouldCauseCheck(int sx, int sy, int dx, int dy) {
   int tmp = board[dy][dx];
   board[dy][dx] = board[sy][sx];
   board[sy][sx] = 0;
   int kingX = -1, kingY = -1;
   for (int y = 0; y < BOARD_SIZE; y++) {
      for (int x = 0; x < BOARD_SIZE; x++) {
         if ((turn == 0 && board[y][x] == 6) ||
             (turn == 1 && board[y][x] == 12)) {
            kingX = x;
            kingY = y;
            break;
         }
      }
   }
   bool inCheck = isSquareAttacked(kingX, kingY, turn == 0 ? 1 : 0);
   board[sy][sx] = board[dy][dx];
   board[dy][dx] = tmp;
   return inCheck;
}

bool hasLegalMoves(int side) {
   for (int y = 0; y < BOARD_SIZE; y++) {
      for (int x = 0; x < BOARD_SIZE; x++) {
         if ((side == 0 && isWhite(board[y][x])) ||
             (side == 1 && isBlack(board[y][x]))) {
            for (int yy = 0; yy < BOARD_SIZE; yy++) {
               for (int xx = 0; xx < BOARD_SIZE; xx++) {
                  if (validMove(x, y, xx, yy) && !wouldCauseCheck(x, y, xx, yy))
                     return true;
               }
            }
         }
      }
   }
   return false;
}
void findKing(int side, int* kx, int* ky) {
   int king = (side == 0) ? 6 : 12;
   for (int y = 0; y < BOARD_SIZE; y++)
      for (int x = 0; x < BOARD_SIZE; x++)
         if (board[y][x] == king) {
            *kx = x;
            *ky = y;
            return;
         }
}

bool isKingInCheck(int side) {
   int kx = -1, ky = -1;
   findKing(side, &kx, &ky);
   return isSquareAttacked(kx, ky, side == 0 ? 1 : 0);
}

bool isCheckmate(int side) {
   if (!isKingInCheck(side)) return false;
   if (!hasLegalMoves(side)) return true;
   return false;
}

bool isStalemate(int side) {
   if (isKingInCheck(side)) return false;
   if (!hasLegalMoves(side)) return true;
   return false;
}
void drawMoveHistory(int winW, int winH) {
   if (!font) return;
   SDL_Color white = {255, 255, 255, 255};
   SDL_Surface* surf;
   SDL_Texture* t;
   SDL_Rect r;

   int xStart = boardOffX + TILE * BOARD_SIZE + 20;  

   int yStart = boardOffY;
   int lineSpacing = 8;  

   int moveNum = 1;

   for (int i = 0; i < moveCount; i += 2) {
      char line[64];
      if (i + 1 < moveCount)
         snprintf(line, sizeof(line), "%d. %s   %s", moveNum, moveHistory[i],
                  moveHistory[i + 1]);
      else
         snprintf(line, sizeof(line), "%d. %s", moveNum, moveHistory[i]);

      surf = TTF_RenderText_Solid(font, line, white);
      t = SDL_CreateTextureFromSurface(renderer, surf);

      r.x = xStart;
      r.y = yStart + (i / 2) * (surf->h + 8);  

      r.w = surf->w;
      r.h = surf->h;

      SDL_RenderCopy(renderer, t, NULL, &r);
      SDL_FreeSurface(surf);
      SDL_DestroyTexture(t);

      moveNum++;
   }
}
void resetMoveHistory() {
   moveCount = 0;
   for (int i = 0; i < MAX_MOVES; i++) {
      moveHistory[i][0] = '\0';
   }
}
void click(int mx, int my, int winW, int winH) {
   if (gameOver || paused) return;

   int offX = boardOffX;
   int offY = boardOffY;

   int x = (mx - offX) / TILE;
   int y = (my - offY) / TILE;

   if (!inside(x, y)) return;

   if (selX == -1) {
      if (board[y][x] != 0) {
         selX = x;
         selY = y;
      }
   } else {
      if (validMove(selX, selY, x, y) && !wouldCauseCheck(selX, selY, x, y)) {
         addMoveToHistory(selX, selY, x, y);
         movePiece(selX, selY, x, y);

         lastMoveSX = selX;
         lastMoveSY = selY;
         lastMoveDX = x;
         lastMoveDY = y;
         selX = selY = -1;

         turn = !turn;
         lastTick = SDL_GetTicks();

         if (isKingInCheck(turn)) printf("Check!\n");
         if (isCheckmate(turn)) {
            printf("Checkmate!\n");
            gameOver = true;
            winner = !turn;
         }
         if (isStalemate(turn)) {
            printf("Stalemate!\n");
            gameOver = true;
            winner = -1;
         }
      } else {
         selX = selY = -1;
      }
   }
}

void drawBoard(int winW, int winH) {
   boardOffX = (winW - TILE * BOARD_SIZE) / 2;
   boardOffY = (winH - TILE * BOARD_SIZE) / 2;

   for (int y = 0; y < BOARD_SIZE; y++) {
      for (int x = 0; x < BOARD_SIZE; x++) {
         SDL_Rect r = {boardOffX + x * TILE, boardOffY + y * TILE, TILE, TILE};

         if ((x + y) % 2 == 0)
            SDL_SetRenderDrawColor(renderer, 240, 217, 181, 255);
         else
            SDL_SetRenderDrawColor(renderer, 181, 136, 99, 255);
         SDL_RenderFillRect(renderer, &r);

         if (selX == x && selY == y) {
            SDL_SetRenderDrawColor(renderer, 209, 137, 48, 150);
            SDL_RenderFillRect(renderer, &r);
         }

         if (x == lastMoveDX && y == lastMoveDY) {
            SDL_SetRenderDrawColor(renderer, 199, 149, 86, 150);
            SDL_RenderFillRect(renderer, &r);
         }

         int kx = -1, ky = -1;
         if (isKingInCheck(0)) {
            findKing(0, &kx, &ky);
            if (x == kx && y == ky) {
               SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150);
               SDL_RenderFillRect(renderer, &r);
            }
         }
         if (isKingInCheck(1)) {
            findKing(1, &kx, &ky);
            if (x == kx && y == ky) {
               SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150);
               SDL_RenderFillRect(renderer, &r);
            }
         }
      }
   }

   if (selX != -1 && selY != -1) {
      for (int yy = 0; yy < BOARD_SIZE; yy++) {
         for (int xx = 0; xx < BOARD_SIZE; xx++) {
            if ((xx != selX || yy != selY) && validMove(selX, selY, xx, yy) &&
                !wouldCauseCheck(selX, selY, xx, yy)) {
               int centerX = boardOffX + xx * TILE + TILE / 2;
               int centerY = boardOffY + yy * TILE + TILE / 2;
               int radius = TILE / 6;

               if (board[yy][xx] == 0) {
                  SDL_SetRenderDrawColor(renderer, 128, 128, 128, 100);
               } else {
                  SDL_SetRenderDrawColor(renderer, 128, 128, 128, 180);
                  int outerR = TILE / 2 - 4;
                  int thickness = 4;
                  for (int w = -outerR; w <= outerR; w++) {
                     for (int h = -outerR; h <= outerR; h++) {
                        int distSq = w * w + h * h;
                        if (distSq >=
                                (outerR - thickness) * (outerR - thickness) &&
                            distSq <= outerR * outerR) {
                           SDL_RenderDrawPoint(renderer, centerX + w,
                                               centerY + h);
                        }
                     }
                  }
                  continue;
               }

               for (int w = -radius; w <= radius; w++) {
                  for (int h = -radius; h <= radius; h++) {
                     if (w * w + h * h <= radius * radius) {
                        SDL_RenderDrawPoint(renderer, centerX + w, centerY + h);
                     }
                  }
               }
            }
         }
      }
   }

   for (int y = 0; y < BOARD_SIZE; y++) {
      for (int x = 0; x < BOARD_SIZE; x++) {
         int p = board[y][x];
         if (p != 0 && tex[p - 1]) {
            int pad = TILE / 8;
            SDL_Rect pr = {boardOffX + x * TILE + pad,
                           boardOffY + y * TILE + pad, TILE - 2 * pad,
                           TILE - 2 * pad};
            SDL_RenderCopy(renderer, tex[p - 1], NULL, &pr);
         }
      }
   }

   drawCoordinates(winW, winH);
}
void drawUI(int winW, int winH) {
   if (!font) return;
   SDL_Color white = {255, 255, 255, 255};
   char text[64];

   if (!paused && !gameOver && lastTick > 0) {
      Uint32 now = SDL_GetTicks();
      int delta = (now - lastTick) / 1000;
      if (delta > 0) {
         playerTimers[turn] -= delta;
         if (playerTimers[turn] <= 0) {
            playerTimers[turn] = 0;
            gameOver = true;
            winner = !turn;
         }
         lastTick = now;
      }
   }

   SDL_Surface* surf;
   SDL_Texture* t;
   SDL_Rect r;

   int margin = 10;

   snprintf(text, sizeof(text), "Player Two");
   surf = TTF_RenderText_Solid(font, text, white);
   t = SDL_CreateTextureFromSurface(renderer, surf);
   r.x = boardOffX;

   r.y = boardOffY - surf->h - margin;

   r.w = surf->w;
   r.h = surf->h;
   SDL_RenderCopy(renderer, t, NULL, &r);
   SDL_FreeSurface(surf);
   SDL_DestroyTexture(t);

   int rem = playerTimers[1];
   int m = rem / 60;
   int s = rem % 60;
   snprintf(text, sizeof(text), "%02d:%02d", m, s);
   surf = TTF_RenderText_Solid(font, text, white);
   t = SDL_CreateTextureFromSurface(renderer, surf);
   r.x = boardOffX + TILE * BOARD_SIZE - surf->w;

   r.y = boardOffY - surf->h - margin;
   r.w = surf->w;
   r.h = surf->h;
   SDL_RenderCopy(renderer, t, NULL, &r);
   SDL_FreeSurface(surf);
   SDL_DestroyTexture(t);

   snprintf(text, sizeof(text), "Player One");
   surf = TTF_RenderText_Solid(font, text, white);
   t = SDL_CreateTextureFromSurface(renderer, surf);
   r.x = boardOffX;

   r.y = boardOffY + TILE * BOARD_SIZE + margin;
   r.w = surf->w;
   r.h = surf->h;
   SDL_RenderCopy(renderer, t, NULL, &r);
   SDL_FreeSurface(surf);
   SDL_DestroyTexture(t);

   rem = playerTimers[0];
   m = rem / 60;
   s = rem % 60;
   snprintf(text, sizeof(text), "%02d:%02d", m, s);
   surf = TTF_RenderText_Solid(font, text, white);
   t = SDL_CreateTextureFromSurface(renderer, surf);
   r.x = boardOffX + TILE * BOARD_SIZE - surf->w;

   r.y = boardOffY + TILE * BOARD_SIZE + margin;
   r.w = surf->w;
   r.h = surf->h;
   SDL_RenderCopy(renderer, t, NULL, &r);
   SDL_FreeSurface(surf);
   SDL_DestroyTexture(t);

   if (gameOver) {
      char endText[64];
      if (winner == -1)
         snprintf(endText, sizeof(endText), "Draw (Stalemate)");
      else
         snprintf(endText, sizeof(endText), "Player %s Wins!",
                  winner == 0 ? "One" : "Two");

      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
      SDL_Rect bg = {winW / 2 - 200, winH / 2 - 60, 400, 80};
      SDL_RenderFillRect(renderer, &bg);

      TTF_Font* bigFont = gameOverFont ? gameOverFont : font;

      surf =
          TTF_RenderText_Solid(bigFont, endText, (SDL_Color){255, 215, 0, 255});
      t = SDL_CreateTextureFromSurface(renderer, surf);
      r.x = winW / 2 - surf->w / 2;
      r.y = winH / 2 - surf->h / 2;
      r.w = surf->w;
      r.h = surf->h;
      SDL_RenderCopy(renderer, t, NULL, &r);
      SDL_FreeSurface(surf);
      SDL_DestroyTexture(t);


      drawButton("Restart", winW / 2 - 100, winH / 2 + 40, 200, 50);
   }
}
void drawCoordinates(int winW, int winH) {
   if (!font) return;
   SDL_Color black = {0, 0, 0, 255};

   int offX = (winW - TILE * BOARD_SIZE) / 2;
   int offY = (winH - TILE * BOARD_SIZE) / 2;

   char text[2];
   SDL_Surface* surf;
   SDL_Texture* t;
   SDL_Rect r;

   for (int x = 0; x < BOARD_SIZE; x++) {
      text[0] = 'a' + x;
      text[1] = '\0';
      surf = TTF_RenderText_Solid(font, text, black);
      t = SDL_CreateTextureFromSurface(renderer, surf);

      r.w = surf->w;
      r.h = surf->h;
      r.x = offX + x * TILE + TILE - surf->w - 2;

      r.y = offY + (BOARD_SIZE - 1) * TILE + TILE - surf->h - 2;

      SDL_RenderCopy(renderer, t, NULL, &r);
      SDL_FreeSurface(surf);
      SDL_DestroyTexture(t);
   }

   for (int y = 0; y < BOARD_SIZE; y++) {
      text[0] = '8' - y;
      text[1] = '\0';
      surf = TTF_RenderText_Solid(font, text, black);
      t = SDL_CreateTextureFromSurface(renderer, surf);

      r.w = surf->w;
      r.h = surf->h;
      r.x = offX + 2;

      r.y = offY + y * TILE + 2;

      SDL_RenderCopy(renderer, t, NULL, &r);
      SDL_FreeSurface(surf);
      SDL_DestroyTexture(t);
   }
}
bool mouseOver(int mx, int my, SDL_Rect r) {
   return mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h;
}

void drawButton(const char* text, int x, int y, int w, int h) {
   SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
   SDL_Rect r = {x, y, w, h};
   SDL_RenderFillRect(renderer, &r);
   SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
   SDL_RenderDrawRect(renderer, &r);

   if (!font) return;
   SDL_Surface* surf =
       TTF_RenderText_Solid(font, text, (SDL_Color){255, 255, 255, 255});
   SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, surf);
   SDL_Rect tr = {x + w / 2 - surf->w / 2, y + h / 2 - surf->h / 2, surf->w,
                  surf->h};
   SDL_RenderCopy(renderer, t, NULL, &tr);
   SDL_FreeSurface(surf);
   SDL_DestroyTexture(t);
}
void drawHomeScreen(int winW, int winH) {
   SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
   SDL_RenderClear(renderer);

   if (homeBackground) {
      int imgW, imgH;
      SDL_QueryTexture(homeBackground, NULL, NULL, &imgW, &imgH);

      SDL_Rect dest;
      dest.w = imgW;
      dest.h = imgH;

      
      dest.x = (winW - imgW) / 2;
      dest.y = (winH - imgH) / 2;

      SDL_RenderCopy(renderer, homeBackground, NULL, &dest);
   }

   drawButton("Play", winW / 2 - 100, winH / 2 - 60, 200, 50);
   drawButton("Exit", winW / 2 - 100, winH / 2 + 10, 200, 50);
}
void drawPauseMenu(int winW, int winH) {
   SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
   SDL_Rect r = {0, 0, winW, winH};
   SDL_RenderFillRect(renderer, &r);
   drawButton("Resume", winW / 2 - 100, winH / 2 - 60, 200, 50);
   drawButton("Home", winW / 2 - 100, winH / 2 + 10, 200, 50);
}

int main(int argc, char* argv[]) {
   if (TTF_Init() < 0) {
      printf("TTF_Init error: %s\n", TTF_GetError());
      return 1;
   }
   font = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 20);
   if (!font) {
      printf("Font load failed: %s\n", TTF_GetError());
      return 1;
   }

   // Reuse the larger font instead of opening and closing it every end-screen frame.
   gameOverFont = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 48);

   srand(time(NULL));
   if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      printf("SDL error: %s\n", SDL_GetError());
      return 1;
   }
   if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
      printf("IMG error: %s\n", IMG_GetError());
      return 1;
   }

   window = SDL_CreateWindow("Chess", SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED, TILE * BOARD_SIZE + 100,
                             TILE * BOARD_SIZE + 150, SDL_WINDOW_RESIZABLE);
   if (!window) {
      printf("Window error: %s\n", SDL_GetError());
      return 1;
   }
   renderer = SDL_CreateRenderer(
       window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
   if (!renderer) {
      printf("Renderer error: %s\n", SDL_GetError());
      return 1;
   }
   SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

   loadPieces();
   initBoard();
   lastTick = SDL_GetTicks();

   SDL_Event e;
   int winW = TILE * BOARD_SIZE + 100, winH = TILE * BOARD_SIZE + 150;

   while (running) {
      while (SDL_PollEvent(&e)) {
         if (e.type == SDL_QUIT) running = false;
         if (e.type == SDL_WINDOWEVENT &&
             e.window.event == SDL_WINDOWEVENT_RESIZED) {
            winW = e.window.data1;
            winH = e.window.data2;
         }

         int mx, my;
         if (e.type == SDL_MOUSEBUTTONDOWN) {
            mx = e.button.x;
            my = e.button.y;

            if (currentScreen == SCREEN_HOME) {
               SDL_Rect play = {winW / 2 - 100, winH / 2 - 60, 200, 50};
               SDL_Rect exit = {winW / 2 - 100, winH / 2 + 10, 200, 50};
               if (mouseOver(mx, my, play)) {
                  currentScreen = SCREEN_GAME;
                  paused = false;
                  lastTick = SDL_GetTicks();
               }
               if (mouseOver(mx, my, exit)) running = false;
            } else if (currentScreen == SCREEN_PAUSE) {
               SDL_Rect resume = {winW / 2 - 100, winH / 2 - 60, 200, 50};
               SDL_Rect home = {winW / 2 - 100, winH / 2 + 10, 200, 50};
               if (mouseOver(mx, my, resume)) {
                  currentScreen = SCREEN_GAME;
                  paused = false;
                  lastTick = SDL_GetTicks();
               }
               if (mouseOver(mx, my, home)) {
                  currentScreen = SCREEN_HOME;
                  initBoard();
                  turn = 0;
                  gameOver = false;
                  playerTimers[0] = playerTimers[1] = 600;
                  selX = selY = -1;
                  resetMoveHistory();  

               }
            } else if (currentScreen == SCREEN_GAME && !paused) {
               click(mx, my, winW, winH);
               if (gameOver) {
                  SDL_Rect restart = {winW / 2 - 100, winH / 2 + 20, 200, 50};
                  if (mouseOver(mx, my, restart)) {
                     initBoard();
                     turn = 0;
                     gameOver = false;
                     winner = -1;
                     playerTimers[0] = playerTimers[1] = 600;
                     selX = selY = -1;
                     lastMoveSX = lastMoveSY = lastMoveDX = lastMoveDY = -1;
                     whiteKingMoved = blackKingMoved = false;
                     whiteRookLeftMoved = whiteRookRightMoved = false;
                     blackRookLeftMoved = blackRookRightMoved = false;
                     lastTick = SDL_GetTicks();
                     resetMoveHistory();
                  }
               }
            }
         }

         if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_p) {
               paused = !paused;
               if (!paused) lastTick = SDL_GetTicks();
            }
            if (e.key.keysym.sym == SDLK_ESCAPE) {
               if (currentScreen == SCREEN_GAME) {
                  currentScreen = SCREEN_PAUSE;
                  paused = true;
               } else if (currentScreen == SCREEN_PAUSE) {
                  currentScreen = SCREEN_GAME;
                  paused = false;
                  lastTick = SDL_GetTicks();
               }
            }
         }
      }

      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      SDL_RenderClear(renderer);

      switch (currentScreen) {
         case SCREEN_HOME:
            drawHomeScreen(winW, winH);
            break;
         case SCREEN_GAME:
            drawBoard(winW, winH);
            drawUI(winW, winH);
            drawMoveHistory(winW, winH);  

            break;
         case SCREEN_PAUSE:
            drawBoard(winW, winH);
            drawUI(winW, winH);
            drawPauseMenu(winW, winH);
            break;
      }

      SDL_RenderPresent(renderer);
   }

   if (gameOverFont) TTF_CloseFont(gameOverFont);
   if (font) TTF_CloseFont(font);
   TTF_Quit();
   SDL_DestroyRenderer(renderer);
   SDL_DestroyWindow(window);
   IMG_Quit();
   SDL_Quit();
   return 0;
}
