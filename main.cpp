#include <SFML/Graphics.hpp>
#include "Lab02.h"

int main(){
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Lab02");

    sf::Font font;
    if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) return -1;
    string driveLetter = "";
    bool driveSelected = false;

    while (window.isOpen() && !driveSelected) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            else if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
                char c = static_cast<char>(textEntered->unicode);
                
                if (std::isalpha(c)) {
                    driveLetter = c;
                }
                else if (c == '\r' || c == '\n') {
                    if (!driveLetter.empty()) driveSelected = true;
                }
            }
        }

        window.clear();
        sf::Text prompt(font, "Enter USB Drive Letter: " + driveLetter, 30);
        prompt.setPosition({100, 250});
        window.draw(prompt);
        window.display();
    }

    string drivePath = "\\\\.\\" + driveLetter + ":";
    
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
                bootSector Bs = readBootSector(drivePath.c_str());
                printBootSectorInfo(window, font, Bs);
                break;
            }
            case 1: {
                HANDLE usb = CreateFile(drivePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
                bootSector Bs = readBootSector(drivePath.c_str());
                txtFiles = searchFiles(usb, Bs, Bs.FAT32.rootCluster);

                if (txtFiles.empty()) {
                    vector<string> errorMsg = { "No .txt files found" };
                    handleChoice(window, font, errorMsg);
                    break;
                }
                else {
                    vector<string> fileNames;
                    for (int i = 0; i < txtFiles.size(); i++) {
                        string entry = std::to_string(i + 1) + ". " + txtFiles[i].name + ".txt";
                        fileNames.push_back(entry);
                    }

                    int fileChoice = handleChoice(window, font, fileNames);

                    if (fileChoice != -1) { 
                        fileInfo(window, font, usb, Bs, txtFiles[fileChoice]);

                        vector<string> yesNo = { "Do you want to print CPU diagram?", "Yes", "No" };
                        int drawChoice = handleChoice(window, font, yesNo);

                        if (drawChoice == 1) {
                            CPUScheduling(window, font, usb, Bs, txtFiles[fileChoice]);
                        }
                    }
                    break;
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
