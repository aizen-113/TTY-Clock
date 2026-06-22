//YOHOHOHOHO this is my first project
//took some help




#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <vector>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <csignal>
#include <cstdlib>


const std::vector<std::string> DIGITS[10] = {
    // 0
    {"██████",
     "██  ██",
     "██  ██",
     "██  ██",
     "██████"},
    // 1
    {"    ██",
     "    ██",
     "    ██",
     "    ██",
     "    ██"},
    // 2
    {"██████",
     "    ██",
     "██████",
     "██    ",
     "██████"},
    // 3
    {"██████",
     "    ██",
     "██████",
     "    ██",
     "██████"},
    // 4
    {"██  ██",
     "██  ██",
     "██████",
     "    ██",
     "    ██"},
    // 5
    {"██████",
     "██    ",
     "██████",
     "    ██",
     "██████"},
    // 6
    {"██████",
     "██    ",
     "██████",
     "██  ██",
     "██████"},
    // 7
    {"██████",
     "    ██",
     "    ██",
     "    ██",
     "    ██"},
    // 8
    {"██████",
     "██  ██",
     "██████",
     "██  ██",
     "██████"},
    // 9
    {"██████",
     "██  ██",
     "██████",
     "    ██",
     "██████"}
};

const std::vector<std::string> COLON = {
    "  ",
    "██",
    "  ",
    "██",
    "  "
};

const std::vector<std::string> SPACE = {
    "  ",
    "  ",
    "  ",
    "  ",
    "  "
};
   

struct Winsize {
    int rows;
    int cols;
};

// Terminal raw-mode stuff
static termios orig_termios;

Winsize get_terminal_size() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        return {ws.ws_row, ws.ws_col};
    }
    return {24, 80}; // fallback
}

std::string make_time_line(int hour, int min, int sec, int row) {
    int h1 = hour / 10;
    int h2 = hour % 10;
    int m1 = min / 10;
    int m2 = min % 10;
    int s1 = sec / 10;
    int s2 = sec % 10;

    return DIGITS[h1][row] + SPACE[row] + DIGITS[h2][row]
         + SPACE[row] + COLON[row] + SPACE[row]
         + DIGITS[m1][row] + SPACE[row] + DIGITS[m2][row]
         + SPACE[row] + COLON[row] + SPACE[row]
         + DIGITS[s1][row] + SPACE[row] + DIGITS[s2][row];
}

void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    std::cout << "\033[?25h" << std::flush; // show cursor again
}

void signal_handler(int) {
    restore_terminal();
    std::exit(0);
}

void disable_keyboard_input() {
    // Save original terminal settings
    if (tcgetattr(STDIN_FILENO, &orig_termios) != 0) return;

    // Make sure terminal is restored on exit or Ctrl+C
    std::atexit(restore_terminal);
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Modify: disable canonical mode and echo
    termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;   // return immediately even if 0 chars
    raw.c_cc[VTIME] = 0;  // no timeout
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void drain_input() {
    char buf;
    // Read and discard any keys the user pressed while clock was running
    while (read(STDIN_FILENO, &buf, 1) > 0) {
        // discard
    }
}

int main() {
    disable_keyboard_input();

    // Hide cursor
    std::cout << "\033[?25l";

    // Clear screen once
    std::cout << "\033[2J\033[H";

    while (true) {
        std::time_t now = std::time(nullptr);
        std::tm* local = std::localtime(&now);

        int hour = local->tm_hour;
        int min  = local->tm_min;
        int sec  = local->tm_sec;

        // Build date string
        std::ostringstream date;
        date << std::setfill('0')
             << std::setw(2) << local->tm_mday << "/"
             << std::setw(2) << (local->tm_mon + 1) << "/"
             << (local->tm_year + 1900);

        // Get terminal size
        auto ws = get_terminal_size();

        // The ASCII clock is 5 rows tall
        const int clock_height = 5;
        const int clock_width = 55; 

        int start_row = std::max(1, (ws.rows - clock_height - 2) / 2);
        int start_col = (ws.cols - clock_width) / 2 + 1;

        // Clear screen and move cursor to top-left
        std::cout << "\033[2J\033[H";

        // Move to date position
        int date_col = std::max(1, (ws.cols - static_cast<int>(date.str().size())) / 2);
        std::cout << "\033[" << start_row << ";" << date_col << "H"
                  << "\033[36m" << date.str() << "\033[0m";

        // Print the big digits below the date
        for (int row = 0; row < clock_height; ++row) {
            std::cout << "\033[" << (start_row + 2 + row) << ";" << start_col << "H"
                      << "\033[32m" << make_time_line(hour, min, sec, row) << "\033[0m";
        }

        std::cout << std::flush;

        // Throw away any keystrokes typed during this second
        drain_input();

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // restore_terminal() is called automatically via atexit()
    return 0;
}
