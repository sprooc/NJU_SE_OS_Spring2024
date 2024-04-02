#include "FAT12_reader.h"

uint32_t data_begin = 0;
Fat12Header header;
uint32_t FAT_table[FAT_TABLE_SIZE];
FILE *file;
void read_sec(uint32_t fst_clus, char *buf) {
  printf("read %d\n", fst_clus);
  fseek(file, data_begin + (fst_clus - 2) * header.BPB_BytsPerSec, SEEK_SET);
  fread(buf, header.BPB_BytsPerSec, 1, file);
}
char *read_secs(uint32_t fst_clus) {
  uint32_t sec_cnt = 0;
  uint32_t clus_ptr = fst_clus;
  while (clus_ptr != 0) {
    sec_cnt++;
    clus_ptr = FAT_table[clus_ptr];
  }
  char *buf = (char *)malloc(sec_cnt * header.BPB_SecPerClus);
  char *buf_ptr = buf;
  clus_ptr = fst_clus;
  while (clus_ptr != 0) {
    read_sec(clus_ptr, buf_ptr);
    buf_ptr += header.BPB_SecPerClus;
    clus_ptr = FAT_table[clus_ptr];
  }
  return buf;
}

FileDir *dfs_read(RootEntry *entry, FileDir *parent) {
  if (entry->DIR_Attr == TYPE_FILE) {
    FileDir *new_file = (FileDir *)malloc(sizeof(FileDir));
    *new_file = {
        .type = TYPE_FILE, .parent = parent, .size = entry->DIR_FileSize};
  }
  return nullptr;
}

int main(int arg, char *argv[]) {
  if (arg < 2) {
    printf("Usage: %s <filename>\n", argv[0]);
    return 1;
  }

  file = fopen(argv[1], "r");
  if (file == NULL) {
    printf("Error opening file");
    return 1;
  }

  // Read header into buffer
  fread(&header, sizeof(char), 512, file);

#ifdef DEBUG
  printf("BS_JmpBoot: %c%c%c\n", header.BS_JmpBoot[0], header.BS_JmpBoot[1],
         header.BS_JmpBoot[2]);
  printf("BS_OEMName: %s\n", header.BS_OEMName);
  printf("BPB_BytsPerSec: %u\n", header.BPB_BytsPerSec);
  printf("BPB_SecPerClus: %u\n", (unsigned char)header.BPB_SecPerClus);
  printf("BPB_RsvdSecCnt: %u\n", header.BPB_RsvdSecCnt);
  printf("BPB_NumFATs: %u\n", (unsigned char)header.BPB_NumFATs);
  printf("BPB_RootEntCnt: %u\n", header.BPB_RootEntCnt);
  printf("BPB_TotSec16: %u\n", header.BPB_TotSec16);
  printf("BPB_Media: %c\n", header.BPB_Media);
  printf("BPB_FATSz16: %u\n", header.BPB_FATSz16);
  printf("BPB_SecPerTrk: %u\n", header.BPB_SecPerTrk);
  printf("BPB_NumHeads: %u\n", header.BPB_NumHeads);
  printf("BPB_HiddSec: %u\n", header.BPB_HiddSec);
  printf("BPB_TotSec32: %u\n", header.BPB_TotSec32);
  printf("BS_DrvNum: %c\n", header.BS_DrvNum);
  printf("BS_Reserved1: %c\n", header.BS_Reserved1);
  printf("BS_BootSig: %c\n", header.BS_BootSig);
  printf("BS_VolID: %u\n", header.BS_VolID);
  printf("BS_VolLab: %s\n", header.BS_VolLab);
#endif
  // Read FAT_TABLE

  char buf[3];
  for (uint16_t i = 0; i < FAT_TABLE_SIZE; i++) {
    fread(buf, sizeof(char), 3, file);
    FAT_table[i] = (buf[0] << 16) | (buf[1] << 8) | buf[2];
  }

  // Read root entry
  // Move the pointer to root etry
  RootEntry *root_entries =
      (RootEntry *)malloc(sizeof(RootEntry) * header.BPB_RootEntCnt);
  fseek(file, FAT_TABLE_SIZE * 3, SEEK_CUR);
  fread(root_entries, sizeof(RootEntry), header.BPB_RootEntCnt, file);
  data_begin = ftell(file);
#ifdef DEBUG
  for (int i = 0; i < 10; ++i) {
    printf("Root Entry %d:\n", i + 1);
    printf("DIR_Name: %s\n", root_entries[i].DIR_Name);
    printf("DIR_Attr: %d\n", root_entries[i].DIR_Attr);
    printf("reserve: %s\n", root_entries[i].reserve);
    printf("DIR_WrtTime: %u\n", root_entries[i].DIR_WrtTime);
    printf("DIR_WrtDate: %u\n", root_entries[i].DIR_WrtDate);
    printf("DIR_FstClus: %u\n", root_entries[i].DIR_FstClus);
    printf("DIR_FileSize: %u\n", root_entries[i].DIR_FileSize);
    printf("\n");
  }
#endif

  fclose(file);
  free(root_entries);
  return 0;
}
