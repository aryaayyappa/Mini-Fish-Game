#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

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

#define MAX_USERS 100

typedef struct {
    char id[5];       // 4-letter initial ID
    int high_score;
    int max_level;
    int stored_lives; // Lives saved between sessions
    long last_life_regen; // Unix timestamp of last regen
    int best_time;    // Best total survival seconds
} UserProfile;

UserProfile users[MAX_USERS];
int num_users = 0;

void load_users() {
    FILE *f = fopen("users.dat", "rb");
    if (f) {
        fread(&num_users, sizeof(int), 1, f);
        fread(users, sizeof(UserProfile), num_users, f);
        fclose(f);
    }
}

void save_users() {
    FILE *f = fopen("users.dat", "wb");
    if (f) {
        fwrite(&num_users, sizeof(int), 1, f);
        fwrite(users, sizeof(UserProfile), num_users, f);
        fclose(f);
    }
}

UserProfile* get_or_create_user(const char* id) {
    for (int i=0; i<num_users; i++) {
        if (strcmp(users[i].id, id) == 0) return &users[i];
    }
    if (num_users < MAX_USERS) {
        strcpy(users[num_users].id, id);
        users[num_users].high_score = 0;
        users[num_users].max_level = 1;
        users[num_users].stored_lives = 3;
        users[num_users].last_life_regen = (long)time(NULL);
        users[num_users].best_time = 0;
        num_users++;
        save_users();
        return &users[num_users - 1];
    }
    return NULL;
}

UserProfile* login_screen(int max_y, int max_x) {
    char id[5] = {0};
    int len = 0;
    
    nodelay(stdscr, FALSE);
    while(1) {
        clear();
        bkgd(COLOR_PAIR(9));
        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(max_y/2 - 2, max_x/2 - 15, "--- HUNGRY FISH LOGIN ---");
        attroff(COLOR_PAIR(2) | A_BOLD);
        mvprintw(max_y/2, max_x/2 - 20, "Enter 4-letter Player ID: %s", id);
        refresh();
        
        int ch = getch();
        if (ch == '\n' || ch == '\r') {
            if (len == 4) break;
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (len > 0) id[--len] = '\0';
        } else if (isalpha(ch) && len < 4) {
            id[len++] = toupper(ch);
            id[len] = '\0';
        }
    }
    nodelay(stdscr, TRUE);
    return get_or_create_user(id);
}

int main_menu(UserProfile* current_user, int max_y, int max_x) {
    int choice = 0;
    const char* options[] = {"Play Game", "Level Grid", "Leaderboard", "Logout", "Quit"};
    int num_options = 5;
    nodelay(stdscr, FALSE);
    
    // Life Regeneration: +1 life every 5 minutes (300 seconds)
    #define REGEN_SECS 300
    #define MAX_LIVES 5
    while(1) {
        long now = (long)time(NULL);
        long elapsed = now - current_user->last_life_regen;
        int lives_to_add = (int)(elapsed / REGEN_SECS);
        if (current_user->stored_lives < MAX_LIVES && lives_to_add > 0) {
            current_user->stored_lives += lives_to_add;
            if (current_user->stored_lives > MAX_LIVES) current_user->stored_lives = MAX_LIVES;
            current_user->last_life_regen += lives_to_add * REGEN_SECS;
            save_users();
        }

        long secs_since = now - current_user->last_life_regen;
        long next_regen_in = REGEN_SECS - secs_since;
        if (current_user->stored_lives >= MAX_LIVES) next_regen_in = -1;

        clear();
        bkgd(COLOR_PAIR(9));
        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(max_y/2 - 7, max_x/2 - 12, "=== HUNGRY FISH ===");
        attroff(COLOR_PAIR(1) | A_BOLD);
        
        mvprintw(max_y/2 - 5, max_x/2 - 18, "Player: %s", current_user->id);
        mvprintw(max_y/2 - 4, max_x/2 - 18, "High Score: %d  |  Max Level: %d", current_user->high_score, current_user->max_level);

        // Show lives and regen timer
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(max_y/2 - 3, max_x/2 - 18, "Lives: ");
        attroff(COLOR_PAIR(3) | A_BOLD);
        for (int l = 0; l < MAX_LIVES; l++) {
            if (l < current_user->stored_lives) {
                attron(COLOR_PAIR(3) | A_BOLD);
                printw("[<3] ");
                attroff(COLOR_PAIR(3) | A_BOLD);
            } else {
                attron(A_DIM);
                printw("[   ] ");
                attroff(A_DIM);
            }
        }
        if (next_regen_in >= 0) {
            attron(COLOR_PAIR(7));
            mvprintw(max_y/2 - 2, max_x/2 - 18, "Next life in: %02ld:%02ld", next_regen_in/60, next_regen_in%60);
            attroff(COLOR_PAIR(7));
        } else {
            attron(COLOR_PAIR(4));
            mvprintw(max_y/2 - 2, max_x/2 - 18, "Lives full!");
            attroff(COLOR_PAIR(4));
        }
        
        for (int i=0; i<num_options; i++) {
            if (i == choice) {
                attron(A_REVERSE | A_BOLD);
                mvprintw(max_y/2 + i, max_x/2 - 8, "-> %s", options[i]);
                attroff(A_REVERSE | A_BOLD);
            } else {
                mvprintw(max_y/2 + i, max_x/2 - 8, "   %s", options[i]);
            }
        }
        refresh();
        
        int ch = getch();
        if (ch == KEY_UP || ch == 'w') {
            choice--;
            if (choice < 0) choice = num_options - 1;
        } else if (ch == KEY_DOWN || ch == 's') {
            choice++;
            if (choice >= num_options) choice = 0;
        } else if (ch == '\n' || ch == '\r') {
            nodelay(stdscr, TRUE);
            return choice;
        }
    }
}

int compare_users(const void* a, const void* b) {
    UserProfile* u1 = (UserProfile*)a;
    UserProfile* u2 = (UserProfile*)b;
    if (u2->high_score != u1->high_score)
        return u2->high_score - u1->high_score;
    return u2->best_time - u1->best_time; // tiebreak by survival time
}

void show_leaderboard(int max_y, int max_x) {
    clear();
    bkgd(COLOR_PAIR(9));
    attron(COLOR_PAIR(2) | A_BOLD);
    mvprintw(max_y/2 - 8, max_x/2 - 10, "--- LEADERBOARD ---");
    attroff(COLOR_PAIR(2) | A_BOLD);
    
    UserProfile sorted[MAX_USERS];
    memcpy(sorted, users, num_users * sizeof(UserProfile));
    qsort(sorted, num_users, sizeof(UserProfile), compare_users);
    
    mvprintw(max_y/2 - 6, max_x/2 - 20, "RANK  ID    SCORE   LVL  BEST TIME");
    mvprintw(max_y/2 - 5, max_x/2 - 20, "----------------------------------");
    for (int i=0; i<10 && i<num_users; i++) {
        int bt = sorted[i].best_time;
        mvprintw(max_y/2 - 4 + i, max_x/2 - 20,
            "%2d.   %s  %6d  %3d   %02d:%02d",
            i+1, sorted[i].id, sorted[i].high_score,
            sorted[i].max_level, bt/60, bt%60);
    }
    
    mvprintw(max_y/2 + 8, max_x/2 - 15, "Press any key to return...");
    refresh();
    nodelay(stdscr, FALSE);
    getch();
    nodelay(stdscr, TRUE);
}

// Returns difficulty: 1=Easy, 2=Medium, 3=Hard
int select_difficulty(int max_y, int max_x) {
    int choice = 0;
    const char* opts[] = {
        "[1] Easy   - More time, slower pace",
        "[2] Medium - Balanced challenge",
        "[3] Hard   - Race the clock!"
    };
    nodelay(stdscr, FALSE);
    while(1) {
        clear();
        bkgd(COLOR_PAIR(9));
        attron(COLOR_PAIR(2) | A_BOLD);
        mvprintw(max_y/2 - 5, max_x/2 - 12, "SELECT DIFFICULTY");
        attroff(COLOR_PAIR(2) | A_BOLD);
        for (int i=0; i<3; i++) {
            if (i == choice) { attron(A_REVERSE | A_BOLD); }
            mvprintw(max_y/2 - 2 + i*2, max_x/2 - 20, "%s", opts[i]);
            if (i == choice) { attroff(A_REVERSE | A_BOLD); }
        }
        mvprintw(max_y/2 + 5, max_x/2 - 20, "Arrow Keys / W-S to choose, ENTER to confirm");
        refresh();
        int ch = getch();
        if (ch == KEY_UP || ch == 'w') { choice--; if (choice < 0) choice = 2; }
        else if (ch == KEY_DOWN || ch == 's') { choice++; if (choice > 2) choice = 0; }
        else if (ch == '1') { nodelay(stdscr, TRUE); return 1; }
        else if (ch == '2') { nodelay(stdscr, TRUE); return 2; }
        else if (ch == '3') { nodelay(stdscr, TRUE); return 3; }
        else if (ch == '\n' || ch == '\r') { nodelay(stdscr, TRUE); return choice + 1; }
    }
}

int show_level_grid(UserProfile* user, int max_y, int max_x) {
    int choice = 1;
    nodelay(stdscr, FALSE);
    
    while(1) {
        clear();
        bkgd(COLOR_PAIR(9));
        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(max_y/2 - 10, max_x/2 - 10, "--- LEVEL GRID ---");
        attroff(COLOR_PAIR(1) | A_BOLD);
        
        mvprintw(max_y/2 - 8, max_x/2 - 25, "Select a starting level (up to your Max Level: %d)", user->max_level);
        
        int grid_rows = 5;
        int grid_cols = 5;
        int start_y = max_y/2 - 5;
        int start_x = max_x/2 - 20;
        
        for (int r = 0; r < grid_rows; r++) {
            for (int c = 0; c < grid_cols; c++) {
                int lvl = r * grid_cols + c + 1;
                int is_unlocked = (lvl <= user->max_level);
                
                if (lvl == choice) attron(A_REVERSE);
                
                if (is_unlocked) {
                    mvprintw(start_y + r*2, start_x + c*8, "[ %2d ]", lvl);
                } else {
                    attron(A_DIM);
                    mvprintw(start_y + r*2, start_x + c*8, "[ ** ]");
                    attroff(A_DIM);
                }
                
                if (lvl == choice) attroff(A_REVERSE);
            }
        }
        
        mvprintw(max_y/2 + 7, max_x/2 - 25, "Arrows to move | ENTER to start | Q to back");
        refresh();
        
        int ch = getch();
        if (ch == 'q' || ch == 'Q') {
            nodelay(stdscr, TRUE);
            return 0; // go back
        } else if (ch == KEY_UP || ch == 'w') {
            if (choice - grid_cols >= 1) choice -= grid_cols;
        } else if (ch == KEY_DOWN || ch == 's') {
            if (choice + grid_cols <= grid_rows * grid_cols) choice += grid_cols;
        } else if (ch == KEY_LEFT || ch == 'a') {
            if (choice > 1) choice--;
        } else if (ch == KEY_RIGHT || ch == 'd') {
            if (choice < grid_rows * grid_cols) choice++;
        } else if (ch == '\n' || ch == '\r') {
            if (choice <= user->max_level) {
                nodelay(stdscr, TRUE);
                return choice;
            }
        }
    }
}

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

int play_game(UserProfile* user, int start_level, int difficulty) {
  int max_y = 0, max_x = 0;
  getmaxyx(stdscr, max_y, max_x);
  
  int level = start_level;
  update_environment(level);

  // Use stored lives from profile
  if (user->stored_lives <= 0) user->stored_lives = 1;
  
  int player_x = max_x / 2;
  int score = 0;
  int combo = 0;
  int lives = user->stored_lives;
  int shield_active = 0;
  int game_over = 0;

  // Speed/basket based on difficulty
  int base_speed = (difficulty == 1) ? 110000 : (difficulty == 2) ? 75000 : 45000;
  int speed = base_speed - ((level - 1) * 2000);
  if (speed < 15000) speed = 15000;

  // Level timer: Easy=90s, Medium=60s, Hard=40s; decreases by 5s per level
  int base_time = (difficulty == 1) ? 90 : (difficulty == 2) ? 60 : 40;
  int level_time_limit = base_time - ((level - 1) * 5);
  if (level_time_limit < 15) level_time_limit = 15;

  int basket_speed = (difficulty == 1) ? 5 : (difficulty == 2) ? 7 : 9;
  basket_speed += level / 5;

  // Frame counter for 1-second ticks
  int frames_per_sec = 1000000 / (speed > 0 ? speed : 1);
  if (frames_per_sec < 1) frames_per_sec = 1;
  int frame_count = 0;
  int level_secs_left = level_time_limit;
  int total_secs = 0;

  int slowmo_timer = 0;
  int magnet_timer = 0;
  int dash_cooldown = 0;
  int last_dir = 1;
  int level_up_timer = 0;
  int paralyze_timer = 0;
  int multiplier_timer = 0;
  int powerup_timer = 0;
  int basket_width = 5;

  clear();
  Entity items[12] = {0};
  Bubble bubbles[20] = {0};
  Popup popups[10] = {0};

  while (!game_over) {
    getmaxyx(stdscr, max_y, max_x);
    frame_count++;

    // Recalculate fps each frame (speed can change)
    int cur_fps = 1000000 / speed;
    if (cur_fps < 1) cur_fps = 1;
    if (frame_count >= cur_fps) {
      frame_count = 0;
      level_secs_left--;
      total_secs++;
      if (level_secs_left <= 0) {
        // Time's up — lose a life
        lives--;
        add_popup(popups, max_x/2 - 5, max_y/2, "TIME UP! -LIFE", 3);
        if (lives <= 0) {
          game_over = 1;
        } else {
          level_secs_left = level_time_limit; // Reset timer for new life
        }
      }
    }

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
        nodelay(stdscr, FALSE);
        while(1) {
          erase();
          if ((int)(time(NULL)) % 2 == 0) {
            attron(COLOR_PAIR(2) | A_BOLD);
            mvprintw(max_y/2 - 1, max_x/2 - 6, "** PAUSED **");
            attroff(COLOR_PAIR(2) | A_BOLD);
          }
          mvprintw(max_y/2 + 1, max_x/2 - 14, "Press P to resume, Q to quit");
          refresh();
          usleep(300000);
          int pc = getch();
          if (pc == 'p' || pc == 'P') break;
          if (pc == 'q' || pc == 'Q') { game_over = 1; break; }
        }
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
              
            // Level up logic: level * 100 threshold
            if (score >= level * 100) {
              // Award time bonus
              int time_bonus = level_secs_left * 5;
              score += time_bonus;
              char tbuf[24]; snprintf(tbuf, 24, "TIME BONUS +%d!", time_bonus);
              add_popup(popups, max_x/2 - 8, max_y/2 - 2, tbuf, 2);

              level++;
              lives++; // Extra life on level up
              user->stored_lives = lives;

              // Recalculate timer for new level
              level_time_limit = base_time - ((level - 1) * 5);
              if (level_time_limit < 15) level_time_limit = 15;
              level_secs_left = level_time_limit;

              if (speed > 15000) speed -= 2000;
              basket_speed++;
              level_up_timer = 40;
              update_environment(level);
              frame_count = 0;
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

    // Draw UI
    int need_secs = level * 100 - score;
    if (need_secs < 0) need_secs = 0;
    mvprintw(0, 1, "[%s] LVL:%d SCR:%d NEED:%d CMB:%dx ",
             user->id, level, score, need_secs, combo);
    for (int l = 0; l < lives; l++) printw("<3 ");
    // Timer — flash red when below 10s
    if (level_secs_left <= 10) attron(COLOR_PAIR(3) | A_BOLD);
    else attron(COLOR_PAIR(2));
    printw(" | TIME: %02d", level_secs_left);
    attroff(COLOR_PAIR(3) | A_BOLD);
    attroff(COLOR_PAIR(2));
    printw(" | [P]Pause");

    if (powerup_timer > 0) {
      attron(COLOR_PAIR(5));
      mvprintw(1, 1, "[+WIDE]");
      attroff(COLOR_PAIR(5));
    }
    if (shield_active) {
      attron(COLOR_PAIR(6));
      printw(" [SHIELD]");
      attroff(COLOR_PAIR(6));
    }
    if (slowmo_timer > 0) {
      attron(COLOR_PAIR(7));
      printw(" [SLOW]");
      attroff(COLOR_PAIR(7));
    }
    if (magnet_timer > 0) {
      attron(COLOR_PAIR(8));
      printw(" [MAGNET]");
      attroff(COLOR_PAIR(8));
    }
    if (paralyze_timer > 0) {
      attron(COLOR_PAIR(3) | A_BOLD);
      printw(" [ZAPPED!]");
      attroff(COLOR_PAIR(3) | A_BOLD);
    }
    if (multiplier_timer > 0) {
      attron(COLOR_PAIR(4) | A_BOLD);
      printw(" [x2]");
      attroff(COLOR_PAIR(4) | A_BOLD);
    }
    if (dash_cooldown == 0) {
      attron(COLOR_PAIR(1));
      printw(" [DASH READY]");
      attroff(COLOR_PAIR(1));
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
          // Classic arcade fish animation alternates direction
          if ((items[i].y / 2) % 2 == 0) mvprintw(items[i].y, items[i].x, "><(((*>");
          else                            mvprintw(items[i].y, items[i].x, "<*)))><");
          attroff(COLOR_PAIR(1) | A_BOLD);
        } else if (items[i].type == TYPE_STARFISH) {
          attron(COLOR_PAIR(2) | A_BOLD);
          const char* star_frames[] = {"-*-", "-+-", "*-*"};
          mvprintw(items[i].y, items[i].x, "%s", star_frames[(items[i].y/2)%3]);
          attroff(COLOR_PAIR(2) | A_BOLD);
        } else if (items[i].type == TYPE_BOMB) {
          attron(COLOR_PAIR(3) | A_BOLD);
          const char* bomb_frames[] = {"(B)", "(b)", "(*)"};
          mvprintw(items[i].y, items[i].x, "%s", bomb_frames[(items[i].y/2)%3]);
          attroff(COLOR_PAIR(3) | A_BOLD);
        } else if (items[i].type == TYPE_POWERUP) {
          attron(COLOR_PAIR(5) | A_BOLD);
          mvprintw(items[i].y, items[i].x, "(+)");
          attroff(COLOR_PAIR(5) | A_BOLD);
        } else if (items[i].type == TYPE_GOLD_FISH) {
          attron(COLOR_PAIR(2) | A_BOLD);
          if ((items[i].y/2)%2==0) mvprintw(items[i].y, items[i].x, "><$$$>");
          else                     mvprintw(items[i].y, items[i].x, "<$$$><");
          attroff(COLOR_PAIR(2) | A_BOLD);
        } else if (items[i].type == TYPE_SHIELD) {
          attron(COLOR_PAIR(6) | A_BOLD);
          mvprintw(items[i].y, items[i].x, "(S)");
          attroff(COLOR_PAIR(6) | A_BOLD);
        } else if (items[i].type == TYPE_SLOWMO) {
          attron(COLOR_PAIR(7) | A_BOLD);
          mvprintw(items[i].y, items[i].x, "(~)");
          attroff(COLOR_PAIR(7) | A_BOLD);
        } else if (items[i].type == TYPE_MAGNET) {
          attron(COLOR_PAIR(8) | A_BOLD);
          mvprintw(items[i].y, items[i].x, "(M)");
          attroff(COLOR_PAIR(8) | A_BOLD);
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
  mvprintw((max_y / 2), (max_x / 2) - 10, "Final Score: %d", score);
  mvprintw((max_y / 2) + 1, (max_x / 2) - 10, "Level Reached: %d", level);
  mvprintw((max_y / 2) + 2, (max_x / 2) - 10, "Time Survived: %02d:%02d", total_secs/60, total_secs%60);
  mvprintw((max_y / 2) + 4, (max_x / 2) - 15, "Press R to Retry, M for Menu...");
  refresh();
  
  if (score > user->high_score) user->high_score = score;
  if (level > user->max_level) user->max_level = level;
  if (total_secs > user->best_time) user->best_time = total_secs;
  user->stored_lives = lives > 0 ? lives : 0;
  user->last_life_regen = (long)time(NULL); // Start regen from now if 0 lives
  save_users();
  
  flushinp();
  nodelay(stdscr, FALSE);
  int ch;
  while(1) {
      ch = getch();
      if (ch == 'r' || ch == 'R') { nodelay(stdscr, TRUE); return 1; }
      if (ch == 'm' || ch == 'M') { nodelay(stdscr, TRUE); return 0; }
  }
}

int main() {
  setup();
  load_users();
  
  int max_y = 0, max_x = 0;
  getmaxyx(stdscr, max_y, max_x);
  update_environment(1);
  
  while(1) {
      UserProfile* current_user = login_screen(max_y, max_x);
      
      int logged_in = 1;
      while(logged_in) {
          int choice = main_menu(current_user, max_y, max_x);
          
          if (choice == 0) {
              int diff = select_difficulty(max_y, max_x);
              int action = 1;
              while(action == 1)
                  action = play_game(current_user, 1, diff);
          } else if (choice == 1) {
              int selected = show_level_grid(current_user, max_y, max_x);
              if (selected > 0) {
                  int diff = select_difficulty(max_y, max_x);
                  int action = 1;
                  while(action == 1)
                      action = play_game(current_user, selected, diff);
              }
          } else if (choice == 2) {
              show_leaderboard(max_y, max_x);
          } else if (choice == 3) {
              logged_in = 0;
          } else if (choice == 4) {
              endwin();
              return 0;
          }
      }
  }
  endwin();
  return 0;
}
