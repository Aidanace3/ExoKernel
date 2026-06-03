#ifndef FS_H
#define FS_H

#define EXFS_MAX_FILES 16
#define EXFS_NAME_LEN 24
#define EXFS_DATA_SECTORS 4
#define EXFS_FILE_BYTES (EXFS_DATA_SECTORS * 512)

struct __attribute__((packed)) exfs_entry {
    unsigned char used;
    char name[EXFS_NAME_LEN];
    unsigned int size;
    unsigned int start_lba;
};

/* Function Prototypes */
int ata_read_sector(unsigned int lba, unsigned char* buffer);
int ata_write_sector(unsigned int lba, unsigned char* buffer);
void init_fs();
void list_files();
void fs_cat(char* name);
void fs_touch(char* name);
void fs_write(char* name, char* data);
void fs_delete(char* name);

#endif
