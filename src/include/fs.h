//we need a standard C struct. Every single file, folder, and device in our operating system will be forced to use this struct.
#ifndef FS_H
#define FS_H
#include <stdint.h>

//file type flags
#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05
#define FS_SYMLINK     0x06
#define FS_MOUNTPOINT  0x08 //is the file an active mountpoint?

struct fs_node;

typedef uint32_t (*read_type_t)(struct fs_node*, uint32_t offset, uint32_t size, uint8_t* buffer);
typedef uint32_t (*write_type_t)(struct fs_node*, uint32_t offset, uint32_t size, uint8_t* buffer);
typedef void (*open_type_t)(struct fs_node*);
typedef void (*close_type_t)(struct fs_node*);
typedef struct dirent* (*readdir_type_t)(struct fs_node*, uint32_t index);
typedef struct fs_node* (*finddir_type_t)(struct fs_node*, char* name);

typedef struct fs_node {
    char name[128];     //the requested filename.
    uint32_t mask;      //the permissions mask.
    uint32_t uid;       //the owning user ID.
    uint32_t gid;       //the owning group ID.
    uint32_t flags;     //includes the node type (FS_FILE, FS_DIRECTORY, etc).
    uint32_t inode;     //device-specific identifier (where it lives on the disk).
    uint32_t length;    //size of the file, in bytes.
    uint32_t impl;      //an implementation-defined number (useful for custom drivers).

    //hardware-specific functions mapped to this file.
    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;

    //directory-specific functions
    readdir_type_t readdir;
    finddir_type_t finddir;
    struct fs_node* ptr; //used by mountpoints and symlinks.
} fs_node_t;

//DIRECTORY ENTRY
//returned by readdir() when you want to list the contents of a folder
typedef struct dirent {
    char name[128]; // Filename
    uint32_t ino;   // Inode number
} dirent_t;

//this is the absolute base of your OS tree ("/"). Every file branches from here.
extern fs_node_t* fs_root;

//KERNEL API
//our shell will call these wrappers, which will securely trigger the function pointers.
uint32_t read_fs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
uint32_t write_fs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
void open_fs(fs_node_t* node, uint8_t read, uint8_t write);
void close_fs(fs_node_t* node);
dirent_t* readdir_fs(fs_node_t* node, uint32_t index);
fs_node_t* finddir_fs(fs_node_t* node, char* name);

#endif