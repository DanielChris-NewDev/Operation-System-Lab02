#include <iostream>
#include <windows.h>
#include <vector>
#include <cstring>
#include <iomanip>
using namespace std;

struct ENTRY {
    string name, fullName;
    UINT32 startCluster;
    string dateCreated;
    string timeCreated;
    UINT32 fileSize;
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
                dataType += sector[i + 8]; //0x08 là byte thứ 8
                dataType += sector[i + 9];
                dataType += sector[i + 10];

                if (dataType == "TXT") {
                    ENTRY file;
                    char* temp = (char*)&sector[i]; // trỏ tới điểm đầu của entry
                    file.name = string(temp, 8); //tên file là 8 byte đầu tiên

                    UINT16 highByte;
                    memcpy(&highByte, &sector[i + 20], 2); //0x14 là byte thứ 20
                    UINT16 lowByte;
                    memcpy(&lowByte, &sector[i + 26], 2); // 0x1A là byte thứ 26

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
            cout << i + 1 << ". " << txtFiles[i].name << ".txt ";
        }
    }
    CloseHandle(usb);
    return 0;
}

