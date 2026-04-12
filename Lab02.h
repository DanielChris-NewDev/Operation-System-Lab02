#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>
#include <windows.h>
#include <vector>
#include <cstring>
#include <iomanip>
#include <conio.h>

#include "CPU_scheduling.h"

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

bootSector readBootSector(const char* path);
void printBootSectorInfo(bootSector bs);
void consoleWindow();
int handleChoice(sf::RenderWindow& window, sf::Font& font, const vector<string>& choices);
void CPUScheduling(sf::RenderWindow& window, sf::Font& font, const string& path);