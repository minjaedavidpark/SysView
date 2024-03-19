//
// Created by Min Jae David Park on 2024-03-19.
// Reference: https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
//
#include <ostream>

// Print colorful text to the terminal
// Usage: std::cout << Color::Modifier(Color::FG_RED) << "This is red" << Color::Modifier(Color::FG_DEFAULT) << std::endl;
namespace Color {
    enum Code {
        FG_BLACK = 30,
        FG_RED = 31,
        FG_GREEN = 32,
        FG_YELLOW = 33,
        FG_BLUE = 34,
        FG_MAGENTA = 35,
        FG_CYAN = 36,
        FG_LIGHT_GRAY = 37,
        FG_DEFAULT = 39,
        BG_RED = 41,
        BG_GREEN = 42,
        BG_BLUE = 44,
        BG_DEFAULT = 49,
        FG_DARK_GRAY = 90,
        FG_LIGHT_RED = 91,
        FG_LIGHT_GREEN = 92,
        FG_LIGHT_YELLOW = 93,
        FG_LIGHT_BLUE = 94,
        FG_LIGHT_MAGENTA = 95,
        FG_LIGHT_CYAN = 96,
        FG_WHITE = 97,
    };
    class Modifier {
        Code code;
    public:
        explicit Modifier(Code pCode) : code(pCode) {}
        friend std::ostream&
        operator<<(std::ostream& os, const Modifier& mod) {
            return os << "\033[" << mod.code << "m";
        }
    };
}