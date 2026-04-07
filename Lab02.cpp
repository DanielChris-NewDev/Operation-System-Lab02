#include <iostream>
#include <windows.h>
#include <vector>
#include <cstring>
#include <iomanip>
using namespace std;

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
    cout << "------------------------------------+------------------\n";
    cout << left;
    cout << "| " << setw(35) << "Bytes per sector" << "| " << setw(15) << bs.bytesPerSector << "(bytes)|" << endl;
    cout << "| " << setw(35) << "Sectors per cluster" << "| " << setw(15) << (int)bs.sectorsPerCluster << " |" << endl;
    cout << "| " << setw(35) << "Number of sectors in the Boot Sector region" << "| " << setw(15) << bs.reservedSectorNum << " |" << endl;
    cout << "| " << setw(35) << "Number of FAT tables" << "| " << setw(15) << (int)bs.numOfFATTable << " |" << endl;
    cout << "| " << setw(35) << "Number of sectors per FAT table" << "| " << setw(15) << bs.FAT32.tableSize_32 << " |" << endl;
    cout << "| " << setw(35) << "Number of sectors for the RDET" << "| " << setw(15) << (bs.numOfRootEntry*32)/bs.bytesPerSector << " |" << endl;
    cout << "| " << setw(35) << "Total number of sectors on the disk" << "| " << setw(15) << bs.totalSectors32 << " |" << endl;
     cout << "-------------------------------------------------------\n";
}

int main(){
   //Boot sector info
   bootSector bsector = readBootSector("\\\\.\\E:");
   printBootSectorInfo(bsector);
    
    return 0;
}