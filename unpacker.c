#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define OPERATION_READ_HEADER 0
#define OPERATION_UNPACK      1
#define OPERATION_UNKNOWN     255

typedef struct s_ArchiveBlock {
    uint32_t fileNameOffset;
    uint32_t fileOffset;
    uint32_t fileSize;
} ArchiveBlock;

typedef struct s_ArchiveHeader {
    char name[16];
    uint32_t fileCount;
} ArchiveHeader;

void PrintHelp() {
    puts("program.exe read_header <source_file>\n"
         "program.exe unpack <source_file> <destination_folder (without / on end)>");
}

void ReadHeader(ArchiveHeader *header, FILE *file) {
    fread(header, sizeof(ArchiveHeader), 1, file);
}

void ReadBlocks(ArchiveBlock *blocks, uint32_t count, FILE *file) {
    fread(blocks, sizeof(ArchiveBlock), count, file);
}

void ReadFilenames(char ***fileNames, ArchiveBlock *blocks, int count, FILE *file) {
    printf("Count: %d\n", count);
    *fileNames = (char**)calloc(count, sizeof(char*));
    for (int i = 0; i < count; i++) {
        fseek(file, blocks[i].fileNameOffset, SEEK_SET);

        int fileNameSize = 0;
        do { fileNameSize++; } while(getc(file));
        (*fileNames)[i] = (char*)calloc(fileNameSize, sizeof(char));

        fseek(file, blocks[i].fileNameOffset, SEEK_SET);
        fread((*fileNames)[i], sizeof(char), fileNameSize, file);
    }
}

bool ValidateHeader(ArchiveHeader *header) {
    const char validHeader[] = { 'a', 'r', 'c', 'h', 'i', 'v', 'e', 32, 32, 'V', '2', '.', '2', 0, 0, 0 };
    for (int i = 0; i < sizeof(validHeader); i++)
        if (header->name[i] != validHeader[i])
            return false;
    return true;
}

int main(int argc, char *argv[]) {
    if (argc < 2) { PrintHelp(); return 0; }

    const char *operationName = argv[1];
    const char operation = (
        (strcmp(operationName, "read_header") == 0) ? OPERATION_READ_HEADER:
        (strcmp(operationName, "unpack") == 0) ? OPERATION_UNPACK :
        OPERATION_UNKNOWN
    );
    if (operation == OPERATION_UNKNOWN || argc < 3) { PrintHelp(); return 0; }

    const char *sourceFile = argv[2];
    FILE *file = fopen(sourceFile, "rb");
    if (file == NULL) { printf("Failed to open file %d\n", sourceFile); return 1; }

    ArchiveHeader header;
    ReadHeader(&header, file);
    if (!ValidateHeader(&header)) puts("Warning: header may be corrupted!");

    ArchiveBlock *blocks = (ArchiveBlock*)calloc(header.fileCount, sizeof(ArchiveBlock));
    ReadBlocks(blocks, header.fileCount, file);

    char **fileNames;
    ReadFilenames(&fileNames, blocks, header.fileCount, file);

    printf("File count: %d\n", header.fileCount);
    for (int i = 0; i < header.fileCount; i++) {
        printf("\tFilename position in file: %u\n"
               "\tFile position in file:     0x%0.8X\n"
               "\tFile size:                 %u\n"
               "\tFilename:                  %s\n\n",
            blocks[i].fileNameOffset,
            blocks[i].fileOffset,
            blocks[i].fileSize,
            fileNames[i]);
    }

    if (operation == OPERATION_READ_HEADER) goto freeMem;
    if (argc < 4) {
        puts("Destination file not presented");
        goto freeMem;
    }

    const char *destPath = argv[3];
    size_t destPathLength = strlen(destPath);
    bool destHasSlash = destPath[destPathLength - 1] == '\\' || destPath[destPathLength - 1] == '/';
    printf("%s: %d\n", destPath, destPathLength);

    size_t newFolderNameLength = strlen(fileNames[0]) - 3;
    char *newFolderName = (char*)calloc(newFolderNameLength + 1, sizeof(char)); // .WAV
    strncpy(newFolderName, fileNames[0], newFolderNameLength);

    size_t newFolderRelativeLength = destPathLength + !destHasSlash + newFolderNameLength;
    char *newFolderRelative = (char*)calloc(newFolderRelativeLength + 1, sizeof(char));
    snprintf(newFolderRelative, newFolderRelativeLength, destHasSlash ? "%s%s" : "%s/%s", destPath, newFolderName);
    free(newFolderName);

    struct stat st = {0};
    if (stat(newFolderRelative, &st) == -1) mkdir(newFolderRelative);

    for (int i = 0; i < header.fileCount; i++) {
        size_t fileNameLength = strlen(fileNames[i]);
        size_t targetSize = newFolderRelativeLength + 1 + fileNameLength;

        char *filePath = (char*)calloc(targetSize + 1, sizeof(char));
        snprintf(filePath, targetSize + 1, "%s/%s", newFolderRelative, fileNames[i]);

        printf("Unpacking %s to %s\n", fileNames[i], filePath);

        FILE *wavFile = fopen(filePath, "wb");
        free(filePath);
        if (wavFile == NULL) continue;

        fseek(file, blocks[i].fileOffset, SEEK_SET);

        char *fileBuffer = (char*)malloc(blocks[i].fileSize);
        fread(fileBuffer, blocks[i].fileSize, 1, file);
        fwrite(fileBuffer, blocks[i].fileSize, 1, wavFile);

        free(fileBuffer);
        fclose(wavFile);
    }
    free(newFolderRelative);

freeMem:
    fclose(file);
    free(fileNames);
    free(blocks);

    return 0;
}
