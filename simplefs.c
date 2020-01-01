#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "simplefs.h"

#define MAX_FILENAME_LEN    32
#define MAX_FAT_ENTRIES     128   // 128 
#define FAT_ENTRIES_COUNT   1024
#define MAX_FILE            56
#define FILE_COUNT_PER_BLOCK 8
#define TOTAL_BLOCK_FOR_FILE 7
#define HEAD_DATA           1032
#define MAX_NUMBER_OPEN_FILE 10

typedef struct
{
    int num_of_file;
    // Data info
    int num_of_blocks;
    int num_of_used_data_blocks;
    int num_of_free_data_blocks;

} superBlock;


typedef struct
{
    // 128 B
    char name [MAX_FILENAME_LEN];    // name of file
    int  isUsed;                     // whether file is used
    int  mode;                       // read or write
    int  size;                       // size of file
    int  blocks_count;               // number of blocks
    int  num_fd;                     // # of file descriptors using this file
    int  head;                       // index of first entry in fat table 

} fileInfo;

/* file descriptor */
typedef struct
{
    // 8 B
    int isUsed ;                 // whether the file descriptor is being used
    int file_next;               // next data block to read

} fileDes;

typedef struct {
    char name [MAX_FILENAME_LEN]; 
    int  openedFile;
} opened;

int vdisk_fd; // global virtual disk file descriptor
              // will be assigned with the sfs_mount call
              // any function in this file can use this.

superBlock * ptr_superBlock;
fileInfo fileInfos [FILE_COUNT_PER_BLOCK];
fileDes fat [MAX_FAT_ENTRIES];
opened openFiles [MAX_NUMBER_OPEN_FILE];
int openFileCount = 0;

for (int i=0; i<MAX_NUMBER_OPEN_FILE; i++){
    openFiles[i]->openedFile = -1;
}

int numBlock ; 

// This function is simply used to a create a virtual disk
// (a simple Linux file including all zeros) of the specified size.
// You can call this function from an app to create a virtual disk.
// There are other ways of creating a virtual disk (a Linux file)
// of certain size. 
// size = 2^m Bytes
int create_vdisk (char *vdiskname, int m)
{
    char command[BLOCKSIZE]; 
    int size;
    int num = 1;
    int count; 
    size  = num << m;
    count = size / BLOCKSIZE;
    printf ("%d %d", m, size);
    sprintf (command, "dd if=/dev/zero of=%s bs=%d count=%d", vdiskname, BLOCKSIZE, count);
    numBlock = count; 
    printf ("executing command = %s\n", command); 
    system (command); 
    return (0); 
}



// read block k from disk (virtual disk) into buffer block.
// size of the block is BLOCKSIZE.
// space for block must be allocated outside of this function.
// block numbers start from 0 in the virtual disk. 
int read_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = read (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
	printf ("read error\n");
	return -1;
    }
    return (0); 
}

// write block k into the virtual disk. 
int write_block (void *block, int k)
{
    int n;
    int offset;
    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = write (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
	printf ("write error\n");
	return (-1);
    }
    return 0; 
}


/**********************************************************************
   The following functions are to be called by applications directly. 
***********************************************************************/

int sfs_format (char *vdiskname)
{

    ptr_superBlock = (superBlock *) malloc (sizeof(superBlock));
    ptr_superBlock->num_of_blocks = numBlock;
    ptr_superBlock->num_of_free_data_blocks = numBlock;
    ptr_superBlock->num_of_file = 0;
    ptr_superBlock->num_of_used_data_blocks = 0;

    write_block(ptr_superBlock,0);
    
    for (int i=0; i<FILE_COUNT_PER_BLOCK; i ++){
        fileInfos[i].isUsed = 0;
    }

    for (int i=1; i<TOTAL_BLOCK_FOR_FILE; i++){
        write_block(fileInfos,i);
    }


    for(int i = 0; i < MAX_FAT_ENTRIES; ++i) {
        fat[i].isUsed = 0;
    }

    for(int i  =0; i< FAT_ENTRIES_COUNT; i++){
        write_block(fat, 8 + i);
    }

    free(ptr_superBlock);
    
    return (0); 
}

int sfs_mount (char *vdiskname)
{
    // simply open the Linux file vdiskname and in this
    // way make it ready to be used for other operations.
    // vdisk_fd is global; hence other function can use it. 
    vdisk_fd = open(vdiskname, O_RDWR);
    if (vdisk_fd == -1 ){
        printf("Error: Could not open the vdisk.\n");
        return -1;
    }
    return(0);
}

int sfs_umount ()
{
    fsync (vdisk_fd); 
    if (close (vdisk_fd) == -1 ){
        printf("Error: Could not close the vdisk.\n");
        return -1;
    }
    return (0); 
}

int sfs_create(char *filename)
{
    if (find_file(filename)[0] != -1 ){
        return (-1);
    }

    int free_loc [2] = find_first_empty_file_loc();
    if(free_loc[0] == -1){
        printf("Error: Maximum file count has been exceed.\n");
        return -1;
    }

    read_block(fileInfos,free_loc[0]);

    fileInfos[free_loc[1]]-> isUsed = true;
    fileInfos[free_loc[1]]-> name = filename;
    fileInfos[free_loc[1]]-> mode = -1;
    fileInfos[free_loc[1]]-> num_fd = 0;
    fileInfos[free_loc[1]]-> head = -1;

    write_block(fileInfos,free_loc[0]);

    return (0);
}


int sfs_open(char *filename, int mode){
    int file_place [2] = find_file(filename);

    read_block(fileInfos,file_place[0]);

    fileInfo * file = fileInfos[file_place[1]];

    if (file->mode != mode && file->num_fd > 0){
        printf("Error: The file is already opened in different mode.\n");
        return (-1);
    }

    else if (file->mode == mode || file->num_fd == 0){
        file->mode = mode;
        file->num_fd += 1;
    }

    if (openFileCount == MAX_NUMBER_OPEN_FILE ){
         printf("Error: Max limit reached for opening a file.\n");
         return (-1);
    }

    if (file -> head == -1 ){
        int file_d [2] = find_empty_fat_entry ();
        if(file_d[0] == -1 ){
            printf("Error: No empty fat entry.\n");
            return (-1);
        }

        read_block(fat,file_d[0]);

        fat[file_d[1]]->isUsed = 1;
        fat[file_d[1]]->file_next = -1;

        write_block(fat,file_d[0]);

        int desc = (file_d[0] * 128) + file_d[1];
        int empty = -1;
        file->head = desc;

        for (int i = 0 ; i < MAX_NUMBER_OPEN_FILE ; i++){
            if (openFiles[i]->openedFile == -1 ){
                openFiles[i]->openedFile = desc;
                openFiles[i]->name = filename;
                openFileCount += 1;
                break;
            }
        }

        write_block(fileInfos,file_place[0]);

        return desc;
    }
    
    else 
        return file->head;
  

    return (0); 
}

int sfs_close(int fd){

    for (int i = 0 ; i < MAX_NUMBER_OPEN_FILE; i++){
        if (openFiles[i]->openedFile == fd ){
            openFiles[i]->openedFile = -1;
            openFileCount -= 1;
            return 0;
        }
    }
    printf("Error: There is no opened file with given descriptor.\n");
    return (-1); 
}

int sfs_getsize (int fd){
    for (int i = 0 ; i < MAX_NUMBER_OPEN_FILE; i++){
        if (openFiles[i]->openedFile == fd ){
            char * file_name = openFiles[i]->name;
            int* place = find_file(file_name);
            read_block(fileInfos,place[0]);
            return fileInfos[place[1]]->size;
        }
    }
    return (-1); 
}

int sfs_read(int fd, void *buf, int n){

    int blockNo = fd / 128;
    int indexNo = fd % 128;

    read_block(fat,blockNo);

    fileDes found = fat[indexNo];

    char dataBlock [1024];
    read_block(dataBlock, fd + 1032 );

    // whether data block's size is smaller than n 
    int dataSize = sfs_getsize(fd);
    if(dataSize < n){
        n = dataSize;
    }

    if(n <= 1024){
        for (int i=0; i<n;i++){
            buf[i] = dataBlock[i];
        }
        return n;
    }
    int count = 0;
    while (n > 1024){

        for (int i=0; i<1024;i++){
            buf[count + i] = dataBlock[i]; 
        }
        count += 1024;
        n = n - 1024; 
        if (found->file_next == -1)
            break;
        
        fd = found->file_next;
        read_block(dataBlock,fd+1032);
    }

    for (int i=0; i<n;i++){
        buf[count + i] = dataBlock[i];
    }
    count += n;
    return count; 
}


int sfs_append(int fd, void *buf, int n){

    int blockNo = fd / 128;
    int indexNo = fd % 128;

    read_block(fat,blockNo);
    fileDes found = fat[indexNo];
    char dataBlock [1024];

    while (found->file_next != -1 ){
        fd = found->file_next;
        blockNo = fd / 128;
        indexNo = fd % 128;

        read_block(fat,blockNo);
        found = fat[indexNo];
    }

    read_block(dataBlock, fd + 1032 );

    int boyut = sfs_getsize(fd);
    int doluluk = boyut % 1024;
    int bosluk = 1024 - doluluk;

    if (n<bosluk){
        for (int i= 1; i <= n; i++){
            dataBlock[doluluk+i] = buf[i-1];
        }
    }
    while (n>bosluk){
        for (int i= 1; i <= bosluk; i++){
            dataBlock[doluluk+i] = buf[i-1];
        }
        n -= bosluk;

        
    }


    return (0); 
}

int sfs_delete(char *filename)
{
    return (0); 
}

int * find_first_empty_file_loc (){
    int result [2];
    result[0] = -1;
    result[1] = -1; 

    for (int i = 1; i < 8; i++){
        read_block(fileInfos, i);
        for (int j = 0; j<8; j++){
            if(fileInfos[j]->isUsed == false){
                result[0] = i;
                result[1] = j;
                return result;
            }
        }
    }
    return result;
}

int * find_file(char* filename){
    int result [2];
    result[0] = -1;
    result[1] = -1; 

    for (int i = 1; i < 8; i++){
        read_block(fileInfos, i);
        for (int j = 0; j<8; j++){
            if(fileInfos[j]->name == filename){
                result[0] = i;
                result[1] = j;
                return result;
            }
        }
    }

    return result;
}

int find_empty_fat_entry (){
    int result [2];
    result[0] = -1;
    result[1] = -1; 

    for (int i = 8; i < 1032; i++){
        read_block(fat, i);
        for (int j = 0; j<128; j++){
            if(fat[j]->isUsed == false){
                result[0] = i;
                result[1] = j;
                return result;
            }
        }
    }

    return result;
}


