#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FAT_TABLE_SIZE 1536
#define TYPE_DIR 0x10
#define TYPE_FILE 0x20
// #define DEBUG

typedef struct __attribute__((packed)) Fat12Header {
  char BS_JmpBoot[3];
  char BS_OEMName[8];
  uint16_t BPB_BytsPerSec;
  char BPB_SecPerClus;
  uint16_t BPB_RsvdSecCnt;
  char BPB_NumFATs;
  uint16_t BPB_RootEntCnt;
  uint16_t BPB_TotSec16;
  char BPB_Media;
  uint16_t BPB_FATSz16;
  uint16_t BPB_SecPerTrk;
  uint16_t BPB_NumHeads;
  uint32_t BPB_HiddSec;
  uint32_t BPB_TotSec32;
  char BS_DrvNum;
  char BS_Reserved1;
  char BS_BootSig;
  uint32_t BS_VolID;
  char BS_VolLab[11];
  char BS_FileSysType[8];
  char BOOT_Code[448];
  char END[2];
} Fat12Header;

typedef struct RootEntry {
  char DIR_Name[11];
  uint8_t DIR_Attr;
  char reserve[10];
  uint16_t DIR_WrtTime;
  uint16_t DIR_WrtDate;
  uint16_t DIR_FstClus;
  uint32_t DIR_FileSize;
} RootEntry;

typedef struct FileDir {
  char name[11];
  uint8_t type; // if file 0; if dir 1
  struct FileDir *parent;
  char *buf;
  uint32_t size;
  
  // only useful when it's directory 
  uint32_t dir_size;
  uint32_t file_size;
  uint32_t children_size;
  struct FileDir *children[];
} FileDir;
