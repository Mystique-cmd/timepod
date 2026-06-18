#include <iostream>
#include <string>

extern "C" {
#include "timer.h"
#include "io.h"
}

int main() {
    io_clear_screen();

    while (true) {
        io_print_domains();
        int choice = io_read_int_choice("\nEnter choice: ", 0, 6);
        if (choice == 0) {
            std::cout << "Exiting.\n";
            return 0;
        }

        DomainTimer dt{};
        switch (choice) {
            case 1: timer_init_hours(&dt, "The Portal", 7); break;
            case 2: timer_init_hours(&dt, "The Factory", 4); break;
            case 3: timer_init_hours(&dt, "Benjamin's Game", 4); break;
            case 4: timer_init_hours(&dt, "The Matrix Manual", 1); break;

            case 5: timer_init_hours(&dt, "The Rabbit Hole", 4); break;
            case 6: timer_init_hours(&dt, "Specter Spectacle", 4); break;
            default: continue;
        }

        std::cout << "\nStarting timer for: " << dt.name << "\n";
        timer_run_blocking(&dt);

        std::cout << "\nPress Enter to select another domain...";
        std::string dummy;
        std::getline(std::cin, dummy); /* consume */
        std::getline(std::cin, dummy);
        io_clear_screen();
    }
}

