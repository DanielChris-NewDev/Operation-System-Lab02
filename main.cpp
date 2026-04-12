#include <SFML/Graphics.hpp>
#include "Lab02.h"

int main(){
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Lab02");

    sf::Font font;
    if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) return -1;

    vector<string> menu = {
        "1. Boot sector information.",
        "2. ",
        "3. ",
        "4. CPU Scheduling diagram",
        "5. Exit."
    };

    while (window.isOpen()) {
        int choice = handleChoice(window, font, menu);

        switch (choice) {
            case 0:
                bootSector Bs;
                printBootSectorInfo(Bs);
                break;
            case 1: break;
            case 2: break;
            case 3:
                while (const auto event = window.pollEvent()) {}
                CPUScheduling(window, font, "input.txt");
                break;
            case 4:
            case -1:
                window.close();
                return 0;
        }
    }

    return 0;
}