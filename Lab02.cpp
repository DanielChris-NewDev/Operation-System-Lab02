#include <iostream>
#include <windows.h>
#include <vector>
#include <cstring>
#include <iomanip>
#include <string>
#include <sstream>
using namespace std;

struct ENTRY {
    string name, fullName;
    UINT32 startCluster;
    string dateCreated;
    string timeCreated;
    UINT32 fileSize;
};

struct Process {
    string PID;
    int arrivalTime;
    int burstTime;
    int turnaroundTime;
    int waitingTime;
    int remainingTime;
    int completionTime;
    string queueID;
};

struct Queue {
    string QID;
    int timeSlice;
    string schedulingAlgorithm;
};


#pragma pack(push, 1)
struct fat_32
{
    unsigned int		tableSize_32;
    unsigned short		flags;
    unsigned short		version;
    unsigned int		rootCluster;
    unsigned short		fatInfo;
    unsigned short		backupBootSector;
    unsigned char 		reserved[12];
    unsigned char		driveNumber;
    unsigned char 		reserved1;
    unsigned char		bootSignature;
    unsigned int 		volumeId;
    unsigned char		volumeLabel[11];
    unsigned char		fatTypeLabel[8];

};

struct bootSector
{
    unsigned char 		jump[3];
    unsigned char 		oem[8];
    unsigned short		bytesPerSector; //Bytes per sector
    unsigned char		sectorsPerCluster; // Sector per cluster
    unsigned short		reservedSectorNum; // Number of sectors in the Boot Sector region
    unsigned char		numOfFATTable; // Number of FAT tables
    unsigned short		numOfRootEntry; //Number of sectors for the RDET
    unsigned short		totalSectors16;
    unsigned char		mediaType;
    unsigned short		tableSize16;
    unsigned short		sectorsPerTrack;
    unsigned short		numOfHead;
    unsigned int 		hiddenSector;
    unsigned int 		totalSectors32;

    fat_32		FAT32;

};

#pragma pack(pop)

bootSector readBootSector(LPCWSTR path, HANDLE disk) {
    //Truy cập usb
    //HANDLE disk = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (disk == INVALID_HANDLE_VALUE) {
        cout << "Cannot open drive";
    }

    //Vùng nhớ để lưu các byte được đọc từ usb
    vector<BYTE> buffer(512);

    //Số byte đã đọc
    DWORD numOfBytesRead;

    // Đọc 512 byte đầu tiên của usb
    ReadFile(disk, buffer.data(), 512, &numOfBytesRead, NULL);
    //CloseHandle(disk);

    //Chép 512 byte đã đọc từ usb sang struct
    bootSector bs;
    memcpy(&bs, buffer.data(), sizeof(bootSector));

    return bs;
}

void printBootSectorInfo(bootSector bs) {
    cout << "\n--------------------------------------------------------\n";
    cout << "|               Boot Sector Information                |\n";
    cout << "------------------------------------+-------------------\n";
    cout << left;
    cout << "| " << setw(35) << "Bytes per sector" << "| " << setw(9) << bs.bytesPerSector << "(bytes)|" << endl;
    cout << "| " << setw(35) << "Sectors per cluster" << "| " << setw(15) << (int)bs.sectorsPerCluster << " |" << endl;
    cout << "| " << setw(35) << "Number of sectors in the Boot Sector region" << "| " << setw(7) << bs.reservedSectorNum << " |" << endl;
    cout << "| " << setw(35) << "Number of FAT tables" << "| " << setw(15) << (int)bs.numOfFATTable << " |" << endl;
    cout << "| " << setw(35) << "Number of sectors per FAT table" << "| " << setw(15) << bs.FAT32.tableSize_32 << " |" << endl;
    cout << "| " << setw(35) << "Number of sectors for the RDET" << "| " << setw(15) << (bs.numOfRootEntry * 32) / bs.bytesPerSector << " |" << endl;
    cout << "| " << setw(35) << "Total number of sectors on the disk" << "| " << setw(15) << bs.totalSectors32 << " |" << endl;
    cout << "--------------------------------------------------------\n";
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
                if (sector[i + 11] & 0x10) {
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
    long long startByte = (long long)dataSection + (long long)firstCluster;///

    LARGE_INTEGER large;///
    large.QuadPart = startByte;///

    SetFilePointerEx(usb, large, NULL, FILE_BEGIN);///

    // Bắt đầu ở sector đầu tiên
    BYTE buffer[512];
    DWORD bytes_read; // các byte đã đọc

    if (!ReadFile(usb, buffer, 512, &bytes_read, NULL)) { //bắt đầu từ phần tử/chữ đầu tiên trong file
        cout << "Unable to read file's information\n";
        return "";
    }

    return string((char*)buffer, file.fileSize);
}

void readFileContent(string file) {
    stringstream ss(file);
    int queueAmount;

    ss >> queueAmount;
    
    vector<Queue> queues;
    for (int i = 0; i < queueAmount; i++) {
        Queue q;
        ss >> q.QID >> q.timeSlice >> q.schedulingAlgorithm;
        queues.push_back(q);
    }

    vector<Process> processes;
    Process p;

    while (ss >> p.PID >> p.arrivalTime >> p.burstTime >> p.queueID) {
        p.remainingTime = p.burstTime;
        processes.push_back(p);
    }

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
    for (const auto& process : processes) cout << setw(15) << process.queueID;
    cout << endl;

    cout << left << setw(20) << "Time slice";
    for (const auto& process : processes) {
        int slice = 0;
        for (const auto& q : queues) if (q.QID == process.queueID) slice = q.timeSlice;
        cout << setw(15) << slice;
    }
    cout << endl;

    cout << left << setw(20) << "Algorithm";
    for (const auto& process : processes) {
        string algorithm = "";
        for (const auto& q : queues) if (q.QID == process.queueID) algorithm = q.schedulingAlgorithm;
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
    readFileContent(content);
}

int main()
{
    vector<ENTRY> txtFiles;
    LPCWSTR path = L"\\\\.\\E:";

    HANDLE usb = CreateFile(L"\\\\.\\E:", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    //Boot sector info
    bootSector bsector = readBootSector(path, usb);
    printBootSectorInfo(bsector);
    cout << '\n';

    //Search for txt files
    txtFiles = searchFiles(usb, bsector, bsector.FAT32.rootCluster);
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

    fileInfo(usb, bsector, txtFiles[0]);

    CloseHandle(usb);
    return 0;
}


