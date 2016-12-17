/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	INCLUDES
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
#include "parsefat.h"
#include <string.h>
#include <locale.h>

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	DEFINES
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
#define ASSERT(x) { \
	if (NULL == (x)) \
	{ \
		printf("The error occurred, file: %s, line: %d\n", __FILE__, __LINE__); \
		getchar(); \
		exit(0); \
	} \
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	FUNCTION PROTOTYPES
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
static uint32_t getAdrOfClus(const uint32_t N, const tBS * BS);
static void readDirEntry(uint8_t DirEnt[], FILE * Stream);
static char * getLongName(const tLDIR * LDIR);
static tFOLDER * readClusterChain(tFOLDER * Dir, const tBS * BS, FILE * File, uint32_t ClusNum);
static uint32_t getClusNum(tDIR * DirEnt);
static uint32_t getFAT32ClusEntryVal(uint32_t ClusNum, const tBS * BS, FILE * Stream);

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	GLOBAL FUNCTIONS
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/***********************************************************************************
	Name:			readBS
	Description:	Reads BS and BPB data into the structure
	Parameters:		BS - pointer to tBS structure
					Stream - pointer to *.img file
	Return:			None
***********************************************************************************/
void readBS(tBS * BS, FILE * Stream)
{
	fread(BS, sizeof(uint8_t), sizeof(tBS), Stream);
}

/***********************************************************************************
	Name:			fatType
	Description:	The determination of the FAT type
	Parameters:		BS - pointer to tBS structure
	Return:			FAT12, FAT16 or FAT32
***********************************************************************************/
eFAT_TYPE fatType(const tBS * BS)
{
	/* The first sector of cluster 2 */
	uint32_t RootDirSectors = ((BS->BPB_RootEntCnt * DIR_ENT_SZ) + (BS->BPB_BytsPerSec - 1)) / BS->BPB_BytsPerSec;
	/* The count of sectors in the data region */
	uint32_t FATSz = (BS->BPB_FATSz16 != 0) ? BS->BPB_FATSz16 : BS->BPB_FATSz32;
	/* The total count of sectors */
	uint32_t TotSec = (BS->BPB_TotSec16 != 0) ? BS->BPB_TotSec16 : BS->BPB_TotSec32;
	/* The count of sectors that data occupied */
	uint32_t DataSec = TotSec - (BS->BPB_ResvdSecCnt + (BS->BPB_NumFATs * FATSz) + RootDirSectors);
	/* The count of clusters that data occupied */
	uint32_t CountOfCluster = DataSec / BS->BPB_SecPerClus;
	/* The FAT type determination */
	return (CountOfCluster < 4095) ? FAT12 : (CountOfCluster < 65525) ? FAT16 : FAT32;
}

/***********************************************************************************
	Name:			getFirstSectorOfCluster
	Description:	Calculate the first sector for any cluster number
	Parameters:		N - the number of cluster
					BS - pointer to tBS structure
	Return:			The sector number
***********************************************************************************/
static uint32_t getAdrOfClus(const uint32_t ClusNum, const tBS * BS)
{
	/* The first sector of data region */
	uint32_t FirstDataSector = BS->BPB_ResvdSecCnt + BS->BPB_NumFATs * BS->BPB_FATSz32;
	/* The sector number for any valid data cluster number ClusNum */
	uint32_t SecNum = (ClusNum - FRST_DATA_CLUST) * BS->BPB_SecPerClus + FirstDataSector;
	/* The address of this sector */
	return SecNum * BS->BPB_BytsPerSec;
}

/***********************************************************************************
	Name:			readDirEntry
	Description:	Read the one directory entry
	Parameters:		DirEnt - pointer to directory entry buffer
					Stream - pointer to *.img file
	Return:			The sector number
***********************************************************************************/
static void readDirEntry(uint8_t DirEnt[], FILE * Stream)
{
	fread(DirEnt, sizeof(uint8_t), DIR_ENT_SZ, Stream);
}

/***********************************************************************************
	Name:			createDirTableTree
	Description:	Create directory table tree
	Parameters:		BS - pointer to tBS structure
					Stream - pointer to *.img file
	Return:			Pointer to tFOLDER structure
***********************************************************************************/
tFOLDER * createDirTableTree(const tBS * BS, FILE * Stream)
{
	/* Return the pointer to root directory */
	return readClusterChain(NULL, BS, Stream, FRST_DATA_CLUST);
}

/***********************************************************************************
	Name:			getLongName
	Description:	Gets directory or file name from long directory entries
	Parameters:		LDIR - pointer to tLDIR structure
	Return:			Pointer to name array
***********************************************************************************/
static char * getLongName(const tLDIR * LDIR)
{
	/* Define pointer to string */
	static char * Name;
	/* The size of name per one long directory entry */
	uint32_t LenLongName = NUM_OF_ELEM(LDIR->LDIR_Name1)
		+ NUM_OF_ELEM(LDIR->LDIR_Name2)
		+ NUM_OF_ELEM(LDIR->LDIR_Name3);
	/* Order of current entry */
	uint32_t OrdEnt = LDIR->LDIR_Ord & (LAST_LONG_ENTRY - 1);

	/* If this entry is last*/
	if (LDIR->LDIR_Ord & LAST_LONG_ENTRY)
	{	/* The memory of necessary size is allocated */
		ASSERT(Name = (char *)malloc(LenLongName * OrdEnt))
	}
	/* Set the pointer to an appropriate position */
	char * s = Name + LenLongName * (OrdEnt - 1);
	/* Consistent reading */
	for (uint8_t i = 0; i < NUM_OF_ELEM(LDIR->LDIR_Name1); i++)
		*s++ = (char)LDIR->LDIR_Name1[i];
	for (uint8_t i = 0; i < NUM_OF_ELEM(LDIR->LDIR_Name2); i++)
		*s++ = (char)LDIR->LDIR_Name2[i];
	for (uint8_t i = 0; i < NUM_OF_ELEM(LDIR->LDIR_Name3); i++)
		*s++ = (char)LDIR->LDIR_Name3[i];

	/* Return the pointer to the name string */
	return Name;
}

/***********************************************************************************
	Name:			readClusterChain
	Description:	Process one FAT directory entry structure
	Parameters:		Dir - pointer to tFOLDER structure
					BS - pointer to tBS structure
					Stream - pointer to *.img file
					ClusNum - current cluster number
	Return:			Pointer to directory
***********************************************************************************/
static tFOLDER * readClusterChain(tFOLDER * Dir, const tBS * BS, FILE * Stream, uint32_t ClusNum)
{
	/* The buffer for one directory entry structure */
	uint8_t DirEnt[DIR_ENT_SZ];

	/* Infinite loop */
	for (;;)
	{
		/* The address of the cluster */
		uint32_t SecAddr = getAdrOfClus(ClusNum, BS);
		/* The number of FAT entries per cluster */
		uint32_t NumOfDirEnt = BS->BPB_SecPerClus * BS->BPB_BytsPerSec / DIR_ENT_SZ;

		/* Set the file position */
		ASSERT((void *)!fseek(Stream, SecAddr, SEEK_SET))
			/* The cluster reading in the FAT entries */
			while (NumOfDirEnt-- > 0)
			{
				/* Read the next directory entry */
				readDirEntry(DirEnt, Stream);
				/* Check if this entry is not empty */
				switch (DirEnt[0])
				{
					/* Return because of all other entries are free */
				case 0x00:
					return Dir;

					/* This entry is free or dot or dotdot, skip it */
				case 0x05:
				case 0xE5:
				case '.':
					continue;

					/* Valid name */
				default:
					/* This entry isn't free, let's parse it! */
					switch (((tDIR *)DirEnt)->DIR_Attr)
					{
						extern char * FileName;
						/* Contains the long name */
						static char * LongName;

						/* Root directory */
					case ATTR_VOLUME_ID:
						/* Memory allocation for root folder */
						ASSERT(Dir = (tFOLDER *)calloc(1, sizeof(tFOLDER)))
							/* Set the name of root folder */
							ASSERT(Dir->Name = (char *)malloc(strlen(FileName) + 1))
							/* Copy the root name to the structure field */
							strcpy(Dir->Name, FileName);
						break;

						/* Directory */
					case ATTR_DIRECTORY:
						/* Allocate the new memory for one tFOLDER structure */
						ASSERT(Dir->SubDir = (tFOLDER *)realloc(Dir->SubDir, (Dir->NumOfDirs + 1) * sizeof(tFOLDER)))
							/* Fill the allocated memory by 0 */
							memset((void *)&Dir->SubDir[Dir->NumOfDirs], 0x00, sizeof(tFOLDER));
						/* Set the name of the sub directory */
						Dir->SubDir[Dir->NumOfDirs].Name = LongName;
						/* Save the current position */
						uint32_t FilePos = ftell(Stream);
						/* Call the function */
						readClusterChain(&Dir->SubDir[Dir->NumOfDirs++], BS, Stream, getClusNum((tDIR *)DirEnt));
						/* Restore the file position */
						ASSERT((void *)!fseek(Stream, FilePos, SEEK_SET))
							break;

						/* Long name */
					case ATTR_LONG_NAME:
						/* Get long name */
						LongName = getLongName((tLDIR *)DirEnt);
						break;

						/* File */
					default:
						/* The memory allocation for file */
						ASSERT(Dir->File = (tFILE *)realloc(Dir->File, (Dir->NumOfFiles + 1) * sizeof(tFILE)));
						/* Fill the allocated memory by 0 */
						memset((void *)&Dir->File[Dir->NumOfFiles], 0x00, sizeof(tFILE));
						/* Set the name of the file */
						Dir->File[Dir->NumOfFiles++].Name = LongName;
						break;
					}
					break;
				}
			}
		/* Get next cluster in the chain from FAT */
		ClusNum = getFAT32ClusEntryVal(ClusNum, BS, Stream);
		/* If it 0, 1, BAD or EOC - return */
		if (ClusNum < 0x00000002 || ClusNum > 0x0FFFFFF6) return Dir;
	}
	return Dir;
}

/***********************************************************************************
	Name:			getClusNum
	Description:	Returns the cluster number to the next entry in the chain
	Parameters:		DirEnt - FAT directory entry structure
	Return:			Cluster number
***********************************************************************************/
static uint32_t getClusNum(tDIR * DirEnt)
{	/* Return cluster number */
	return DirEnt->DIR_FstClusHI << sizeof(DirEnt->DIR_FstClusLO) | DirEnt->DIR_FstClusLO;
}

/***********************************************************************************
	Name:			getFAT32ClusEntryVal
	Description:	Returns the value of the next cluster in the chain from FAT
	Parameters:		ClusNum - Current cluster number
					BS - pointer to tBS structure
					Stream - pointer to *.img file
	Return:			The next cluster number or EOC in FAT
***********************************************************************************/
static uint32_t getFAT32ClusEntryVal(uint32_t ClusNum, const tBS * BS, FILE * Stream)
{
	/* Next cluster number from FAT */
	uint32_t FAT32ClusEntryVal;
	/* 4 bytes per one value in the FAT for FAT32 */
	uint32_t FATOffset = ClusNum * 4;
	/* The start address of the FAT */
	uint32_t FATAddr = BS->BPB_ResvdSecCnt * BS->BPB_BytsPerSec;
	/* Set the position to appropriate position */
	ASSERT((void *)!fseek(Stream, FATAddr + FATOffset, SEEK_SET))
		/* Read next cluster number */
		fread(&FAT32ClusEntryVal, sizeof(FAT32ClusEntryVal), 1, Stream);
	/* Return the masked value */
	return FAT32ClusEntryVal & 0x0FFFFFFF;
}

/***********************************************************************************
	Name:			dispDirTableTree
	Description:	Display the directory structure
	Parameters:		Dir - pointer to tFOLDER structure
	Return:			None
***********************************************************************************/
void dispDirTableTree(const tFOLDER * const Dir)
{
	//(setlocale(LC_ALL, ""))
	/* The number of entries */
	static uint32_t EntryNum;
	/* Text patterns */
	static unsigned char Txt1[] = { 0xB3, 0x20, 0x20, 0x00 }; // "│  " For the file if the directory exist
	static unsigned char Txt2[] = { 0x20, 0x20, 0x20, 0x00 }; // "   " For the file if the directory not exist
	static unsigned char Txt3[] = { 0xC3, 0xC4, 0xC4, 0x00 }; // "├──" For the directory
	static unsigned char Txt4[] = { 0xC0, 0xC4, 0xC4, 0x00 }; // "└──" For the last directory
	/* Dynamic array of states */
	static uint8_t * isDirLast;

	/* Allocate the memory */
	ASSERT(isDirLast = (uint8_t *)realloc(isDirLast, (EntryNum + 1) * sizeof(uint8_t)))
	/* For each entry print the necessary indent with diractory name */
	for (uint32_t i = 0; i < EntryNum; i++)
		printf("%s", (i+1 < EntryNum) ? (isDirLast[i]) ? Txt1 : Txt2 : (isDirLast[i]) ? Txt3 : Txt4);
	printf("%s\n", Dir->Name);
	/* Print files with necessary indent */
	for (uint32_t i = 0; i < Dir->NumOfFiles; i++)
	{
		/* Print the file name with indent */
		for (uint32_t i = 0; i < EntryNum; i++)
			printf("%s", (isDirLast[i]) ? Txt1 : Txt2);
		printf("%s%s\n", (Dir->NumOfDirs) ? Txt1 : Txt2, Dir->File[i].Name);
		/* Insert the gap after the last file */
		if (i + 1 == Dir->NumOfFiles)
		{
			for (uint32_t i = 0; i < EntryNum; i++)
				printf("%s", (isDirLast[i]) ? Txt1 : Txt2);
			printf("%s\n", (Dir->NumOfDirs) ? Txt1 : Txt2);
		}
	}
	/* Print sub directories with necessary indent and sign */
	for (uint32_t i = 0; i < Dir->NumOfDirs; i++)
	{
		isDirLast[EntryNum++] = (i + 1) < Dir->NumOfDirs;
		dispDirTableTree(&Dir->SubDir[i]);
	}
	/* Decrement entry number */
	if (EntryNum-- == 0)
		/* Free allocated memory */
		free(isDirLast);
}

/***********************************************************************************
	Name:			destroyDirTableTree
	Description:	Frees all previously allocated memory
	Parameters:		Dir - pointer to tFOLDER structure
	Return:			None
***********************************************************************************/
void destroyDirTableTree(const tFOLDER * const Dir)
{
	/* The number of entries */
	static uint32_t EntryNum;
	/* For each sub directory */
	for (uint32_t i = 0; i < Dir->NumOfDirs; i++)
	{
		EntryNum++;
		destroyDirTableTree(&Dir->SubDir[i]);
	}
	/* Frees directory names */
	free(Dir->Name);
	/* Frees file names */
	for (uint32_t i = 0; i < Dir->NumOfFiles; i++)
		free(Dir->File[i].Name);
	/* Frees the file table */
	free(Dir->File);
	/* Frees the sub directory table */
	free(Dir->SubDir);
	/* Frees root directory itself */
	if (EntryNum-- == 0)
		free(Dir);
}
