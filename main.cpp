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
#include <algorithm>
#include <map>
#include <memory>
#include <array>
#include <cstdio>


const bool NUMBER[10][15] = {
    {1,1,1,1,0,1,1,0,1,1,0,1,1,1,1}, // 0
    {0,0,1,0,0,1,0,0,1,0,0,1,0,0,1}, // 1
    {1,1,1,0,0,1,1,1,1,1,0,0,1,1,1}, // 2
    {1,1,1,0,0,1,1,1,1,0,0,1,1,1,1}, // 3
    {1,0,1,1,0,1,1,1,1,0,0,1,0,0,1}, // 4
    {1,1,1,1,0,0,1,1,1,0,0,1,1,1,1}, // 5
    {1,1,1,1,0,0,1,1,1,1,0,1,1,1,1}, // 6
    {1,1,1,0,0,1,0,0,1,0,0,1,0,0,1}, // 7
    {1,1,1,1,0,1,1,1,1,1,0,1,1,1,1}, // 8
    {1,1,1,1,0,1,1,1,1,0,0,1,1,1,1}  // 9
};




std::string make_time_line(int hour, int min, int sec, int row,
                           const std::string& bg_color, bool show_seconds) {
    int h1 = hour / 10, h2 = hour % 10;
    int m1 = min / 10, m2 = min % 10;

    auto pixel = [&](bool on) -> std::string {
        return on ? bg_color + "  " : "\033[0m  ";
    };

    auto digit = [&](int n) -> std::string {
        return pixel(NUMBER[n][row * 3 + 0])
             + pixel(NUMBER[n][row * 3 + 1])
             + pixel(NUMBER[n][row * 3 + 2]);
    };

    auto gap = []() -> std::string { return "\033[0m  "; };

    auto colon = [&](int r) -> std::string {
        return (r == 1 || r == 3) ? bg_color + "  " : "\033[0m  ";
    };

    std::string line = digit(h1) + gap() + digit(h2) + gap() + colon(row) + gap()
                     + digit(m1) + gap() + digit(m2);

    if (show_seconds) {
        int s1 = sec / 10, s2 = sec % 10;
        line += gap() + colon(row) + gap() + digit(s1) + gap() + digit(s2);
    }

    return line + "\033[0m";
}


std::string get_accent_color() {
    std::array<char, 128> buffer;
    std::string result;

    FILE* pipe = popen("python3 get_accent_color.py", "r");
    if (!pipe) return "37";

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    pclose(pipe);

    // Trim whitespace
    result.erase(0, result.find_first_not_of(" \n\r\t"));
    result.erase(result.find_last_not_of(" \n\r\t") + 1);

    return result.empty() ? "37" : result;
}
   

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
    return {24, 80}; // fall
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


int main(int argc, char** argv) {
    // Parse arguments
    bool show_seconds = false;
    bool color_set = false;
    std::string clock_color = "37";

    std::map<std::string, std::string> color_names = {
        {"red", "31"}, {"green", "32"}, {"yellow", "33"}, {"blue", "34"},
        {"magenta", "35"}, {"cyan", "36"}, {"white", "37"}, {"black", "30"}
    };

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-s") {
            show_seconds = true;
        } else if (arg == "-c" && i + 1 < argc) {
            std::string val = argv[++i];
            if (color_names.count(val)) {
                clock_color = color_names[val];
            } else {
                clock_color = val;
            }
            color_set = true;
        }
    }

    if (!color_set) {
        clock_color = get_accent_color();
    }

    int bg_code = std::stoi(clock_color) + 10;
    std::string bg_color = "\033[" + std::to_string(bg_code) + "m";

    disable_keyboard_input();
    std::cout << "\033[?25l";

    while (true) {
        std::time_t now = std::time(nullptr);
        std::tm* local = std::localtime(&now);

        int hour = local->tm_hour;
        int min = local->tm_min;
        int sec = local->tm_sec;

        // Day of week (abbreviated, uppercase)
        char day_abbr[4];
        std::strftime(day_abbr, sizeof(day_abbr), "%a", local);
        std::string day_str = day_abbr;
        std::transform(day_str.begin(), day_str.end(), day_str.begin(), ::toupper);

        const int clock_height = 5;
        int time_width = show_seconds ? 54 : 32;

        auto ws = get_terminal_size();

        // 5 rows time + 1 row gap + 1 row day = 7 total
        int time_row = std::max(1, (ws.rows - 7) / 2 + 1);
        int time_col = std::max(1, (ws.cols - time_width) / 2 + 1);

        int day_row = time_row + 6;
        int day_col = std::max(1, (ws.cols - static_cast<int>(day_str.size())) / 2 + 1);

        std::cout << "\033[2J\033[H";

        // Draw time
        for (int row = 0; row < clock_height; ++row) {
            std::cout << "\033[" << (time_row + row) << ";" << time_col << "H"
                      << make_time_line(hour, min, sec, row, bg_color, show_seconds);
        }

        // Draw day below
        std::cout << "\033[" << day_row << ";" << day_col << "H"
                  << "\033[" << clock_color << "m" << day_str << "\033[0m";

        std::cout << std::flush;
        drain_input();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
