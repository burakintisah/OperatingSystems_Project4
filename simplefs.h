
#define MODE_READ 0
#define MODE_APPEND 1

#define BLOCKSIZE 1024 // bytes

#define MAX_FILENAME_LEN    32
#define MAX_FAT_ENTRIES     128   // 128 
#define FAT_ENTRIES_COUNT   1024
#define MAX_FILE            56
#define FILE_COUNT_PER_BLOCK 8
#define TOTAL_BLOCK_FOR_FILE 7
#define HEAD_DATA           1032
#define MAX_NUMBER_OPEN_FILE 10

typedef struct{
    //update num of file and num of blocks
    // int num_of_file;
    // Data info
    //int num_of_blocks;
    int num_of_free_data_blocks;
    int dummy [255];

} superBlock;


typedef struct{
    // 128 B
    char * name ;    // name of file
    int  isUsed;                     // whether file is used
    int  mode;                       // read or write
    int  size;                       // size of file
    //int  blocks_count;               // number of blocks
    //int  num_fd;                     // # of file descriptors using this file
    int  head;                       // index of first entry in fat table
    int dummy [25]; 

} fileInfo;

/* file descriptor */
typedef struct{
    // 8 B
    int isUsed ;                 // whether the file descriptor is being used
    int file_next;               // next data block to read

} fileDes;

typedef struct {
    char * name ; 
    int  openedFile;
    int offset;
} opened;

typedef struct {
    int val[2];
}myArray;

typedef struct {
   char name [32];
   int len;
}structName;

int create_vdisk (char *vdiskname, int m);
/* 
   This function will be used to create a virtual disk (as simple Linux file)
   of certain size. The name of the Linux file is vdiskfilename. 
   The parameter m is used to set the size. 
   Size will be 2^m bytes. If success, 0 will returned; if error, -1
   will be returned. 
*/

int sfs_format (char *vdiskname);
/*
  This function will be used to initialize/create 
  an sfs file system on the virtual disk (high-level formatting the disk). 
  On disk file system structures (like superblock, FAT, etc.) will be 
  initialized as part of this call. 
  If success, 0 will be returned. If error, -1 will be returned.
 */  


int sfs_mount (char *vdiskname);
/* 
   This function will be used to mount the file system, i.e., to 
   prepare the file system be used. This is a simple operation. 
   Basically, it will open the regular Linux file (acting as the virtual disk) 
   and obtain an integer file descriptor.  Other operations in the library 
   will use this file descriptor. This descriptor will be a global variable 
   in the library. If success, 0 will be returned; if error, -1 will 
   be returned. 
 */ 

int sfs_umount (); 
/* 
   This function will be used to unmount the file system: flush the 
   cached data to disk and close the virtual disk (Linux file) file 
   descriptor.  
   if success, 0 will be returned, if error, -1 will be returned.
 */ 

int sfs_create(char *filename);
/* 
   With this, an applicaton will create a new file with name 
   filename. You library implementation of this function will 
   use an entry in the root directory to store information 
   about the created file, like its name, size, first data block
   number, etc. 
   If success, 0 will be returned. If error, -1 will be returned.  
 */

int sfs_open(char *filename, int mode);
/*
   With  this an application will open a file. The name of the 
   file to open is filename. The mode paramater specifies if the 
   file will be opened in read-only mode or in append-only mode. 
   if 0, read-only; if 1, append-only. We can either  
   read the file or append to it. A file can not be opened for both
   reading and appending at the same time. In your library you 
   should have a open file table and entry in that table will be 
   used for the opened file. The index of that entry can be returned
   as the return value of this function. 
   Hence the return value will be a non-negative integer acting as a
   file descriptor to be used in subsequent file operations. 
   if error, -1 will be returned. 
 */

int sfs_close(int fd);
/* 
   With this an application will close a file whose descriptor is fd.
   The related open file table entry should be marked as free. 
*/

int sfs_getsize (int fd);
/* 
   With the an application learns the size of the file in bytes. 
   A file witn no content has size 0. 
   Returns the number of data bytes in the file. If error, returns -1. 
*/

int sfs_read(int fd, void *buf, int n);
/* 
   With this, an application can read data from a file. fd is the
   file descriptor. buf is pointing to a memory area for which 
   space is allocated earlier with malloc (or it can be a static array). 
   n is the amount of data to read. Upon failure, -1 will be returned. 
   Otherwise, number of bytes sucessfully read will be returned. 
 */


int sfs_append(int fd, void *buf, int n);
/* 
   With this, an application can append new data to the file. The 
   parameter fd is the file descriptor. The parameter buf is pointing to (i.e., 
   is the address of) a static array holding the data or a dynamically 
   allocted memory space holding the data. The parameter n is the size of 
   the data to write (append) into the file. Upon failure, will return -1. 
   Otherwise, the number of bytes successfully appended will be returned. 
 */ 


int sfs_delete(char *filename);
/* 
   With this, an application can delete a file. The name of the
   file to be deleted is filename. If succesful, 0 will be returned. 
   In case of an error, -1 will be returned. 
*/ 

// HELPER FUNCTIONS

myArray find_first_empty_file_loc ();
myArray find_file ();
myArray find_empty_fat_entry ();
int checkName(); 



