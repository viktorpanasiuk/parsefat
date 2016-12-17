/*
* parsefat.h
*
*  Created on: 14 груд. 2016 р.
*      Author: Panasiuk
*/

#ifndef PARSEFAT_H_
#define PARSEFAT_H_

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
INCLUDES
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
DEFINES
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/* The size of one FAT directory entry (bytes) for FAT32 */
#define DIR_ENT_SZ 32
/* The first data cluster */
#define FRST_DATA_CLUST 2
/* Number of elements in array */
#define NUM_OF_ELEM(element) (sizeof(element) / sizeof(element[0]))
/* Last long entry mask */
#define LAST_LONG_ENTRY ((uint8_t)0x40)

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
ENUMS
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/* FAT types */
typedef enum _FAT_TYPE
{
	FAT12,
	FAT16,
	FAT32,
} eFAT_TYPE;

/* File attributes */
typedef enum _ATTR
{
	ATTR_READ_ONLY = 0x01,
	ATTR_HIDDEN = 0x02,
	ATTR_SYSTEM = 0x04,
	ATTR_VOLUME_ID = 0x08,
	ATTR_DIRECTORY = 0x10,
	ATTR_ARCHIVE = 0x20,
	ATTR_LONG_NAME = ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID,
} eATTR;

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
STRUCTURES
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/* FAT32 boot sector and BPB structure */
#pragma pack(push, 1)
typedef struct _BS
{
	uint8_t		BS_jmpBoot[3];
	char		BS_OEMName[8];
	uint16_t	BPB_BytsPerSec;
	uint8_t		BPB_SecPerClus;
	uint16_t	BPB_ResvdSecCnt;
	uint8_t		BPB_NumFATs;
	uint16_t	BPB_RootEntCnt;
	uint16_t	BPB_TotSec16;
	uint8_t		BPB_Media;
	uint16_t	BPB_FATSz16;
	uint16_t	BPB_SecPerTrk;
	uint16_t	BPB_NumHeads;
	uint32_t	BPB_HiddSec;
	uint32_t	BPB_TotSec32;
	uint32_t	BPB_FATSz32;
	uint16_t	BPB_ExtFlags;
	uint16_t	BPB_FSVer;
	uint32_t	BPB_RootClus;
	uint16_t	BPB_FSInfo;
	uint16_t	BPB_BkBootSec;
	uint8_t		BPB_Reserved[12];
	uint8_t		BS_DrvNum;
	uint8_t		BS_Reserved1;
	uint8_t		BS_BootSig;
	uint32_t	BS_VolID;
	char		BS_VolLab[11];
	char		BS_FilSysType[8];
} tBS;
#pragma pack(pop)

/* FAT32 short directory entry structure */
#pragma pack(push, 1)
typedef struct _DIR
{
	char		DIR_Name[11];
	uint8_t		DIR_Attr;
	uint8_t		DIR_NTRes;
	uint8_t		DIR_CrtTimeTenth;
	uint16_t	DIR_CrtTime;
	uint16_t	DIR_CrtDate;
	uint16_t	DIR_LstAccDate;
	uint16_t	DIR_FstClusHI;
	uint16_t	DIR_WrtTime;
	uint16_t	DIR_WrtDate;
	uint16_t	DIR_FstClusLO;
	uint32_t	DIR_FileSize;
} tDIR;
#pragma pack(pop)

/* FAT32 long directory entry structure */
#pragma pack(push, 1)
typedef struct _LDIR
{
	uint8_t		LDIR_Ord;
	uint16_t	LDIR_Name1[5];
	uint8_t		LDIR_Attr;
	uint8_t		LDIR_Type;
	uint8_t		LDIR_ChkSum;
	uint16_t	LDIR_Name2[6];
	uint16_t	LDIR_FstClusLO;
	uint16_t	LDIR_Name3[2];
} tLDIR;
#pragma pack(pop)

/* File structure */
typedef struct _FILE {
	char * Name;					// File name
} tFILE;

/* Folder structure */
typedef struct _FOLDER {
	char * Name;					// Directory name
	struct {
		uint32_t NumOfFiles;		// Number of files
		tFILE * File;				// Pointer to tFILE structure
	};
	struct {
		uint32_t NumOfDirs;			// Number of directories
		struct _FOLDER * SubDir;	// Pointer to tFOLDER structure
	};
} tFOLDER;

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
FUNCTION PROTOTYPES
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
extern void readBS(tBS * bs, FILE * fid);
extern eFAT_TYPE fatType(const tBS * BS);
extern tFOLDER * createDirTableTree(const tBS * BS, FILE * Stream);
extern void dispDirTableTree(const tFOLDER * const Dir);
extern void destroyDirTableTree(const tFOLDER * const Dir);

#endif /* PARSEFAT_H_ */