#include <math.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==========================================
// 1. DATA STRUCTURES (Unchanged)
// ==========================================
typedef struct {
  int gender; // 1: Male, 2: Female
  float height;
  float massBody;
} athlete;

typedef struct {
  float M_upperArm;
  float M_foreArm;
  float M_hand;
  float M_arm;
} mass_arm;

typedef struct {
  float L_upperArm;
  float L_forearm;
  float L_hand;
  float L_arm;
  float L_cg;
} length_arm;

typedef struct {
  float L_pec;
  float A_pec;
  float L_lat;
  float A_lat;
  float PCSA_ratio;
} muscle_insertion;

// ==========================================
// 2. PHYSICS LOGIC (Unchanged Logic)
// ==========================================

float upperArm_M(int gender, float weight, float height) {
  height = height * 100;
  float b0 = (gender == 1) ? -5.40 : -3.90;
  float b1 = (gender == 1) ? 0.0336 : 0.0363;
  float b2 = (gender == 1) ? 0.0242 : 0.0162;
  return b0 + (b1 * weight) + (b2 * height);
}

float foreArm_M(int gender, float weight, float height) {
  height = height * 100;
  float b0 = (gender == 1) ? 0.86 : 0.28;
  float b1 = (gender == 1) ? 0.0123 : 0.0095;
  float b2 = (gender == 1) ? -0.0051 : -0.0011;
  return b0 + (b1 * weight) + (b2 * height);
}

float hand_M(int gender, float weight, float height) {
  height = height * 100;
  float b0 = (gender == 1) ? -0.27 : -0.24;
  float b1 = (gender == 1) ? 0.0055 : 0.0039;
  float b2 = (gender == 1) ? 0.0023 : 0.0028;
  return b0 + (b1 * weight) + (b2 * height);
}

float upperArm_L(int gender, float height) {
  return (gender == 1) ? height * 0.186 : height * 0.182;
}

float foreArm_L(int gender, float height) {
  return (gender == 1) ? height * 0.146 : height * 0.143;
}

float hand_L(int gender, float height) {
  return (gender == 1) ? height * 0.108 : height * 0.106;
}

float cog_Arm(float mass_arm, float mass_upperArm, float mass_foreArm,
              float mass_hand, float length_upperArm, float length_forearm,
              float length_hand) {
  float x1 = length_upperArm * 0.436;
  float x2 = length_upperArm + (length_forearm * 0.430);
  float x3 = length_upperArm + length_forearm + (length_hand * 0.506);
  return ((mass_upperArm * x1) + (mass_foreArm * x2) + (mass_hand * x3)) /
         mass_arm;
}

// ==========================================
// 3. TUI HELPER FUNCTIONS
// ==========================================

void draw_centered(int y, const char *text) {
  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);
  int start_x = (max_x - strlen(text)) / 2;
  mvprintw(y, start_x, "%s", text);
}

void init_colors() {
  start_color();
  // Pair 1: Cyan text on Black (Titles)
  init_pair(1, COLOR_CYAN, COLOR_BLACK);
  // Pair 2: Green text on Black (Results)
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  // Pair 3: Black text on White (Input Fields)
  init_pair(3, COLOR_BLACK, COLOR_WHITE);
  // Pair 4: Red text (Errors)
  init_pair(4, COLOR_RED, COLOR_BLACK);
}

// ==========================================
// 4. MAIN TUI LOGIC
// ==========================================

int main() {
  // --- NCURSES SETUP ---
  initscr();            // Start ncurses
  cbreak();             // No line buffering
  noecho();             // Don't echo inputs automatically
  keypad(stdscr, TRUE); // Enable arrow keys
  init_colors();        // Setup colors

  int row, col;
  getmaxyx(stdscr, row, col);

  while (1) {
    clear();

    // --- DRAW TITLE ---
    attron(COLOR_PAIR(1) | A_BOLD);
    box(stdscr, 0, 0);
    draw_centered(2, "BIOMECHANICAL ANALYZER: IRON CROSS");
    attroff(COLOR_PAIR(1) | A_BOLD);

    // --- INPUT VARIABLES ---
    int gender = 0;
    float height = 0.0;
    float weight = 0.0;
    char buf[20];

    // --- DRAW FORM ---
    int form_y = row / 2 - 5;
    int center_x = col / 2;

    mvprintw(form_y, center_x - 15, "Gender (M/F):");
    mvprintw(form_y + 2, center_x - 15, "Height (m):");
    mvprintw(form_y + 4, center_x - 15, "Weight (kg):");

    // --- INPUT: GENDER ---
    move(form_y, center_x + 5);
    int ch;
    while (1) {
      ch = getch();
      if (ch == 'm' || ch == 'M') {
        gender = 1;
        mvprintw(form_y, center_x + 5, "Male  ");
        break;
      }
      if (ch == 'f' || ch == 'F') {
        gender = 2;
        mvprintw(form_y, center_x + 5, "Female");
        break;
      }
    }

    // --- INPUT: HEIGHT ---
    echo();                // Enable echo so user sees what they type
    attron(COLOR_PAIR(3)); // Highlight input field
    mvprintw(form_y + 2, center_x + 5, "     "); // Clear space
    move(form_y + 2, center_x + 5);
    getnstr(buf, 10);
    height = atof(buf);
    attroff(COLOR_PAIR(3));

    // --- INPUT: WEIGHT ---
    attron(COLOR_PAIR(3));
    mvprintw(form_y + 4, center_x + 5, "     ");
    move(form_y + 4, center_x + 5);
    getnstr(buf, 10);
    weight = atof(buf);
    attroff(COLOR_PAIR(3));
    noecho(); // Disable echo again

    // --- CALCULATIONS (Wrapped from your code) ---
    mass_arm m_arm;
    m_arm.M_upperArm = upperArm_M(gender, weight, height);
    m_arm.M_foreArm = foreArm_M(gender, weight, height);
    m_arm.M_hand = hand_M(gender, weight, height);
    m_arm.M_arm = m_arm.M_upperArm + m_arm.M_foreArm + m_arm.M_hand;

    length_arm l_arm;
    l_arm.L_upperArm = upperArm_L(gender, height);
    l_arm.L_forearm = foreArm_L(gender, height);
    l_arm.L_hand = hand_L(gender, height);
    l_arm.L_arm = l_arm.L_upperArm + l_arm.L_forearm + l_arm.L_hand;
    l_arm.L_cg =
        cog_Arm(m_arm.M_arm, m_arm.M_upperArm, m_arm.M_foreArm, m_arm.M_hand,
                l_arm.L_upperArm, l_arm.L_forearm, l_arm.L_hand);

    muscle_insertion musc;
    musc.L_pec = l_arm.L_upperArm * 0.175;
    musc.L_lat = l_arm.L_upperArm * 0.140;
    musc.A_pec = 20 * (M_PI / 180.0);
    musc.A_lat = 15 * (M_PI / 180.0);
    musc.PCSA_ratio = 0.73;

    float g = 9.81;
    float ring_force = (weight * g) / 2;
    float arm_weight = m_arm.M_arm * g;
    float ring_torque = ring_force * l_arm.L_arm;
    float arm_weight_torque = arm_weight * l_arm.L_cg;
    float total_external = ring_torque - arm_weight_torque;

    float F_pec =
        total_external / ((musc.L_pec * sin(musc.A_pec)) +
                          (musc.PCSA_ratio * (musc.L_lat * sin(musc.A_lat))));
    float F_lat = F_pec * musc.PCSA_ratio;

    float joint_horizontal =
        (F_pec * cos(musc.A_pec)) + (F_lat * cos(musc.A_lat));
    float joint_vertical = ring_force - arm_weight - (F_pec * sin(musc.A_pec)) -
                           (F_lat * sin(musc.A_lat));
    float deltoid_total =
        sqrt(pow(joint_horizontal, 2) + pow(joint_vertical, 2));

    // --- DRAW RESULTS WINDOW ---
    int res_y = form_y + 7;
    int res_h = 10;
    int res_w = 50;
    int res_x = (col - res_w) / 2;

    WINDOW *win = newwin(res_h, res_w, res_y, res_x);
    box(win, 0, 0);
    wbkgd(win, COLOR_PAIR(0)); // Standard background

    wattron(win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win, 0, 2, "[ RESULTS ]");
    wattroff(win, COLOR_PAIR(2) | A_BOLD);

    mvwprintw(win, 2, 2, "Ring Force:          %6.2f N", ring_force);
    mvwprintw(win, 3, 2, "External Torque:     %6.2f Nm", total_external);
    mvwprintw(win, 4, 2, "Horiz. Compression:  %6.2f N", joint_horizontal);
    mvwprintw(win, 5, 2, "Vertical Shear:      %6.2f N", joint_vertical);

    wattron(win, A_REVERSE);
    mvwprintw(win, 7, 2, "Deltoid Reaction:    %6.2f N", deltoid_total);
    wattroff(win, A_REVERSE);

    wrefresh(win);

    // --- FOOTER ---
    attron(COLOR_PAIR(1));
    mvprintw(row - 2, 2, "Press 'Q' to Quit, any other key to Reset");
    attroff(COLOR_PAIR(1));
    refresh();

    int cmd = getch();
    if (cmd == 'q' || cmd == 'Q')
      break;
  }

  // --- CLEANUP ---
  endwin();
  return 0;
}
