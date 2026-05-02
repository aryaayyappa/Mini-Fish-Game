#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>

// Define entity types
#define TYPE_FISH 1
#define TYPE_STARFISH 2
#define TYPE_BOMB 3
#define TYPE_POWERUP 4 // New Power-up type
#define TYPE_GOLD_FISH 5
#define TYPE_SHIELD 6
#define TYPE_SLOWMO 7
#define TYPE_MAGNET 8
#define TYPE_MINE 9
#define TYPE_JELLYFISH 10
#define TYPE_SHARK 11
#define TYPE_MULTIPLIER 12

typedef struct {
  int x, y;
  int type;
  int active;
  int dir; // For moving obstacles like sharks
} Entity;

typedef struct {
  int x, y;
  int active;
  int size;
} Bubble;

typedef struct {
  int x, y;
  int life;
  char text[16];
  int color_pair;
} Popup;

void add_popup(Popup popups[], int x, int y, const char* text, int color) {
    for(int i=0; i<10; i++) {
        if(popups[i].life <= 0) {
            popups[i].x = x;
            popups[i].y = y;
            snprintf(popups[i].text, 16, "%s", text);
            popups[i].life = 15;
            popups[i].color_pair = color;
            break;
        }
    }
}

void update_environment(int level) {
    if (!has_colors()) return;
    if (level < 4) {
        init_pair(9, COLOR_BLACK, COLOR_CYAN); // Shallow Reef
        init_pair(1, COLOR_WHITE, COLOR_CYAN);
        init_pair(2, COLOR_YELLOW, COLOR_CYAN);
        init_pair(3, COLOR_RED, COLOR_CYAN);
        init_pair(4, COLOR_BLUE, COLOR_CYAN);
        init_pair(5, COLOR_MAGENTA, COLOR_CYAN);
        init_pair(6, COLOR_BLUE, COLOR_CYAN);
        init_pair(7, COLOR_WHITE, COLOR_CYAN);
        init_pair(8, COLOR_WHITE, COLOR_RED);
        init_pair(10, COLOR_RED, COLOR_CYAN); // Mine
    } else if (level < 8) {
        init_pair(9, COLOR_CYAN, COLOR_BLUE); // Deep Ocean
        init_pair(1, COLOR_WHITE, COLOR_BLUE);
        init_pair(2, COLOR_YELLOW, COLOR_BLUE);
        init_pair(3, COLOR_RED, COLOR_BLUE);
        init_pair(4, COLOR_GREEN, COLOR_BLUE);
        init_pair(5, COLOR_MAGENTA, COLOR_BLUE);
        init_pair(6, COLOR_CYAN, COLOR_BLUE);
        init_pair(7, COLOR_WHITE, COLOR_BLUE);
        init_pair(8, COLOR_WHITE, COLOR_RED);
        init_pair(10, COLOR_RED, COLOR_BLUE);
    } else {
        init_pair(9, COLOR_WHITE, COLOR_BLACK); // Abyssal Trench
        init_pair(1, COLOR_CYAN, COLOR_BLACK);
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);
        init_pair(3, COLOR_RED, COLOR_BLACK);
        init_pair(4, COLOR_GREEN, COLOR_BLACK);
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(6, COLOR_BLUE, COLOR_BLACK);
        init_pair(7, COLOR_WHITE, COLOR_BLACK);
        init_pair(8, COLOR_WHITE, COLOR_RED);
        init_pair(10, COLOR_RED, COLOR_BLACK);
    }
}

void setup() {
  initscr();
  noecho();
  cbreak();
  nodelay(stdscr, TRUE);
  keypad(stdscr, TRUE);
  curs_set(0);
  srand(time(NULL));

  if (has_colors()) {
    start_color();
  }
}

int main() {
  setup();

  int max_y = 0, max_x = 0;
  getmaxyx(stdscr, max_y, max_x);
  
  update_environment(1);
  bkgd(COLOR_PAIR(9));

  int player_x = max_x / 2; // Start player in the middle
  int score = 0;
  int combo = 0;
  int lives = 3; // New lives system
  int level = 1;
  int shield_active = 0;
  int game_over = 0;
  int speed = 80000;
  int slowmo_timer = 0;
  int magnet_timer = 0;
  int dash_cooldown = 0;
  int last_dir = 1;
  int basket_speed = 6;
  int level_up_timer = 0;
  int paralyze_timer = 0;
  int multiplier_timer = 0;

  // Difficulty Selection
  clear();
  mvprintw(max_y / 2 - 3, (max_x / 2) - 10, "Select Difficulty:");
  mvprintw(max_y / 2 - 1, (max_x / 2) - 10, "1. Easy");
  mvprintw(max_y / 2, (max_x / 2) - 10, "2. Medium");
  mvprintw(max_y / 2 + 1, (max_x / 2) - 10, "3. Hard");
  refresh();

  nodelay(stdscr, FALSE); // Wait for user input
  while (1) {
    int diff_ch = getch();
    if (diff_ch == '1') {
      speed = 100000; // Slower game
      basket_speed = 4; // Slower basket for easy mode
      break;
    } else if (diff_ch == '2') {
      speed = 70000; // Normal
      basket_speed = 6;
      break;
    } else if (diff_ch == '3') {
      speed = 40000; // Faster game
      basket_speed = 8; // Faster basket to keep up with fast items
      break;
    }
  }
  nodelay(stdscr, TRUE); // Back to non-blocking
  clear();

  int powerup_timer = 0; // Timer for the wide basket
  int basket_width = 5;  // Default width

  Entity items[12] = {0}; // Increased pool size slightly
  Bubble bubbles[20] = {0};
  Popup popups[10] = {0};

  while (!game_over) {
    getmaxyx(stdscr, max_y, max_x);

    // --- 1. Handle Power-up Timer ---
    if (powerup_timer > 0) {
      powerup_timer--;
      basket_width = 11; // Wide basket
    } else {
      basket_width = 5; // Normal basket
    }
    
    if (slowmo_timer > 0) {
      slowmo_timer--;
    }
    if (magnet_timer > 0) magnet_timer--;
    if (dash_cooldown > 0) dash_cooldown--;
    if (level_up_timer > 0) level_up_timer--;
    if (paralyze_timer > 0) paralyze_timer--;
    if (multiplier_timer > 0) multiplier_timer--;
    
    // Ambient Bubbles Animation
    if (rand() % 4 == 0) {
      for (int i=0; i<20; i++) {
          if (!bubbles[i].active) {
              bubbles[i].x = rand() % max_x;
              bubbles[i].y = max_y - 1;
              bubbles[i].size = rand() % 3;
              bubbles[i].active = 1;
              break;
          }
      }
    }
    for(int i=0; i<20; i++) {
        if(bubbles[i].active) {
            bubbles[i].y--;
            if (rand()%4==0) bubbles[i].x += (rand()%3 - 1); 
            if(bubbles[i].y < 0) bubbles[i].active = 0;
        }
    }
    
    // Popup Animation
    for(int i=0; i<10; i++) {
        if(popups[i].life > 0) {
            if (popups[i].life % 2 == 0) popups[i].y--; // Float up slowly
            popups[i].life--;
        }
    }

    // --- 2. Handle Input ---
    // Process all pending inputs to avoid input lag and improve responsiveness
    int ch;
    int dx = 0;
    while ((ch = getch()) != ERR) {
      if (ch == 'q' || ch == 'Q') game_over = 1;
      if (ch == 'p' || ch == 'P') {
        mvprintw(max_y / 2, max_x / 2 - 4, "PAUSED");
        refresh();
        nodelay(stdscr, FALSE);
        while (getch() != 'p');
        nodelay(stdscr, TRUE);
      }
      if (paralyze_timer == 0) {
        if (ch == 'a' || ch == 'A' || ch == KEY_LEFT) { dx -= basket_speed; last_dir = -1; }
        if (ch == 'd' || ch == 'D' || ch == KEY_RIGHT) { dx += basket_speed; last_dir = 1; }
        if (ch == ' ') {
           if (dash_cooldown == 0) {
               dx += last_dir * 25; // Big dash
               dash_cooldown = 40; // 40 frames cooldown
           }
        }
      }
    }
    
    // Cap movement speed per frame
    if (dx < -30) dx = -30;
    if (dx > 30) dx = 30;
    player_x += dx;

    // Keep player within bounds based on current basket size
    if (player_x < 0)
      player_x = 0;
    if (player_x > max_x - basket_width - 1)
      player_x = max_x - basket_width - 1;

    // --- 3. Update Logic ---
    if (rand() % 10 == 0) {
      for (int i = 0; i < 12; i++) {
        if (!items[i].active) {
          items[i].x = rand() % (max_x - 4);
          items[i].y = 1;
          int r = rand() % 100;

          // Expanded Loot tables
          if (r < 40)
            items[i].type = TYPE_FISH;
          else if (r < 55)
            items[i].type = TYPE_STARFISH;
          else if (r < 65)
            items[i].type = TYPE_JELLYFISH;
          else if (r < 80) {
            int r2 = rand() % 10;
            if (level >= 8 && r2 < 2) {
               items[i].type = TYPE_SHARK;
               items[i].y = max_y - 3 - (rand() % 5); // Spawn near bottom
               items[i].dir = (rand() % 2 == 0) ? 1 : -1;
               items[i].x = items[i].dir == 1 ? 0 : max_x - 5;
            } else if (level >= 4 && r2 < 5) {
               items[i].type = TYPE_MINE;
            } else {
               items[i].type = TYPE_BOMB;
            }
          } else if (r < 86)
            items[i].type = TYPE_POWERUP;
          else if (r < 90)
            items[i].type = TYPE_SHIELD;
          else if (r < 93)
            items[i].type = TYPE_SLOWMO;
          else if (r < 95)
            items[i].type = TYPE_MAGNET;
          else if (r < 97)
            items[i].type = TYPE_MULTIPLIER;
          else
            items[i].type = TYPE_GOLD_FISH;

          items[i].active = 1;
          break;
        }
      }
    }

    // Move items & Check collisions
    for (int i = 0; i < 12; i++) {
      if (items[i].active) {
        if (items[i].type == TYPE_SHARK) {
            items[i].x += items[i].dir * 2;
            if (items[i].x < -10 || items[i].x > max_x + 10) items[i].active = 0;
            
            // Check shark collision
            if (items[i].x >= player_x - 4 && items[i].x <= player_x + basket_width + 4 && 
                items[i].y >= max_y - 4) {
                 combo = 0;
                 add_popup(popups, items[i].x, items[i].y, "SHARK BAIT!", 3);
                 lives = 0;
                 game_over = 1;
            }
            continue; // Sharks don't fall, they just move horizontally
        }

        if (magnet_timer > 0 && (items[i].type == TYPE_FISH || items[i].type == TYPE_STARFISH || items[i].type == TYPE_GOLD_FISH)) {
            // Pull towards player
            if (items[i].x < player_x + basket_width/2) items[i].x++;
            else if (items[i].x > player_x + basket_width/2) items[i].x--;
        }
        if (items[i].type == TYPE_JELLYFISH) {
            if (rand() % 2 == 0) items[i].x += (rand() % 3) - 1; // Wiggle horizontally
        }
        items[i].y++;

        if (items[i].y >= max_y - 2) {
          // Check collision using the dynamic basket width
          if (items[i].x >= player_x - 2 &&
              items[i].x <= player_x + basket_width) {
            int caught_good = 0;
            int mult = (multiplier_timer > 0) ? 2 : 1;
            if (items[i].type == TYPE_FISH) {
              int pts = (10 + combo) * mult;
              score += pts;
              char buf[16]; snprintf(buf, 16, "+%d", pts);
              add_popup(popups, items[i].x, items[i].y, buf, 1);
              caught_good = 1;
            } else if (items[i].type == TYPE_STARFISH) {
              int pts = (50 + (combo * 2)) * mult;
              score += pts;
              char buf[16]; snprintf(buf, 16, "+%d", pts);
              add_popup(popups, items[i].x, items[i].y, buf, 2);
              caught_good = 1;
            } else if (items[i].type == TYPE_GOLD_FISH) {
              int pts = (200 + (combo * 5)) * mult;
              score += pts;
              char buf[16]; snprintf(buf, 16, "+%d!", pts);
              add_popup(popups, items[i].x, items[i].y, buf, 2);
              caught_good = 1;
            } else if (items[i].type == TYPE_POWERUP) {
              powerup_timer = 150; // Active for 150 frames
              add_popup(popups, items[i].x, items[i].y, "WIDE!", 5);
            } else if (items[i].type == TYPE_SLOWMO) {
              slowmo_timer = 200;
              add_popup(popups, items[i].x, items[i].y, "SLOW!", 7);
            } else if (items[i].type == TYPE_MAGNET) {
              magnet_timer = 200;
              add_popup(popups, items[i].x, items[i].y, "MAGNET!", 8);
            } else if (items[i].type == TYPE_MULTIPLIER) {
              multiplier_timer = 200;
              add_popup(popups, items[i].x, items[i].y, "SCORE x2!", 2);
            } else if (items[i].type == TYPE_JELLYFISH) {
              combo = 0;
              paralyze_timer = 40;
              add_popup(popups, items[i].x, items[i].y, "ZAPPED!", 3);
            } else if (items[i].type == TYPE_SHIELD) {
              shield_active = 1;
              add_popup(popups, items[i].x, items[i].y, "SHIELD!", 6);
            } else if (items[i].type == TYPE_MINE) {
              combo = 0;
              add_popup(popups, items[i].x, items[i].y, "FATAL!", 3);
              lives -= 2;
              score -= 500;
              if (score < 0) score = 0;
              if (lives <= 0) game_over = 1;
              shield_active = 0; // Mines instantly break shields
            } else if (items[i].type == TYPE_BOMB) {
              combo = 0;
              add_popup(popups, items[i].x, items[i].y, "BOOM!", 3);
              if (shield_active) {
                shield_active = 0; // Shield absorbed bomb
              } else {
                lives--; // Lose a life instead of instant death
                if (lives <= 0)
                  game_over = 1;
              }
            }

            if (caught_good) combo++;

            if (speed > 30000)
              speed -= 500;
              
            // Level up logic
            if (score >= level * 500) {
              level++;
              lives++; // Extra life on level up
              if (speed > 15000) speed -= 3000;
              basket_speed++; // Increase basket speed as levels get harder
              level_up_timer = 40; // Display level up splash
              update_environment(level);
            }
          } else {
            // Missed item
            if (items[i].type == TYPE_FISH || items[i].type == TYPE_STARFISH || items[i].type == TYPE_GOLD_FISH) {
               combo = 0; // Reset combo on miss
            }
            if (items[i].type != TYPE_BOMB) {
               add_popup(popups, items[i].x, max_y - 2, "~splash~", 9);
            }
          }
          items[i].active = 0;
        }
      }
    }

    // --- 4. Render Graphics ---
    erase();
    
    // Draw Ambient Bubbles
    for(int i=0; i<20; i++) {
        if(bubbles[i].active) {
            attron(COLOR_PAIR(9) | A_DIM);
            char c = bubbles[i].size == 0 ? '.' : (bubbles[i].size == 1 ? 'o' : 'O');
            mvprintw(bubbles[i].y, bubbles[i].x, "%c", c);
            attroff(COLOR_PAIR(9) | A_DIM);
        }
    }

    // Draw UI (Now includes Lives)
    mvprintw(0, 2, "LEVEL: %d | SCORE: %d | COMBO: %dx | LIVES: ", level, score, combo);
    for (int l = 0; l < lives; l++)
      printw("X "); // Draw lives as X's
      
    if (dash_cooldown == 0) {
       attron(COLOR_PAIR(1));
       printw(" | DASH: READY");
       attroff(COLOR_PAIR(1));
    } else {
       printw(" | DASH: WAIT");
    }

    if (powerup_timer > 0) {
      attron(COLOR_PAIR(5));
      mvprintw(0, 35, "POWER UP!");
      attroff(COLOR_PAIR(5));
    }
    
    if (shield_active) {
      attron(COLOR_PAIR(6));
      mvprintw(0, 50, "SHIELD ACTIVE!");
      attroff(COLOR_PAIR(6));
    }
    
    if (slowmo_timer > 0) {
      attron(COLOR_PAIR(7));
      mvprintw(0, 70, "SLOW MOTION!");
      attroff(COLOR_PAIR(7));
    }
    
    if (magnet_timer > 0) {
      attron(COLOR_PAIR(8));
      mvprintw(0, 90, "MAGNET ACTIVE!");
      attroff(COLOR_PAIR(8));
    }

    if (paralyze_timer > 0) {
      attron(COLOR_PAIR(3) | A_BOLD);
      mvprintw(0, 110, "PARALYZED!");
      attroff(COLOR_PAIR(3) | A_BOLD);
    }
    
    if (multiplier_timer > 0) {
      attron(COLOR_PAIR(4) | A_BOLD);
      mvprintw(0, 130, "x2 SCORE!");
      attroff(COLOR_PAIR(4) | A_BOLD);
    }

    // Draw Level Up Splash
    if (level_up_timer > 0) {
        if (level_up_timer % 6 > 2) { // Flashing effect
            attron(COLOR_PAIR(2) | A_BOLD);
            mvprintw(max_y/2, max_x/2 - 5, "LEVEL UP!");
            attroff(COLOR_PAIR(2) | A_BOLD);
        }
    }

    // Draw Popups
    for(int i=0; i<10; i++) {
        if(popups[i].life > 0) {
            attron(COLOR_PAIR(popups[i].color_pair) | A_BOLD);
            mvprintw(popups[i].y, popups[i].x, "%s", popups[i].text);
            attroff(COLOR_PAIR(popups[i].color_pair) | A_BOLD);
        }
    }

    // Draw items
    for (int i = 0; i < 12; i++) {
      if (items[i].active) {
        if (level >= 8) {
           int dist_y = abs(items[i].y - (max_y - 2));
           int dist_x = abs(items[i].x - player_x);
           if (dist_y > 10 || dist_x > 15) continue; // Abyssal Darkness
        }
        
        if (items[i].type == TYPE_FISH) {
          attron(COLOR_PAIR(1) | A_BOLD);
          if ((items[i].y / 2) % 2 == 0) mvprintw(items[i].y, items[i].x, "<><");
          else mvprintw(items[i].y, items[i].x, "><>");
          attroff(COLOR_PAIR(1) | A_BOLD);
        } else if (items[i].type == TYPE_STARFISH) {
          attron(COLOR_PAIR(2) | A_BOLD);
          char* stars[] = {"*", "+", "x"};
          mvprintw(items[i].y, items[i].x, "%s", stars[(items[i].y / 2) % 3]);
          attroff(COLOR_PAIR(2) | A_BOLD);
        } else if (items[i].type == TYPE_BOMB) {
          attron(COLOR_PAIR(3));
          mvprintw(items[i].y, items[i].x, "O");
          attroff(COLOR_PAIR(3));
        } else if (items[i].type == TYPE_POWERUP) {
          attron(COLOR_PAIR(5));
          mvprintw(items[i].y, items[i].x, "[+]");
          attroff(COLOR_PAIR(5));
        } else if (items[i].type == TYPE_GOLD_FISH) {
          attron(COLOR_PAIR(2) | A_BOLD);
          mvprintw(items[i].y, items[i].x, "$$$");
          attroff(COLOR_PAIR(2) | A_BOLD);
        } else if (items[i].type == TYPE_SHIELD) {
          attron(COLOR_PAIR(6));
          mvprintw(items[i].y, items[i].x, "[S]");
          attroff(COLOR_PAIR(6));
        } else if (items[i].type == TYPE_SLOWMO) {
          attron(COLOR_PAIR(7));
          mvprintw(items[i].y, items[i].x, "[~]");
          attroff(COLOR_PAIR(7));
        } else if (items[i].type == TYPE_MAGNET) {
          attron(COLOR_PAIR(8));
          mvprintw(items[i].y, items[i].x, "[M]");
          attroff(COLOR_PAIR(8));
        } else if (items[i].type == TYPE_MINE) {
          attron(COLOR_PAIR(10) | A_BOLD);
          mvprintw(items[i].y, items[i].x, "[X]");
          attroff(COLOR_PAIR(10) | A_BOLD);
        } else if (items[i].type == TYPE_JELLYFISH) {
          attron(COLOR_PAIR(5) | A_BOLD);
          mvprintw(items[i].y, items[i].x, "~@~");
          attroff(COLOR_PAIR(5) | A_BOLD);
        } else if (items[i].type == TYPE_MULTIPLIER) {
          attron(COLOR_PAIR(4) | A_BOLD);
          mvprintw(items[i].y, items[i].x, "[x2]");
          attroff(COLOR_PAIR(4) | A_BOLD);
        } else if (items[i].type == TYPE_SHARK) {
          attron(COLOR_PAIR(3) | A_BOLD); // Red Shark
          if (items[i].dir == 1) mvprintw(items[i].y, items[i].x, "==///>");
          else mvprintw(items[i].y, items[i].x, "<\\\\\\==");
          attroff(COLOR_PAIR(3) | A_BOLD);
        }
      }
    }

    // Draw Player Basket dynamically based on power-up state
    attron(COLOR_PAIR(4));
    if (powerup_timer > 0) {
      mvprintw(max_y - 2, player_x, "\\_________/"); // Wide
    } else {
      mvprintw(max_y - 2, player_x, "\\___/"); // Normal
    }
    attroff(COLOR_PAIR(4));

    refresh();

    // Frame Rate Control
    if (slowmo_timer > 0) {
      usleep(speed * 2);
    } else {
      usleep(speed);
    }
  }

  // Game Over Sequence
  nodelay(stdscr, FALSE);
  for (int i = 0; i < 6; i++) {
      clear();
      if (i % 2 == 0) {
          attron(COLOR_PAIR(3) | A_BOLD);
          mvprintw(max_y / 2, (max_x / 2) - 5, "GAME OVER!");
          attroff(COLOR_PAIR(3) | A_BOLD);
      }
      refresh();
      usleep(250000); // flash delay
  }
  clear();
  attron(COLOR_PAIR(3) | A_BOLD);
  mvprintw(max_y / 2 - 2, (max_x / 2) - 5, "GAME OVER!");
  attroff(COLOR_PAIR(3) | A_BOLD);
  mvprintw((max_y / 2), (max_x / 2) - 8, "Final Score: %d", score);
  mvprintw((max_y / 2) + 1, (max_x / 2) - 8, "Level Reached: %d", level);
  mvprintw((max_y / 2) + 3, (max_x / 2) - 12, "Press any key to exit...");
  refresh();
  
  flushinp(); // Clear lingering input
  getch();
  endwin();

  return 0;
}
