// Nagendra Upadhyay. CS360 Lab 6 Tarx.c. This lab is about Tarx which recreates a directory from a tarfile that was created with tarc, 
// including all file and directory protections and modification times. It reads the tarfile from standard input.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "dllist.h"
#include "jrb.h"

// A struct to help store and access all the necessary information using the variables and trees and list.
typedef struct TarInfo {
    JRB Inodetree;
    JRB tree;
    Dllist fileName;
    int file_mode;
    long mod_time;
} TarInfo;

// Initialize the list and trees
TarInfo* create_tarinfo() {
    TarInfo* info = (TarInfo*) malloc(sizeof(TarInfo));
    info->Inodetree = make_jrb();
    info->tree = make_jrb();
    info->fileName = new_dllist();
    // return a pointer to the newly created TarInfo struct
    return info;
}

int main() {
    // create a new TarInfo struct
    TarInfo* tar = create_tarinfo();
    // declare necessary variables
    size_t bytes;
    int size;
    long Inum;
    long fsize;
    // file status
    struct stat buf;

    // read file name, inode number, and file size from standard input until end of file is reached
    while (1) {
        // read the size of the file name from standard input
        bytes = fread(&size, 1, 4, stdin);
        // if end of file is reached break
        if (bytes == 0) {
            break;
        }
        // error reading the size of the file name
        if (bytes != 4) {
            fprintf(stderr, "Error: Could not read filename size\n");
            exit(1);
        }
        // allocate memory for the file name
        char *filename = (char *)malloc(sizeof(char) * size + 1);
        // read the file name from standard input
        bytes = fread(filename, 1, size, stdin);
        if (bytes != size) {
            // error reading the file name
            fprintf(stderr, "Error: Could not read filename\n");
            exit(1);
        }

        // Set the last character of the filename string to null terminator
        filename[size] = '\0';
    
        // Read the inode number from standard input and check if it was read successfully
        bytes = fread(&Inum, 1, sizeof(long), stdin);
        if (bytes != sizeof(long)) {
            fprintf(stderr, "Error: Could not read inode\n");
            exit(1);
        }
        
        // Try to find inode in the inode tree
        JRB inode = jrb_find_int(tar->Inodetree, Inum);
        if (inode != NULL) {
            // If inode is found, create a hard link to the existing file
            link(inode->val.s, filename);
            stat(inode->val.s, &buf);
        } else {
            // If inode is not found, create a new file or directory
            // Add inode to the inode tree
            jrb_insert_int(tar->Inodetree, Inum, new_jval_s(strdup(filename)));
            TarInfo *file_data = (TarInfo *)malloc(sizeof(TarInfo));
            // Read file mode and modification time from standard input
            bytes = fread(&file_data->file_mode, 1, 4, stdin);
            bytes = fread(&file_data->mod_time, 1, 8, stdin);
            // Add filename to the file tree and link to the TarInfo structure
            jrb_insert_str(tar->tree, filename, new_jval_v(file_data));

            if (S_ISDIR(file_data->file_mode)) {
                // If it's a directory, create a new directory with given permissions
                mkdir(filename, 0777);
            } else {
                // If it's a file, create a new file and write its content
                // Read the size of the file from standard input
                bytes = fread(&fsize, 1, 8, stdin);
                if (bytes != 8) {
                    fprintf(stderr, "Error: Could not read file size\n");
                    exit(1);
                }
                // Allocate memory for the file contents
                char *file_contents = (char *) malloc(fsize);
                if (file_contents == NULL) {
                    fprintf(stderr, "Error: Could not allocate memory for file contents\n");
                    exit(1);
                }
                // Read the file contents from standard input
                bytes = fread(file_contents, 1, fsize, stdin);
                if (bytes != fsize) {
                    fprintf(stderr, "Error: Incorrect file data read\n");
                    exit(1);
                }
                // Create a new file with given permissions and write the contents to it
                FILE *file = fopen(filename, "wb");
                if (file == NULL) {
                    fprintf(stderr, "Error: Could not create file '%s'\n", filename);
                    exit(1);
                }
                fwrite(file_contents, 1, fsize, file);
                fclose(file);
                free(file_contents);
            }
        }
        // Add the filename to the list of processed files
        dll_append(tar->fileName, new_jval_s(strdup(filename)));
    }

    // Get the current system time and store it in `mtime` variable
    time_t mtime = time(NULL);
    // Initialize pointer to inode
    JRB inode_ptr = NULL;
    // Allocate memory for times struct
    struct timeval *modTimer = malloc(sizeof(struct timeval));

    // Set the first time to current time
    modTimer[0].tv_sec = time(NULL);
    modTimer[0].tv_usec = 0;
    // Set the second time to the modification time of the file
    modTimer[1].tv_sec = mtime;
    modTimer[1].tv_usec = 0;

    // Traverse the Inodetree of tar
    jrb_traverse(inode_ptr, tar->Inodetree) {
        // Get the value associated with the current inode
        char* inode_val = (char*)inode_ptr->val.s;
        // Find the storage pointer associated with the inode value
        JRB storage_ptr = jrb_find_str(tar->tree, inode_val);
        // If the storage pointer is not found, skip to the next inode
        if (storage_ptr == NULL) {
            continue;
        }

        // Get the TarInfo struct associated with the storage pointer
        TarInfo *file_data = (TarInfo*)jval_v(storage_ptr->val);
        if (file_data == NULL) {
            continue;
        }
        // Set the second time to the modification time of the file
        modTimer[1].tv_sec = file_data->mod_time;
        // Modify the access and modification times of the file
        int utimes_result = utimes(storage_ptr->key.s, modTimer);
        // If the modification fails, print an error message to stderr
        if (utimes_result == -1) {
            fprintf(stderr, "Time modification failure\n");
        }
        // Modify the file mode (permissions) of the file
        int chmod_result = chmod(storage_ptr->key.s, file_data->file_mode);
        // If the chmod fails, print an error message to stderr
        if (chmod_result == -1) {
            fprintf(stderr, "Chmod failure\n");
        }
    }
    free(modTimer);
    return 0;
}
