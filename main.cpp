#include <SFML/Graphics.hpp>
#include "Lab02.h"

int main(){
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Lab02");

    sf::Font font;
    if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) return -1;
    vector<ENTRY> txtFiles;

    vector<string> menu = {
        "1. Boot sector information.",
        "2. Search .txt files",
        "3. View .txt file information",
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
            case 1: txtFiles = searchFiles(usb, Bs, Bs.FAT32.rootCluster);
                if (txtFiles.empty()) {
                    cout << "No .txt files found in the Root Directory." << endl;
                }
                else {
                    cout << ".txt files founded\n";
                    for (int i = 0; i < txtFiles.size(); i++) {
                        cout << i + 1 << ". " << txtFiles[i].name << ".txt " << '\n';
                    }
                }
                cout << '\n';
                break;
            case 2: int choose;
                cout << "Choose file\n";
                cin >> choose;
                if (choose > txtFiles.size()) cout << "Invalid number\n";
                fileInfo(usb, Bs, txtFiles[choose - 1]);
                break;
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
