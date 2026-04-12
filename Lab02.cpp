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

void printBootSectorInfo(sf::RenderWindow& window, sf::Font& font, bootSector bs) {
    stringstream ss;
    ss << "\n--------------------------------------------------------\n";
    ss << "|               Boot Sector Information                |\n";
    ss << "------------------------------------+-------------------\n";
    ss << left;
    ss << "| " << setw(35) << "Bytes per sector" << "| " << setw(9) << bs.bytesPerSector << "(bytes)|" << endl;
    ss << "| " << setw(35) << "Sectors per cluster" << "| " << setw(15) << (int)bs.sectorsPerCluster << " |" << endl;
    ss << "| " << setw(35) << "Number of sectors in the Boot Sector region" << "| " << setw(7) << bs.reservedSectorNum << " |" << endl;
    ss << "| " << setw(35) << "Number of FAT tables" << "| " << setw(15) << (int)bs.numOfFATTable << " |" << endl;
    ss << "| " << setw(35) << "Number of sectors per FAT table" << "| " << setw(15) << bs.FAT32.tableSize_32 << " |" << endl;
    ss << "| " << setw(35) << "Number of sectors for the RDET" << "| " << setw(15) << (bs.numOfRootEntry * 32) / bs.bytesPerSector << " |" << endl;
    ss << "| " << setw(35) << "Total number of sectors on the disk" << "| " << setw(15) << bs.totalSectors32 << " |" << endl;
    ss << "--------------------------------------------------------\n";
    ss << "Press Enter or Esc to exit\n";

    sf::Text text(font, ss.str(), 18);
    text.setPosition({30.f, 30.f});

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                return;
            }
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape || key->code == sf::Keyboard::Key::Enter) {
                    return; 
                }
            }
        }

        window.clear(sf::Color::Black);
        window.draw(text);
        window.display();
    }
}

vector<ENTRY> searchFiles(HANDLE usb, const bootSector& bs, UINT32 startCluster) {
    vector<ENTRY> txtfiles;

    DWORD dataSection = (bs.reservedSectorNum + bs.numOfFATTable * bs.FAT32.tableSize_32) * bs.bytesPerSector;
    //Skip boot sector and FAT, bắt đầu ở byte đầu tiên của vùng data 

    DWORD firstCluster = (startCluster - 2) * bs.sectorsPerCluster * bs.bytesPerSector;
    //Cluster đầu tiên (cluster thứ 2) của vùng data (bỏ cluster 0 và 1)

    DWORD startByte = dataSection + firstCluster;

    SetFilePointer(usb, startByte, NULL, FILE_BEGIN);
    // Bắt đầu ở sector đầu tiên

    for (int s = 0; s < bs.sectorsPerCluster; s++) {
        BYTE sector[512]; //các byte được đọc
        DWORD bytes_read; // các byte đã đọc

        if (ReadFile(usb, sector, 512, &bytes_read, NULL)) {// Bat91 đầu đọc data
            for (int i = 0; i < 512; i += 32) {
                if (sector[i] == 0x00) return txtfiles; //rỗng
                if (sector[i] == 0xE5) continue; //file bị xóa

                string dataType = "";
                dataType += sector[i + 8]; //0x08 là byte thứ 9
                dataType += sector[i + 9];
                dataType += sector[i + 10];

                if (dataType == "TXT") {
                    ENTRY file;
                    char* temp = (char*)&sector[i]; // trỏ tới điểm đầu của entry
                    file.name = string(temp, 8); //tên file là 8 byte đầu tiên

                    memcpy(&file.fileSize, &sector[i + 28], 4);// 0x1C là byte thứ 29

                    UINT16 date;
                    UINT32 time;
                    memcpy(&date, &sector[i + 16], 2); // 0x10 là byte thứ 17
                    //Đọc từ phải sang trái thay vì sử dụng endian
                    int day = date & 0x1F; //Chỉ đọc 5 bit cuối (chuyển bit khác thành 0)
                    int month = (date >> 5) & 0x0F; // Chỉ đọc 4 bit cuối
                    int year = (date >> 9) + 1980; //Đọc 7 bit còn lại
                    file.dateCreated = to_string(day) + "/" + to_string(month) + "/" + to_string(year);

                    memcpy(&time, &sector[i + 13], 3); // 0x0E là byte thứ 14
                    int milisecond = time & 0x7F; //đọc 7 bit
                    int second = (time >> 7) & 0x3F;//đọc 6 bit
                    int minute = (time >> 13) & 0x3F; //đọc 6 bit
                    int hour = (time >> 19) & 0x1F; //đọc 5 bit
                    file.timeCreated = to_string(hour) + ":" + to_string(minute) + ":" + to_string(second) + "." + to_string(milisecond);

                    UINT16 highByte;
                    memcpy(&highByte, &sector[i + 20], 2); //0x14 là byte thứ 21
                    UINT16 lowByte;
                    memcpy(&lowByte, &sector[i + 26], 2); // 0x1A là byte thứ 27

                    file.startCluster = (highByte << 16) | lowByte; // Little endian

                    txtfiles.push_back(file);
                }
                //Kiểm tra subfolders
                if (sector[i + 11] & 0x10) { //Lấy bit thứ 4 của offset 0x0B
                    if (sector[i] == '.') continue;

                    UINT16 highByte;
                    memcpy(&highByte, &sector[i + 20], 2);
                    UINT16 lowByte;
                    memcpy(&lowByte, &sector[i + 26], 2);

                    UINT32 subFolderCluster = (highByte << 16) | lowByte;
                    vector<ENTRY> subFolder = searchFiles(usb, bs, subFolderCluster);
                    for (const auto& file : subFolder) {
                        txtfiles.push_back(file);
                    }
                    SetFilePointer(usb, startByte + (s * 512) + (i + 32), NULL, FILE_BEGIN);
                    //trỏ tới sector của entry tiếp theo
                }
            }
        }
    }
    //CloseHandle(usb);
    return txtfiles;
}

string fileContent(HANDLE usb, const bootSector& bs, const ENTRY& file) {
    DWORD dataSection = (bs.reservedSectorNum + bs.numOfFATTable * bs.FAT32.tableSize_32) * bs.bytesPerSector;
    DWORD firstCluster = (file.startCluster - 2) * bs.sectorsPerCluster * bs.bytesPerSector;
    // Cluster bắt đầu của file

    DWORD startByte = dataSection + firstCluster;
    SetFilePointer(usb, startByte, NULL, FILE_BEGIN);

    // Bắt đầu ở sector đầu tiên
    BYTE buffer[512];
    DWORD bytes_read; // các byte đã đọc

    if (!ReadFile(usb, buffer, 512, &bytes_read, NULL)) { //bắt đầu từ phần tử/chữ đầu tiên trong file
        cout << "Unable to read file's information\n";
        return "";
    }

    return string((char*)buffer, file.fileSize);
}

void readFileContent(string file, vector<Queue>& queues, vector<Process>& processes) {
    stringstream ss(file);
    int queueAmount;

    ss >> queueAmount;

    for (int i = 0; i < queueAmount; i++) {
        Queue q;
        ss >> q.QID >> q.timeSlice >> q.schedulingPolicy;
        queues.push_back(q);
    }

    Process p;

    while (ss >> p.PID >> p.arrivalTime >> p.burstTime >> p.QueueID) {
        p.remainingTime = p.burstTime;
        processes.push_back(p);
    }
}

void printProcessTable(vector<Queue>& queues, vector<Process>& processes){
    cout << left << setw(20) << " ";
    for (const auto& process : processes) cout << setw(15) << process.PID;
    cout << endl;

    cout << left << setw(20) << "Arrival time";
    for (const auto& process : processes) cout << setw(15) << process.arrivalTime;
    cout << endl;

    cout << left << setw(20) << "Burst time";
    for (const auto& process : processes) cout << setw(15) << process.burstTime;
    cout << endl;

    cout << left << setw(20) << "Queue ID";
    for (const auto& process : processes) cout << setw(15) << process.QueueID;
    cout << endl;

    cout << left << setw(20) << "Time slice";
    for (const auto& process : processes) {
        int slice = 0;
        for (const auto& q : queues) if (q.QID == process.QueueID) slice = q.timeSlice;
        cout << setw(15) << slice;
    }
    cout << endl;

    cout << left << setw(20) << "Algorithm";
    for (const auto& process : processes) {
        string algorithm = "";
        for (const auto& q : queues) if (q.QID == process.QueueID) algorithm = q.schedulingPolicy;
        cout << setw(15) << algorithm;
    }
}

void fileInfo(HANDLE usb, const bootSector& bs, const ENTRY& file) {

    cout << "File name: " << file.name << ".txt" << '\n'
        << "Date created: " << file.dateCreated << '\n'
        << "Time created: " << file.timeCreated << '\n'
        << "Size: " << file.fileSize << " Bytes" << '\n';
    cout << "== Process Information Table ==\n";
    string content = fileContent(usb, bs, file);
    if (content.empty()) {
        cout << "File is empty\n";
    }

    vector<Queue> queues;
    vector<Process> processes;
    readFileContent(content, queues, processes);
    printProcessTable(queues, processes);
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
            sf::Text text(font);
            text.setCharacterSize(24);
            text.setPosition({50.f, 50.f + i * 40.f});

            if(i == choice){
                text.setString(">>" + choices[i]);
                text.setFillColor(sf::Color::Blue);
            } else{
                text.setString(" " + choices[i]);
                text.setFillColor(sf::Color::White);
            }
        }

        window.display();
    }

    return -1;
}


void CPUScheduling(sf::RenderWindow& window, sf::Font& font, HANDLE usb, const bootSector& bs, const ENTRY& file){
    vector<Queue> queues;
    vector<Process> processes;
    vector<Gantt> gantt;

    string content = fileContent(usb, bs, file);
    readFileContent(content, queues, processes);
    runQueue(processes, queues, gantt);
    printProcessStatistic(window, font, processes);

    //Tính tổng thời gian chạy
    int runningTime = 1;
    for (int i = 0; i < gantt.size(); i++) {
        if (gantt[i].end > runningTime) {
            runningTime = gantt[i].end;
        }
    }
    // Tạo mảng chứa queue id
    vector<string> qid;
    for(int i = 0; i < queues.size(); i++){
        qid.push_back(queues[i].QID);
    }

    float leftEdge = 30.f;
    float top = 80.f;
    float rHeight = 60.f;
    float chartWidth = 750.f;
    float blockHeight = 40.f;

    while(window.isOpen()){
        while(const auto event = window.pollEvent()){
            //Nhấn dấu x thì tắt luôn console
            if(event->is<sf::Event::Closed>()){
                window.close();
                return;
            }
            //Nhấn Esc để quay về menu
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()){
                if(key->code == sf::Keyboard::Key::Escape){
                    return;
                }
            }
        }

        window.clear(sf::Color::Black);

        //Duyệt qua từng hàng đợi
        for(int j = 0; j < qid.size(); j++){
            float row = top + j * rHeight;

            //Vẽ tên hàng đợi 
            sf::Text queueName(font, qid[j], 16);
            queueName.setPosition({10.f, row + 10.f});
            queueName.setFillColor(sf::Color::White);
            window.draw(queueName);

            for (int z = 0; z < gantt.size(); z++) {
                //Nếu khối không thuộc hàng đợi thì bỏ qua
                if (gantt[z].queueID != qid[j]) {
                    continue;
                }
                float scale = chartWidth / runningTime;
                float x = leftEdge + gantt[z].start * scale;
                float width = (gantt[z].end - gantt[z].start) * scale;

                //Vẽ khối chữ nhật
                sf::RectangleShape rect({width - 2.f, blockHeight});
                rect.setPosition({x + 1.f, row});
                rect.setFillColor(sf::Color(70, 130, 180));
                rect.setOutlineColor(sf::Color::White);
                rect.setOutlineThickness(1.f);
                window.draw(rect);
                
                //Vẽ tên process vào khối rectangle
                sf::Text pid(font, gantt[z].PID, 13);
                pid.setPosition({x + 4.f, row + 12.f});
                pid.setFillColor(sf::Color::White);
                window.draw(pid);

                //Vẽ mốc thời gian chạy và kết thúc của khối
                sf::Text tStart(font, to_string(gantt[z].start), 11);
                tStart.setPosition({x, row + blockHeight + 4.f});
                tStart.setFillColor(sf::Color(200, 200, 200));
                window.draw(tStart);
            }

            //Ghi mốc thời gian kết thúc cho mốc cuối cùng
            for (int z = gantt.size() - 1; z >= 0; z--) {
                if (gantt[z].queueID == qid[j]) {
                    float scale = chartWidth / runningTime;
                    float x = leftEdge + gantt[z].end * scale;
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

void printProcessStatistic(sf::RenderWindow& window, sf::Font& font, const vector<Process>& processes){
    stringstream ss;
    ss << "\n================ PROCESS STATISTICS ================\n";
    ss << left << setw(12) << "\nProcess" << setw(12) << "Arrival" << setw(10) << "Burst" << setw(15) << "Completion" << setw(15) << "Turnaround" << setw(10) << "Waiting" << '\n';
    ss << "---------------------------------------------------------------------------------\n";

    double averageTurnaroundTime = 0.0;
    double averageWaitingTime = 0.0;
    for (int i = 0; i < processes.size(); i++) {
        const Process& p = processes[i];
        averageTurnaroundTime += p.turnaroundTime;
        averageWaitingTime += p.waitingTime;

        ss << left << setw(12) << p.PID << setw(12) << p.arrivalTime << setw(13) << p.burstTime << setw(15) << p.completionTime << setw(13) << p.turnaroundTime << setw(10) << p.waitingTime << '\n';
    }
    ss << "---------------------------------------------------------------------------------\n";
    ss << "\nAverage Turnaround Time : " << averageTurnaroundTime / processes.size();
    ss << "\nAverage Waiting Time : " << averageWaitingTime / processes.size() << '\n';
    ss << "\n============================================================\n";

    sf::Text text(font, ss.str(), 18);
    text.setPosition({30.f, 30.f});

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                return;
            }
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape || key->code == sf::Keyboard::Key::Enter) {
                    return; 
                }
            }
        }

        window.clear(sf::Color::Black);
        window.draw(text);
        window.display();
    }
}
    
