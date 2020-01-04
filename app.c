#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include    <sys/time.h>
#include "simplefs.h"

#define DISKNAME "vdisk1.bin"

int main()
{
    struct timeval startTime, endTime;
    gettimeofday(&startTime, NULL);
    gettimeofday(&endTime, NULL);
    long seconds; 
    long mseconds; 
    long total; 

    int ret;
    int fd1, fd2, fd; 
    int i; 
    char buffer[1024];
    char buffer2[8] = {50, 50, 50, 50, 50, 50, 50, 50};
    int size;
    char c; 

    printf ("started\n"); 
    
    // ****************************************************
    // if this is the first running of app, we can
    // create a virtual disk and format it as below
    ret  = create_vdisk (DISKNAME, 24); // size = 16 MB
    if (ret != 0) {
        printf ("there was an error in creating the disk\n");
        exit(1); 
    }
    ret = sfs_format (DISKNAME);
    if (ret != 0) {
        printf ("there was an error in format\n");
        exit(1); 
    }
    // ****************************************************

    ret = sfs_mount (DISKNAME); 
    if (ret != 0) {
        printf ("could not mount \n");
        exit (1); 
    }


    printf ("creating files\n"); 
    gettimeofday(&startTime, NULL);
    sfs_create ("file1.bin");
    sfs_create ("file2.bin");
    sfs_create ("file3.bin");
    sfs_create ("file4.bin");
    sfs_create ("file5.bin");

    gettimeofday(&endTime, NULL);
    seconds = endTime.tv_sec - startTime.tv_sec;
    mseconds= endTime.tv_usec - startTime.tv_usec;
    total = (seconds * 1000000) + mseconds;
    printf("Files Created\n");
    printf("File Creation took %ld microseconds\n", total);

    fd1 = sfs_open ("file1.bin", MODE_APPEND);
    fd2 = sfs_open ("file2.bin", MODE_APPEND); 
    for (i = 0; i < 10000; ++i) {
    buffer[0] =   (char) 65;  
    sfs_append (fd1, (void *) buffer, 1);
    }

    for (i = 0; i < 10000; ++i) {
        buffer[0] = (char) 70;
        buffer[1] = (char) 71;
        buffer[2] = (char) 72;
        buffer[3] = (char) 73;
        sfs_append(fd2, (void *) buffer, 4);
    }
    
    sfs_close(fd1); 
    sfs_close(fd2); 

    printf("APPEND STARTED\n");
    gettimeofday(&startTime, NULL);

    fd = sfs_open("file3.bin", MODE_APPEND);
    for (i = 0; i < 100; ++i) {
        memcpy (buffer, buffer2, 5); // just to show memcpy
        sfs_append(fd, (void *) buffer, 5); 
    }
    sfs_close (fd);

    gettimeofday(&endTime, NULL);
    seconds = endTime.tv_sec - startTime.tv_sec;
    mseconds= endTime.tv_usec - startTime.tv_usec;
    total = (seconds * 1000000) + mseconds;
    printf("APPEND ENDED\n");
    printf("APPEND %ld microseconds\n", total);

    printf("READ STARTED\n");
    gettimeofday(&startTime, NULL);

    fd = sfs_open("file3.bin", MODE_READ);

    size = sfs_getsize (fd);
    printf("%d\n",size );
    for (i = 0; i < size; ++i) {

        sfs_read (fd, (void *) buffer, 1);
        c = (char) buffer[0];
    }
    sfs_close (fd); 

    gettimeofday(&endTime, NULL);
    seconds = endTime.tv_sec - startTime.tv_sec;
    mseconds= endTime.tv_usec - startTime.tv_usec;
    total = (seconds * 1000000) + mseconds;
    printf("READ ENDED\n");
    printf("READ took %ld microseconds\n", total);
    
    ret = sfs_umount(); 
}