// FAT32.cpp : Defines the entry point for the console application.
//

#include "parsefat.h"

char * FileName;

int main(int argc, char * argv[])
{
	FileName = argv[1];
	/* Open the *.img file */
	FILE * Stream = fopen(FileName, "rb");
	/* Check the status of the previous operation */
	if (NULL == Stream)
	{
		/* The file wasn't found or couldn't be opened */
		printf("File not found!\n");
		return 0;
	}

	/* The structure for BS and BPB data */
	tBS BS;
	/* Read the BS and BPB form *.img file */
	readBS(&BS, Stream);
	/* FAT type determination */
	if (FAT32 != fatType(&BS))
	{
		/* If the type of FAT is not FAT32 */
		printf("%s", "The type of FAT is not FAT32\n");
		/* Return */
		return 0;
	}
	/* Create the directory table tree */
	tFOLDER * Root = createDirTableTree(&BS, Stream);
	/* Display the directory table tree */
	dispDirTableTree(Root);
	/* Destroy the directory table tree */
	destroyDirTableTree(Root); // It's not necessary because we exit from the program
	/* Close the *.img file */
	fclose(Stream);
	/* Waiting for pressing any key */
	getchar();
	/* Return */
	return 0;
}