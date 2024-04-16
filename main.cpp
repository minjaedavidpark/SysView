#include <iostream>
#include <unistd.h>
#include <termios.h>
#include "colormod.h"
#include <cstdlib>
#include <cctype>
#include <cstdio>
#include <mach/mach_init.h>
#include <mach/mach_error.h>
#include <mach/mach_host.h>
#include <mach/vm_map.h>
#include <cmath>
#include <iomanip>


#define CTRL_KEY(k) ((k) & 0x1f)

// Reference: https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
struct termios orig_termios;

struct editorConfig {
    struct termios orig_termios;
};
struct editorConfig E;

void die(const char *s) {
    // Clear the screen upon exit
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

// Disable raw mode
void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}

// Enable raw mode
void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);
    struct termios raw = E.orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

char editorReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

/*** input ***/
void processKeypress() {
    char c = editorReadKey();
    switch (c) {
        case ('q'):
            // Clear the screen upon exit
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    }
}

void editorDrawRows() {
    int y;
    for (y = 0; y < 24; y++) {
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}

void refreshScreen() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}

void writeQuitMessage() {
    std::cout << Color::Modifier(Color::FG_BLUE) << "Press Q to exit" << Color::Modifier(Color::FG_DEFAULT) << std::endl;
}




static unsigned long long previousTotalTicks = 0;
static unsigned long long previousIdleTicks = 0;

float calculateCpuLoad(unsigned long long idleTicks, unsigned long long totalTicks)
{
    unsigned long long totalTicksSinceLastTime = totalTicks-previousTotalTicks;
    unsigned long long idleTicksSinceLastTime  = idleTicks-previousIdleTicks;
    float ret = 1.0f-((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime)/totalTicksSinceLastTime : 0);
    previousTotalTicks = totalTicks;
    previousIdleTicks  = idleTicks;
    return ret;
}

// Reference: https://stackoverflow.com/questions/8736713/retrieve-system-information-on-mac-os-x
// Returns 1.0f for "CPU fully pinned", 0.0f for "CPU idle", or somewhere in between
// You'll need to call this at regular intervals, since it measures the load between
// the previous call and the current one.
float getCpuLoad()
{
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuinfo, &count) == KERN_SUCCESS)
    {
        unsigned long long totalTicks = 0;
        for (int i=0; i<CPU_STATE_MAX; i++)
            totalTicks += cpuinfo.cpu_ticks[i];
        return calculateCpuLoad(cpuinfo.cpu_ticks[CPU_STATE_IDLE], totalTicks);
    }
    else
        return -1.0f;
}

static double parseMemValue(const char * b)
{
    while((*b)&&(isdigit(*b) == false))
        b++;
    return isdigit(*b) ? atof(b) : -1.0;
}

// Returns a number between 0.0f and 1.0f, with 0.0f meaning all RAM is available, and 1.0f meaning all RAM is currently in use
float getSystemMemoryUsagePercentage()
{
    FILE * fpIn = popen("/usr/bin/vm_stat", "r");
    if (fpIn)
    {
        double pagesUsed = 0.0, totalPages = 0.0;
        char buf[512];
        while(fgets(buf, sizeof(buf), fpIn) != NULL)
        {
            if (strncmp(buf, "Pages", 5) == 0)
            {
                double val = parseMemValue(buf);
                if (val >= 0.0)
                {
                    if ((strncmp(buf, "Pages wired",  11) == 0) ||
                        (strncmp(buf, "Pages active", 12) == 0)
                            )

                        pagesUsed += val;

                    totalPages += val;
                }
            }
            else
            if (strncmp(buf, "Mach Virtual Memory Statistics", 30) != 0)
                break; // Stop at "Translation Faults". We don't care
            // about anything at or below that
        }
        pclose(fpIn);

        if (totalPages > 0.0)
            return (float) (pagesUsed/totalPages);
    }
    return -1.0f;  // Indicate failureq
}

void drawStatusBar(float num) {
    float percentage = num * 10;
    int roundedPercentage = (int) std::round(percentage);

    for (int i = 0; i < 10; i++) {
        if (i < roundedPercentage) {
            std::cout << Color::Modifier(Color::FG_GREEN) << "█" << Color::Modifier(Color::FG_DEFAULT);
        } else {
            std::cout << Color::Modifier(Color::FG_RED) << "█" << Color::Modifier(Color::FG_DEFAULT);
        }
    }

    std::cout << " ";
}


void writeSysInfo() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::cout << "Current time: " << std::ctime(&now_time);

    float cpuLoad = getCpuLoad();
    std::cout << Color::Modifier(Color::FG_GREEN) << "CPU Usage: ";
    drawStatusBar(cpuLoad);
    std::cout << std::setprecision(4) << cpuLoad * 100 << '%' << Color::Modifier(Color::FG_DEFAULT) << std::endl;


    float memUsage = getSystemMemoryUsagePercentage();
    std::cout << Color::Modifier(Color::FG_RED) << "Memory Usage: ";
    drawStatusBar(memUsage);
    std::cout << memUsage * 100 << '%' << Color::Modifier(Color::FG_DEFAULT) << std::endl;

}



int main() {
    enableRawMode();

    // Clear the screen
    refreshScreen();
    writeSysInfo();
    writeQuitMessage();

    const int FPS = 2; // Frames per second
    const std::chrono::milliseconds frameDuration(1000 / FPS);
    auto lastFrameTime = std::chrono::steady_clock::now();

    // User input
    while (1) {
        // Check if input is available
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        int ready = select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &timeout);

        // Refresh screen at fixed FPS
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastFrameTime);
        if (elapsedTime >= frameDuration) {
            refreshScreen();
            writeSysInfo();
            writeQuitMessage();
            lastFrameTime = currentTime;
        }

        // Process user input if available
        if (ready > 0 && FD_ISSET(STDIN_FILENO, &read_fds)) {
            processKeypress();
        }
    }

    return 0;
}
