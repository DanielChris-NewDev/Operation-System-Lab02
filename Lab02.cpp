#include "Lab02.h"

void consoleWindow() {
	HWND consoleWindow = GetConsoleWindow();
	LONG style = GetWindowLong(consoleWindow, GWL_STYLE);
	style = style & ~(WS_MAXIMIZEBOX) & ~(WS_THICKFRAME);
	SetWindowLong(consoleWindow, GWL_STYLE, style);
}


bootSector readBootSector(const char* path){
     //Truy cập usb
    HANDLE disk = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if(disk == INVALID_HANDLE_VALUE){
        cout << "Cannot open drive";
    } 

    //Vùng nhớ để lưu các byte được đọc từ usb
    vector<BYTE> buffer(512);

    //Số byte đã đọc
    DWORD numOfBytesRead;

    // Đọc 512 byte đầu tiên của usb
    ReadFile(disk, buffer.data(), 512, &numOfBytesRead, NULL);
    CloseHandle(disk);

    //Chép 512 byte đã đọc từ usb sang struct
    bootSector bs;
    memcpy(&bs, buffer.data(), sizeof(bootSector));

    return bs;
}

void printBootSectorInfo(bootSector bs){
    cout << "\n-------------------------------------------------------\n";
    cout << "|             Boot Sector Information                   |\n";
    cout << "-------------------------------------------------------\n";
    cout << left;
    cout << "| " << setw(35) << "Bytes per sector" << "| " << setw(15) << bs.bytesPerSector << "(bytes)|" << endl;
    cout << "| " << setw(35) << "Sectors per cluster" << "| " << setw(15) << (int)bs.sectorsPerCluster << " |" << endl;
    cout << "| " << setw(35) << "Number of sectors in the Boot Sector region" << "| " << setw(15) << bs.reservedSectorNum << " |" << endl;
    cout << "| " << setw(35) << "Number of FAT tables" << "| " << setw(15) << (int)bs.numOfFATTable << " |" << endl;
    cout << "| " << setw(35) << "Number of sectors per FAT table" << "| " << setw(15) << bs.FAT32.tableSize_32 << " |" << endl;
    int rdetSectors = (bs.bytesPerSector != 0) ? (bs.numOfRootEntry * 32) / bs.bytesPerSector : 0;
    cout << "| " << setw(35) << "Number of sectors for the RDET" << "| " << setw(15) << rdetSectors << " |" << endl;
    cout << "| " << setw(35) << "Total number of sectors on the disk" << "| " << setw(15) << bs.totalSectors32 << " |" << endl;
    cout << "-------------------------------------------------------\n";
}

int handleChoice(sf::RenderWindow& window, sf::Font& font, const vector<string>& choices) {
    int choice = 0;
    int n = choices.size();

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                return -1;
            }
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::W || key->code == sf::Keyboard::Key::Up)
                    choice = (choice - 1 + n) % n;
                else if (key->code == sf::Keyboard::Key::S || key->code == sf::Keyboard::Key::Down)
                    choice = (choice + 1) % n;
                else if (key->code == sf::Keyboard::Key::Enter)
                    return choice;
                else if (key->code == sf::Keyboard::Key::Escape)
                    return -1;
            }
        }

        window.clear(sf::Color::Black);

        for (int i = 0; i < n; i++) {
            sf::Text text(font, (i == choice ? ">> " : "   ") + choices[i], 24);
            text.setPosition({50.f, 50.f + i * 40.f});
            text.setFillColor(i == choice ? sf::Color::Yellow : sf::Color::White);
            window.draw(text);
        }

        window.display();
    }

    return -1;
}


void CPUScheduling(sf::RenderWindow& window, sf::Font& font, const string& path){
    vector<Queue> queues;
    vector<Process> processes;
    vector<Gantt> gantt;

    readFile(path, queues, processes);
    runQueue(processes, queues, gantt);

    int runningTime = 1;
    if (!gantt.empty()) {
        runningTime = gantt.back().end;
    }

    vector<string> qid;
    for(int i = 0; i < queues.size(); i++){
        qid.push_back(queues[i].QID);
    }

    float left        = 120.f;
    float top         = 80.f;
    float rHeight     = 60.f;
    float chartWidth  = 650.f;
    float blockHeight = 40.f;

    while(window.isOpen()){
        while(const auto event = window.pollEvent()){
            if(event->is<sf::Event::Closed>()){
                window.close();
                return;
            }
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()){
                if(key->code == sf::Keyboard::Key::Escape){
                    return;
                }
            }
        }

        window.clear(sf::Color(30,30,30));

        for(int j = 0; j < qid.size(); j++){
            float row = top + j * rHeight;

            sf::Text qLabel(font, qid[j], 16);
            qLabel.setPosition({10.f, row + 10.f});
            qLabel.setFillColor(sf::Color::White);
            window.draw(qLabel);

            for (int z = 0; z < gantt.size(); z++) {
                if (gantt[z].queueID != qid[j]) continue;

                float scale = chartWidth / runningTime;
                float x = left + gantt[z].start * scale;
                float w = (gantt[z].end - gantt[z].start) * scale;

                sf::RectangleShape rect({w - 2.f, blockHeight});
                rect.setPosition({x + 1.f, row});
                rect.setFillColor(sf::Color(70, 130, 180));
                rect.setOutlineColor(sf::Color::White);
                rect.setOutlineThickness(1.f);
                window.draw(rect);

                sf::Text pid(font, gantt[z].PID, 13);
                pid.setPosition({x + 4.f, row + 12.f});
                pid.setFillColor(sf::Color::White);
                window.draw(pid);

                sf::Text tStart(font, to_string(gantt[z].start), 11);
                tStart.setPosition({x, row + blockHeight + 4.f});
                tStart.setFillColor(sf::Color(200, 200, 200));
                window.draw(tStart);
            }

            for (int z = gantt.size() - 1; z >= 0; z--) {
                if (gantt[z].queueID == qid[j]) {
                    float scale = chartWidth / runningTime;
                    float x = left + gantt[z].end * scale;
                    sf::Text tEnd(font, to_string(gantt[z].end), 11);
                    tEnd.setPosition({x, row + blockHeight + 4.f});
                    tEnd.setFillColor(sf::Color(200, 200, 200));
                    window.draw(tEnd);
                    break;
                }
            }
        }

        window.display();  
    }
}
    
