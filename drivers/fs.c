#include "fs.h"
#include "ata.h"
#include "vga.h"

#define EXFS_MAGIC 0x53465845
#define EXFS_VERSION 1
#define EXFS_SUPER_LBA 0
#define EXFS_DIR_LBA 1
#define EXFS_DIR_SECTORS 2
#define EXFS_DATA_LBA (EXFS_DIR_LBA + EXFS_DIR_SECTORS)

struct __attribute__((packed)) exfs_superblock {
    unsigned int magic;
    unsigned int version;
    unsigned int max_files;
    unsigned int data_start_lba;
};

static struct exfs_entry directory[EXFS_MAX_FILES];
static unsigned char memory_data[EXFS_MAX_FILES][EXFS_FILE_BYTES];
static int fs_ready = 0;
static int fs_memory_only = 0;

static void mem_clear(unsigned char* buffer, int len) {
    for (int i = 0; i < len; i++) buffer[i] = 0;
}

static int str_len(char* s) {
    int len = 0;
    while (s && s[len] != '\0') len++;
    return len;
}

static int str_eq(char* a, char* b) {
    int i = 0;
    while (a[i] == b[i]) {
        if (a[i] == '\0') return 1;
        i++;
    }
    return 0;
}

static void str_copy_name(char* dest, char* src) {
    int i = 0;
    while (src[i] != '\0' && i < EXFS_NAME_LEN - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static void print_uint(unsigned int value) {
    char out[12];
    int i = 0;

    if (value == 0) {
        zeal_putc('0');
        return;
    }

    while (value > 0 && i < 11) {
        out[i++] = (value % 10) + '0';
        value /= 10;
    }

    while (i > 0) zeal_putc(out[--i]);
}

static void load_directory() {
    unsigned char sector[512];
    unsigned char* dst = (unsigned char*)directory;
    int copied = 0;

    mem_clear((unsigned char*)directory, sizeof(directory));
    for (int s = 0; s < EXFS_DIR_SECTORS; s++) {
        if (!ata_read_sector(EXFS_DIR_LBA + s, sector)) return;

        for (int i = 0; i < 512 && copied < (int)sizeof(directory); i++) {
            dst[copied++] = sector[i];
        }
    }
}

static void save_directory() {
    if (fs_memory_only) return;

    unsigned char* src = (unsigned char*)directory;
    int copied = 0;

    for (int s = 0; s < EXFS_DIR_SECTORS; s++) {
        unsigned char sector[512];
        mem_clear(sector, 512);

        for (int i = 0; i < 512 && copied < (int)sizeof(directory); i++) {
            sector[i] = src[copied++];
        }

        ata_write_sector(EXFS_DIR_LBA + s, sector);
    }
}

static void format_fs() {
    unsigned char sector[512];
    struct exfs_superblock* super = (struct exfs_superblock*)sector;

    mem_clear((unsigned char*)directory, sizeof(directory));
    mem_clear((unsigned char*)memory_data, sizeof(memory_data));
    mem_clear(sector, 512);

    super->magic = EXFS_MAGIC;
    super->version = EXFS_VERSION;
    super->max_files = EXFS_MAX_FILES;
    super->data_start_lba = EXFS_DATA_LBA;

    if (!fs_memory_only) {
        ata_write_sector(EXFS_SUPER_LBA, sector);
        save_directory();
    }
}

static int find_file(char* name) {
    for (int i = 0; i < EXFS_MAX_FILES; i++) {
        if (directory[i].used && str_eq(directory[i].name, name)) return i;
    }
    return -1;
}

static int find_free_slot() {
    for (int i = 0; i < EXFS_MAX_FILES; i++) {
        if (!directory[i].used) return i;
    }
    return -1;
}

void init_fs() {
    unsigned char sector[512];

    if (!ata_read_sector(EXFS_SUPER_LBA, sector)) {
        fs_memory_only = 1;
        format_fs();
        fs_ready = 1;
        zeal_write("ExFS: ATA storage unavailable, using volatile memory.\n");
        return;
    }

    struct exfs_superblock* super = (struct exfs_superblock*)sector;
    if (super->magic != EXFS_MAGIC || super->version != EXFS_VERSION) {
        format_fs();
        zeal_write("ExFS: formatted storage.img.\n");
    } else {
        load_directory();
        zeal_write("ExFS: mounted storage.img.\n");
    }

    fs_ready = 1;
}

void list_files() {
    int count = 0;

    if (!fs_ready) {
        zeal_write("ExFS: not initialized\n");
        return;
    }

    for (int i = 0; i < EXFS_MAX_FILES; i++) {
        if (directory[i].used) {
            zeal_write(directory[i].name);
            zeal_write("  ");
            print_uint(directory[i].size);
            zeal_write(" bytes\n");
            count++;
        }
    }

    if (count == 0) zeal_write("(empty)\n");
}

void fs_touch(char* name) {
    int slot;

    if (!name || name[0] == '\0') {
        zeal_write("Usage: touch <file>\n");
        return;
    }

    if (find_file(name) >= 0) return;

    slot = find_free_slot();
    if (slot < 0) {
        zeal_write("touch: no free file slots\n");
        return;
    }

    directory[slot].used = 1;
    str_copy_name(directory[slot].name, name);
    directory[slot].size = 0;
    directory[slot].start_lba = EXFS_DATA_LBA + (slot * EXFS_DATA_SECTORS);
    save_directory();
}

void fs_write(char* name, char* data) {
    int slot;
    int len;

    if (!name || name[0] == '\0' || !data) {
        zeal_write("Usage: write <file> <text>\n");
        return;
    }

    slot = find_file(name);
    if (slot < 0) {
        fs_touch(name);
        slot = find_file(name);
    }

    if (slot < 0) return;

    len = str_len(data);
    if (len > EXFS_FILE_BYTES) len = EXFS_FILE_BYTES;

    for (int s = 0; s < EXFS_DATA_SECTORS; s++) {
        unsigned char sector[512];
        mem_clear(sector, 512);

        for (int i = 0; i < 512; i++) {
            int src_index = (s * 512) + i;
            if (src_index >= len) break;
            sector[i] = data[src_index];
        }

        if (!fs_memory_only) {
            ata_write_sector(directory[slot].start_lba + s, sector);
        } else {
            for (int i = 0; i < 512; i++) {
                memory_data[slot][(s * 512) + i] = sector[i];
            }
        }
    }

    directory[slot].size = len;
    save_directory();
}

void fs_cat(char* name) {
    int slot;
    unsigned int remaining;

    if (!name || name[0] == '\0') {
        zeal_write("Usage: cat <file>\n");
        return;
    }

    slot = find_file(name);
    if (slot < 0) {
        zeal_write("cat: file not found: ");
        zeal_write(name);
        zeal_putc('\n');
        return;
    }

    remaining = directory[slot].size;
    for (int s = 0; s < EXFS_DATA_SECTORS && remaining > 0; s++) {
        unsigned char sector[512];

        mem_clear(sector, 512);
        if (fs_memory_only) {
            for (int i = 0; i < 512; i++) {
                sector[i] = memory_data[slot][(s * 512) + i];
            }
        } else if (!ata_read_sector(directory[slot].start_lba + s, sector)) {
            zeal_write("cat: read failed\n");
            return;
        }

        for (int i = 0; i < 512 && remaining > 0; i++) {
            zeal_putc(sector[i]);
            remaining--;
        }
    }
    zeal_putc('\n');
}

void fs_delete(char* name) {
    int slot;

    if (!name || name[0] == '\0') {
        zeal_write("Usage: rm <file>\n");
        return;
    }

    slot = find_file(name);
    if (slot < 0) {
        zeal_write("rm: file not found: ");
        zeal_write(name);
        zeal_putc('\n');
        return;
    }

    mem_clear((unsigned char*)&directory[slot], sizeof(struct exfs_entry));
    mem_clear(memory_data[slot], EXFS_FILE_BYTES);
    save_directory();
}
