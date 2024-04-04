#include "FAT12_reader.h"
#include <assert.h>

uint32_t data_begin = 0;
Fat12Header header;
uint32_t FAT_table[FAT_TABLE_SIZE];
FILE *file;
char red[] = "\033[31m";
char reset[] = "\033[0m";
int valid_entry(RootEntry *entry) {
  return (entry->DIR_Attr == TYPE_FILE || entry->DIR_Attr == TYPE_DIR) &&
         entry->DIR_Name[0] != 0x00 && (uint8_t)entry->DIR_Name[0] != 0xE5 &&
         strncmp(entry->DIR_Name, "TRASH-~1", 8) != 0;
}
void read_sec(uint32_t fst_clus, char *buf) {
  fseek(file, data_begin + (fst_clus - 2) * header.BPB_BytsPerSec, SEEK_SET);
  fread(buf, header.BPB_BytsPerSec, 1, file);
}
char *read_secs(uint32_t fst_clus, uint32_t *clus_cnt) {
  uint32_t sec_cnt = 0;
  int clus_ptr = fst_clus;
  while (clus_ptr > 0 && clus_ptr < FAT_TABLE_SIZE) {
    sec_cnt++;
    clus_ptr = FAT_table[clus_ptr];
  }
  *clus_cnt = sec_cnt;
  char *buf = (char *)malloc(sec_cnt * header.BPB_BytsPerSec);
  char *buf_ptr = buf;
  clus_ptr = fst_clus;
  while (clus_ptr > 0 && clus_ptr < FAT_TABLE_SIZE) {
    read_sec(clus_ptr, buf_ptr);
    buf_ptr += header.BPB_BytsPerSec;
    // printf("%d->", clus_ptr);
    clus_ptr = FAT_table[clus_ptr];
    // printf("%d\n", clus_ptr);
  }
  return buf;
}

FileDir *dfs_read(RootEntry *entry, FileDir *parent) {
  FileDir *new_dir_file = (FileDir *)malloc(sizeof(FileDir));
  uint32_t clus_cnt = 0;
  if (entry->DIR_Attr == TYPE_FILE) {
    int a = 1;
    a = 2;
  }
  new_dir_file->parent = parent;
  new_dir_file->size = entry->DIR_FileSize;
  new_dir_file->buf = read_secs(entry->DIR_FstClus, &clus_cnt);
  strcpy(new_dir_file->name, entry->DIR_Name);
#ifdef DEBUG
  printf("%.11s\n", new_dir_file->name);
#endif
  if (entry->DIR_Attr == TYPE_FILE) {
    new_dir_file->type = TYPE_FILE;
#ifdef DEBUG
    printf("%s\n", new_dir_file->buf);
#endif
    return new_dir_file;
  }
  new_dir_file->type = TYPE_DIR;
  new_dir_file->children_size = 0;
  new_dir_file->file_size = 0;
  new_dir_file->file_size = 0;
  RootEntry *child_entries = (RootEntry *)new_dir_file->buf;
  uint32_t max_entry = clus_cnt * header.BPB_BytsPerSec / sizeof(RootEntry);
  FileDir **children = malloc(sizeof(FileDir *) * max_entry);
  for (uint32_t i = 0; i < max_entry; i++) {
    if (!valid_entry(child_entries + i)) {
      continue;
    }
    if (child_entries[i].DIR_FstClus == entry->DIR_FstClus ||
        strncmp(child_entries[i].DIR_Name, "..", 2) == 0) {
      continue;
    }
    children[new_dir_file->children_size++] =
        dfs_read(child_entries + i, new_dir_file);

    if (child_entries[i].DIR_Attr == TYPE_DIR) {
      new_dir_file->dir_size++;
    } else {
      new_dir_file->file_size++;
    }
  }
  new_dir_file->children = children;
  return new_dir_file;
}

int dir_name_cmp(char *input, char *file) {
  int len = strlen(input);
  if (strncmp(input, file, len) != 0) {
    return 0;
  }
  for (int i = len; i < 8; i++) {
    if (file[i] != ' ') {
      return 0;
    }
  }
  return 1;
}

int file_name_cmp(char *input, char *file) {
  int pre_len = 0;
  int len = strlen(input);
  while (pre_len < len && input[pre_len] != '.') {
    pre_len++;
  }
  char tmp_str[12] = "           ";
  strncpy(tmp_str, input, pre_len);
  if (pre_len < len) {
    strncpy(tmp_str + 8, input + pre_len + 1, len - pre_len - 1);
  }
  if (strncmp(tmp_str, file, 11) == 0) {
    return 1;
  }
  return 0;
}

FileDir *find_dir(char *path, FileDir *root) {
  const char *delimiter = "/";

  char *token = strtok(path, delimiter);
  FileDir *dir_ptr = root;
  while (token != NULL) {
    int flag = 0;
    if (strcmp(token, ".") == 0) {

    } else if (strcmp(token, "..") == 0) {
      dir_ptr = dir_ptr->parent;
      if (!dir_ptr) {
        return NULL;
      }
    } else {
      for (int i = 0; i < dir_ptr->children_size; i++) {
        if (dir_ptr->children[i]->type == TYPE_FILE) {
          continue;
        }
        if (dir_name_cmp(token, dir_ptr->children[i]->name)) {
          dir_ptr = dir_ptr->children[i];
          flag = 1;
          break;
        }
      }
      if (!flag) {
        return NULL;
      }
    }
    token = strtok(NULL, delimiter);
  }
  return dir_ptr;
}

FileDir *find_file(char *path, FileDir *root) {
  int len = strlen(path);
  int terminal_loc = len;
  while (terminal_loc > 0 && path[terminal_loc - 1] != '/') {
    terminal_loc--;
  }
  char dir_path[64] = "";
  strncpy(dir_path, path, terminal_loc);
  FileDir *dir = find_dir(dir_path, root);
  for (int i = 0; i < dir->children_size; i++) {
    if (dir->children[i]->type == TYPE_FILE) {
      if (file_name_cmp(path + terminal_loc, dir->children[i]->name)) {
        return dir->children[i];
      }
    }
  }
  return NULL;
}
void print_name(char *name) {
  int len = 8;
  while (len >= 1 && name[len - 1] == ' ') {
    len--;
  }
  printf("%.*s", len, name);
  len = 3;
  while (len >= 1 && name[8 + len - 1] == ' ') {
    len--;
  }
  if (len) {
    printf(".");
    printf("%.*s", len, name + 8);
  }
}
void print_number(uint32_t n) {
  if (n == 0) {
    printf("0");
  }
  char num_str[16];
  int cnt = 0;
  while (n > 0) {
    num_str[cnt++] = (n % 10) + 48;
    n /= 10;
  }
  for (int l = 0, r = cnt - 1; l < r; l++, r--) {
    char t = num_str[l];
    num_str[l] = num_str[r];
    num_str[r] = t;
  }
  printf("%.*s", cnt, num_str);
}

void print_dir(char *path, FileDir *dir, uint32_t cmd_flag) {
  if (cmd_flag == 0) {
    char line[64];
    strcpy(line, path);
    int len = 8;
    while (len >= 1 && dir->name[len - 1] == ' ') {
      len--;
    }
    strncat(line, dir->name, len);
    strcat(line, "/");
    printf("%s", line);
    printf(":\n");
    if (dir->parent) {
      printf("%s", red);
      printf(".  ..  ");
      printf("%s", reset);
    }
    for (int i = 0; i < dir->children_size; i++) {
      if (dir->children[i]->type == TYPE_DIR) {
        printf("%s", red);

        print_name(dir->children[i]->name);
        printf("%s", reset);
        printf("  ");
      } else {
        print_name(dir->children[i]->name);
        printf("  ");
      }
    }
    printf("\n");
    for (int i = 0; i < dir->children_size; i++) {
      if (dir->children[i]->type == TYPE_DIR) {
        print_dir(line, dir->children[i], cmd_flag);
      }
    }
  } else if (cmd_flag == DETAIL) {
    char line[64];
    strcpy(line, path);
    int len = 8;
    while (len >= 1 && dir->name[len - 1] == ' ') {
      len--;
    }
    strncat(line, dir->name, len);
    strcat(line, "/");
    printf("%s", line);
    printf(" ");
    print_number(dir->dir_size);
    printf(" ");
    print_number(dir->file_size);
    printf(":\n");
    if (dir->parent) {
      printf("%s", red);
      printf(".\n..\n");
      printf("%s", reset);
    }
    for (int i = 0; i < dir->children_size; i++) {
      if (dir->children[i]->type == TYPE_DIR) {
        printf("%s", red);

        print_name(dir->children[i]->name);
        printf("%s", reset);

        printf(" ");
        print_number(dir->children[i]->dir_size);
        printf(" ");
        print_number(dir->children[i]->file_size);
        printf("\n");
      } else {
        print_name(dir->children[i]->name);
        printf(" ");
        print_number(dir->children[i]->size);
        printf("\n");
      }
    }
    printf("\n");
    for (int i = 0; i < dir->children_size; i++) {
      if (dir->children[i]->type == TYPE_DIR) {
        print_dir(line, dir->children[i], cmd_flag);
      }
    }
  }
}

void print_file(FileDir *file) { printf("%s", file->buf); }

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

  unsigned char buf[3];
  for (uint16_t i = 0; i < FAT_TABLE_SIZE; i += 2) {
    fread(buf, sizeof(char), 3, file);
    FAT_table[i] = ((buf[1] & 0xf) << 8) | buf[0];
    FAT_table[i + 1] = ((uint32_t)buf[2] << 4) | (buf[1] >> 4);
#ifdef DEBUG
    if (i < 100)
      printf("%d: %01x %01x %01x: %d %d\n", i, buf[0], buf[1], buf[2],
             FAT_table[i], FAT_table[i + 1]);
#endif
  }

  // Read root entry
  // Move the pointer to root etry
  RootEntry *root_entries =
      (RootEntry *)malloc(sizeof(RootEntry) * header.BPB_RootEntCnt);
  fseek(file, FAT_TABLE_SIZE * 3 / 2, SEEK_CUR);
  fread(root_entries, sizeof(RootEntry), header.BPB_RootEntCnt, file);
  data_begin = ftell(file);
#ifdef DEBUG
  for (int i = 0; i < 20; ++i) {
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
  FileDir root;
  root.parent = NULL;
  FileDir **root_dir = malloc(sizeof(FileDir *) * header.BPB_RootEntCnt);
  root.children_size = 0;
  strcpy(root.name, "");
  root.file_size = 0;
  root.dir_size = 0;
  root.type = TYPE_DIR;
  for (int i = 0; i < header.BPB_RootEntCnt; i++) {
    if (valid_entry(root_entries + i)) {
      if (root_entries[i].DIR_Attr == TYPE_DIR) {
        root.dir_size++;
      } else {
        root.file_size++;
      }
      root_dir[root.children_size++] = dfs_read(root_entries + i, &root);
    }
  }
  root.children = root_dir;

  while (1) {
    char command[64];
    printf("> ");
    fgets(command, sizeof(command), stdin);
    command[strlen(command) - 1] = 0;
    if (strcmp(command, "exit") == 0) {
      return 0;
    }
    uint32_t cmd_type = 0;
    uint32_t cmd_flag = 0;
    int error = 0;
    char target_path[64] = "";
    char delim[2] = " ";
    char *token = strtok(command, delim);
    uint32_t cnt = 0;
    while (token != NULL) {
      if (cnt == 0) {
        if (strcmp(token, "ls") == 0) {
          cmd_type = CMD_LS;
        } else if (strcmp(token, "cat") == 0) {
          cmd_type = CMD_CAT;
        } else {
          error = 1;
          break;
        }
      } else {
        if (token[0] == '-') {
          int len = strlen(token);
          for (int i = 1; i < len; i++) {
            if (token[i] != 'l') {
              error = 1;
              break;
            }
          }
          if (error)
            break;
          cmd_flag = DETAIL;
        } else {
          if (target_path[0] != 0) {
            error = 1;
            break;
          }
          strcpy(target_path, token);
        }
      }

      token = strtok(NULL, delim);
      cnt++;
    }
    if (error) {
      printf("Error input\n");
      continue;
    }

    if (cmd_type == CMD_LS) {
      int len = strlen(target_path);
      char *dirty_path = malloc(len + 1);
      strcpy(dirty_path, target_path);
      FileDir *target_dir = find_dir(dirty_path, &root);
      free(dirty_path);
      if (target_path[len - 1] == '/') {
        target_path[len - 1] = 0;
        len--;
      }
      while(len > 0) {
        if (target_path[len - 1] != '/') {
          target_path[len - 1] = 0;
          len--;
        } else {
          break;
        }
      }
      if (!target_dir) {
        printf("cannot find directory\n");
      } else {
        print_dir(target_path, target_dir, cmd_flag);
      }
    } else {
      FileDir *target_file = find_file(target_path, &root);
      if (!target_file) {
        printf("connot find file\n");
      } else {
        print_file(target_file);
      }
    }
  }
  fclose(file);
  free(root_entries);
  return 0;
}
