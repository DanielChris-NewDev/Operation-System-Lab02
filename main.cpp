#include <SFML/Graphics.hpp>
#include "Lab02.h"

int main(){
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Lab02");

    sf::Font font;
    if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) return -1;
    vector<ENTRY> txtFiles;

    vector<string> menu = {
        "1. Boot sector information.",
        "2. Search and handle files",
        "3. Exit."
    };

    while (window.isOpen()) {
        int choice = handleChoice(window, font, menu);

        switch (choice) {
            case 0:
            {
                bootSector Bs = readBootSector("\\\\.\\E:");
                printBootSectorInfo(window, font, Bs);
                break;
            }
            case 1: {
                HANDLE usb = CreateFile("\\\\.\\E:", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
                bootSector Bs = readBootSector("\\\\.\\E:");
                txtFiles = searchFiles(usb, Bs, Bs.FAT32.rootCluster);

                if (txtFiles.empty()) {
                    cout << "No .txt files found in the Root Directory." << endl;
                    break;
                }
                else {
                    cout << ".txt files founded\n";
                    for (int i = 0; i < txtFiles.size(); i++) {
                        cout << i + 1 << ". " << txtFiles[i].name << ".txt " << '\n';
                    }
                }
                
                int choose;
                cout << "Choose file\n";
                cin >> choose;

                if (choose > txtFiles.size()) cout << "Invalid number\n";

                fileInfo(usb, Bs, txtFiles[choose - 1]);
                char answer;
                cout << "\nDo you want to draw the CPU scheduling diagram for this file's content? (y/n): ";
                cin >> answer;

                if(answer == 'y' || answer == 'Y'){
                    while (const auto event = window.pollEvent()) {};
                    CPUScheduling(window, font, usb, Bs, txtFiles[choose - 1]);

                } else if (answer == 'n' || answer == 'N'){
                    cout << "Returning to menu\n";
                } else {
                    cout << "Invalid choice\n";
                }
            }

            case 2:
            case -1:
                window.close();
                return 0;
        }
    }

    return 0;
}
