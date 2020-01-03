#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "simplefs.h"

// datablock icin fdye 1032 degil 1024 vermemiz gerekmiyo mu? (fd sadece fatlarin basladigi yerden basladigi icin)

int vdisk_fd; // global virtual disk file descriptor
              // will be assigned with the sfs_mount call
              // any function in this file can use this.

void *  ptr_superBlock;
fileInfo fileInfos [FILE_COUNT_PER_BLOCK];
fileDes fat [MAX_FAT_ENTRIES];
opened openFiles [MAX_NUMBER_OPEN_FILE];
void* dummy;

int numBlock = 0 ; // changed in the createVdisk
int openFileCount = 0; 
char * glob_name ; 

// This function is simply used to a create a virtual disk
// (a simple Linux file including all zeros) of the specified size.
// You can call this function from an app to create a virtual disk.
// There are other ways of creating a virtual disk (a Linux file)
// of certain size. 
// size = 2^m Bytes
int create_vdisk (char *vdiskname, int m)
{
    glob_name = vdiskname;
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
    vdisk_fd = open(glob_name,O_RDONLY);
    int n;
    int offset;
    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = read (vdisk_fd, block, BLOCKSIZE);
    //printf("HOW MANY DATA BYTES READ:        %d\n",n );
    close(vdisk_fd);
    if (n != BLOCKSIZE) {
	printf ("read error\n");
	return -1;
    }
    return (0); 
}

// write block k into the virtual disk. 
int write_block (void *block, int k)
{
    vdisk_fd = open(glob_name,O_WRONLY);
    int n;
    int offset;
    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = write (vdisk_fd, block, BLOCKSIZE);
    //printf("HOW MANY DATA BYTES WRITTEN:        %d\n",n );
    close(vdisk_fd);
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
    
    ptr_superBlock =  malloc (sizeof(superBlock));
    //void * as = malloc (sizeof(superBlock));
    ((superBlock *)ptr_superBlock)->num_of_free_data_blocks = numBlock;
    printf("free blocks %d\n", ((superBlock *)ptr_superBlock)->num_of_free_data_blocks);

    write_block(ptr_superBlock,0);

    // read_block(as,0);

    // printf("free block read         %d\n",((superBlock *)as)->num_of_free_data_blocks );

    for (int i=0; i < MAX_NUMBER_OPEN_FILE; i++ ){
        openFiles[i].openedFile = -1;
    }

    for (int i=0; i<FILE_COUNT_PER_BLOCK; i ++){
        fileInfos[i].isUsed = 0;
        fileInfos[i].name = "";
    }

    fileInfos[4].isUsed = 1;

    for (int i=1; i<=TOTAL_BLOCK_FOR_FILE; i++){
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
    if (checkName(filename) == 0 ) {
        printf("Error: filename length is too long\n");
        return (-1);
    }

    myArray fl = find_file(filename);
    if (fl.val[0] != -1 ){
        return (-1);
    }

    myArray res = find_first_empty_file_loc();
    int free_loc [2] = {res.val[0], res.val[1]};

    if(free_loc[0] == -1){
        printf("Error: Maximum file count has been exceed.\n");
        return -1;
    }

    read_block(fileInfos,free_loc[0]);

    fileInfos[free_loc[1]].isUsed = 1;
    fileInfos[free_loc[1]].name = filename;
    fileInfos[free_loc[1]].mode = -1;

    fileInfos[free_loc[1]].head = -1;

    write_block(fileInfos,free_loc[0]);

    // fileInfo as [FILE_COUNT_PER_BLOCK];
    // read_block(as,free_loc[0]);
    // printf("%s\n",as[free_loc[1]].name );

    return (0);
}


int sfs_open(char *filename, int mode){
    int desc;
    myArray res = find_file(filename);
    int file_place [2] = {res.val[0], res.val[1]};
    if(file_place[0] == -1){
       printf("Error: No such file exits.\n"); 
       return -1;
    }
    read_block(fileInfos,file_place[0]);

    fileInfo file = fileInfos[file_place[1]];

    if (file.mode != -1 ){
        printf("Error: The file is already opened.\n");
        return (-1);
    }
    file.mode = mode;

    if (openFileCount == MAX_NUMBER_OPEN_FILE ){
         printf("Error: Max limit reached for opening a file.\n");
         return (-1);
    }

    if (file.head == -1 ){

        myArray res = find_empty_fat_entry();
        int fat_d [2] = {res.val[0], res.val[1]};

        if(fat_d[0] == -1 ){
            printf("Error: No empty fat entry.\n");
            return (-1);
        }

        read_block(fat,fat_d[0]);

        fat[fat_d[1]].isUsed = 1;
        fat[fat_d[1]].file_next = -1;

        write_block(fat,fat_d[0]);

        desc = ((fat_d[0]-8) * 128) + fat_d[1];
        file.head = desc;
        fileInfos[file_place[1]] = file;

        write_block(fileInfos,file_place[0]);
        

    }

    for (int i = 0 ; i < MAX_NUMBER_OPEN_FILE ; i++){
        if (openFiles[i].openedFile == -1 ){
            openFiles[i].openedFile = desc;
            openFiles[i].name = filename;
            openFileCount += 1;
            break;
        }
    }

    printf("                    %d\n",file.head );
    fileInfo as [FILE_COUNT_PER_BLOCK];
    read_block(as,file_place[0]);
    printf("THIS IS The MODE OF OPENED FILE             %d\n",as[file_place[1]].mode);

    return file.head;
  
}

int sfs_close(int fd){

    for (int i = 0 ; i < MAX_NUMBER_OPEN_FILE; i++){
        if (openFiles[i].openedFile == fd ){
            myArray res = find_file(openFiles[i].name);
            int fil [2] = {res.val[0], res.val[1]};

            read_block(fileInfos,fil[0]);
            fileInfos[fil[1]].mode = -1;
            write_block(fileInfos,fil[0]);

            openFiles[i].openedFile = -1;
            openFileCount -= 1;
            return 0;
        }
    }
    printf("Error: There is no opened file with given descriptor.\n");
    return (-1); 
}

int sfs_getsize (int fd){
    for (int i = 0 ; i < MAX_NUMBER_OPEN_FILE; i++){
        if (openFiles[i].openedFile == fd ){
            char * file_name = openFiles[i].name;
            myArray res = find_file(file_name);
            int place [2] = {res.val[0], res.val[1]};
            read_block(fileInfos,place[0]);
            return fileInfos[place[1]].size;
        }
    }
    return (-1); 
}

int sfs_read(int fd, void *buf, int n){
    ///blockNo = fd/ 128 + 8 ????
    int blockNo = fd / 128 + 8;
    int indexNo = fd % 128;

    read_block(fat,blockNo);

    fileDes found = fat[indexNo];

    char  dataBlock  [1024] ;
    read_block(dataBlock, fd + 1032 );

    // whether data block's size is smaller than n 
    int dataSize = sfs_getsize(fd);
    if(dataSize < n){
        n = dataSize;
    }

    if(n <= 1024){
        memcpy(buf,dataBlock,n);
        // for (int i=0; i<n;i++){
        //     buf[i] = dataBlock[i];
        // }
        return n;
    }

    int count = 0;
    while (n > 1024){
        memcpy(buf + count, dataBlock, 1024);
        // for (int i=0; i<1024;i++){
        //     buf[count + i] = dataBlock[i]; 
        // }
        count += 1024;
        n = n - 1024; 
        if (found.file_next == -1)
            break;
        
        fd = found.file_next;

        read_block(dataBlock,fd+1032);
    }
    memcpy(buf+count, dataBlock, n);
    // for (int i=0; i<n;i++){
    //     buf[count + i] = dataBlock[i];
    // }
    count += n;
    return count; 
}


int sfs_append(int fd, void *buf, int n){

    int isopen = 0; 
    int sizeoffile = 0;
    int place [2];
    int temp = fd; // not to loose the place of the fd

    for (int i = 0 ; i < MAX_NUMBER_OPEN_FILE; i++){
        if (openFiles[i].openedFile == fd ){

            char * file_name = openFiles[i].name;
            myArray res = find_file(file_name);
            place [0] = res.val[0];
            place [1] = res.val[1];
            read_block(fileInfos,place[0]);
            sizeoffile = fileInfos[place[1]].size;

            isopen = 1;
        }
    }

    if(isopen == 0){
        printf("Error: The file is not open.\n");
        return (-1);
    }

    int blockNo = fd / 128 + 8;
    int indexNo = fd % 128;
    printf("%d          %d\n",blockNo,indexNo );

    read_block(fat,blockNo);
    fileDes found = fat[indexNo];
    char dataBlock [1024];

    while (found.file_next != -1 ){
        fd = found.file_next;
        blockNo = fd / 128 + 8;
        indexNo = fd % 128;

        read_block(fat,blockNo);
        found = fat[indexNo];
    }

    read_block(dataBlock, fd + 1032);

    int boyut = sfs_getsize(temp);
    int doluluk = boyut % 1024;
    int bosluk = 1024 - doluluk;

    if (n < bosluk){
        memcpy(dataBlock+doluluk, buf, n);
        // for (int i= 1; i <= n; i++){
        //     dataBlock[doluluk+i] = buf[i-1];
        // }
        sizeoffile += n;
        fileInfos[place[1]].size = sizeoffile;
        write_block(fileInfos, place[0]);
        write_block(dataBlock,fd + 1032);
        return n;
    }
    int free_blocks;
    read_block(ptr_superBlock, 0);
    free_blocks = ((superBlock *)ptr_superBlock)->num_of_free_data_blocks;
    if(n == bosluk){
        memcpy(dataBlock+doluluk, buf, n);
        // for (int i= 0; i < n; i++){
        //     dataBlock[doluluk+i] = buf[i];
        // }
        myArray res = find_empty_fat_entry();
        int newfat [2] = {res.val[0], res.val[1]};

        int fd0 = (newfat[0]-8) * 128 + newfat[1];  
        found.file_next = fd0;
        fat[indexNo] = found;
        sizeoffile += n;
        fileInfos[place[1]].size = sizeoffile;
        free_blocks -= 1;
        write_block(fat,blockNo);
        write_block(fileInfos, place[0]);
        ((superBlock *)ptr_superBlock)->num_of_free_data_blocks = free_blocks;
        write_block(ptr_superBlock, 0);  //change ptr super block than write it back 
        read_block(fat, newfat[0]);
        fat[newfat[1]].isUsed = 1;
        fat[newfat[1]].file_next = -1;

        write_block(fat, newfat[0]);
        write_block(dataBlock,fd + 1032);
        return n;
    }

    int ret_val = n;
    while (n >= bosluk){
        
        if(n > (1024 * free_blocks)){
            n = (1024 * free_blocks);
            ret_val = n;
        }
        sizeoffile += n;
        
        memcpy(dataBlock+doluluk, buf, bosluk);
        // for (int i= 0; i < bosluk; i++){
        //     dataBlock[doluluk+i] = buf[i];
        // }

        n -= bosluk;
        bosluk = 1024;
        doluluk = 0;
        free_blocks -= 1;
        myArray res = find_empty_fat_entry();
        int newfat0 [2] = {res.val[0], res.val[1]};

        int fd1 = (newfat0[0]-8) * 128 + newfat0[1];
        found.file_next = fd1;
        fat[indexNo] = found;
        write_block(fat,blockNo);
        read_block(fat, newfat0[0]);
        fat[newfat0[1]].isUsed = 1;
        fat[newfat0[1]].file_next = -1;
        found = fat[newfat0[1]];
        write_block(fat, newfat0[0]);
        blockNo = fd1 / 128 + 8;
        indexNo = fd1 % 128;
        write_block(dataBlock,fd + 1032);
        fd = fd1;

    }

    fileInfos[place[1]].size = sizeoffile;
    write_block(fileInfos, place[0]);
    ((superBlock *)ptr_superBlock)->num_of_free_data_blocks = free_blocks;
    write_block(ptr_superBlock, 0);
    
    memcpy(dataBlock+doluluk, buf, n);
    // for (int i= 1; i <= n; i++){
    //         dataBlock[doluluk+i] = buf[i-1];
    // }
    write_block(dataBlock,fd + 1032);
    return ret_val; 
}

int sfs_delete(char *filename)
{   
    myArray res = find_file(filename);
    int location [2] = {res.val[0], res.val[1]};

    if(location[0] == -1){
        printf("Error: No such file to delete.\n");
        return (-1);
    }
    int head;
    read_block(fileInfos, location[0]);
    head = fileInfos[location[1]].head;
    fileInfos[location[1]].head = -1;
    fileInfos[location[1]].isUsed = 0;
    write_block(fileInfos, location[0]);
    int blockNo = head / 128 + 8;
    int indexNo = head % 128;
    int next = 0;
    read_block(fat, blockNo);
    while (fat[indexNo].file_next != -1 ){
        fat[indexNo].isUsed = 0;
        next = fat[indexNo].file_next;
        fat[indexNo].file_next = -1;
        write_block(fat,blockNo);
        blockNo = next / 128 + 8;
        indexNo = next  % 128;
        read_block(fat,blockNo);
        
    }
    return (0); 
}

myArray find_first_empty_file_loc (){
    myArray result;
    result.val[0] = -1;
    result.val[1] = -1; 

    for (int i = 1; i < 8; i++){
        read_block(fileInfos, i);
        for (int j = 0; j<8; j++){
            if(fileInfos[j].isUsed == 0){
                result.val[0] = i;
                result.val[1] = j;
                return result;
            }
        }
    }
    return result;
}

myArray find_file(char* filename){
    myArray result;
    result.val[0] = -1;
    result.val[1] = -1; 
    
    for (int i = 1; i < 8; i++){
        read_block(fileInfos, i);
        for (int j = 0; j<8; j++){
            //printf("IS USED AFTER READ OPERATION IN FIND FILE            %s\n", fileInfos[4].name);
            if(strcmp(fileInfos[j].name, filename) == 0){
                result.val[0] = i;
                result.val[1] = j;
                return result;
            }
        }
    }
    return result;
}

myArray find_empty_fat_entry (){

    myArray result ;
    result.val[0] = -1;
    result.val[1] = -1; 

    for (int i = 8; i < 1032; i++){
        read_block(fat, i);
        for (int j = 0; j<128; j++){
            if(fat[j].isUsed == 0){
                result.val[0] = i;
                result.val[1] = j;
                return result;
            }
        }
    }


    return result;
}

int checkName(char * file_name){

    int len = strlen(file_name) + 1;
    if (len > 32)
        return 0;
    return 1;
}







