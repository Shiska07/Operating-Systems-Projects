#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <dirent.h>

#define MAX_COMMAND_SIZE       255
#define NUM_BLOCKS             4226
#define BLOCK_SIZE             8192
#define MAX_BLOCKS_PER_FILE    1250
#define NUM_FILES              128
#define NUM_INODES             128
#define MAX_FILENAME_SIZE      32
#define MAX_ARGS               11
#define SINGLE_COMMAND_SIZE    32
#define TOK_DELIM              " \r\n\t\a"


// this is a 2D array where every single block is a byte
uint8_t blocks[NUM_BLOCKS][BLOCK_SIZE];
char cwd[MAX_COMMAND_SIZE];                  // to store curr working
char fs_name[255];                           // stored file system name of current fs
char path[1000];
uint8_t fs_open;                             // 1 means a file system image is open 0 means not open

struct _DirectoryEntry
{
    char        name[MAX_FILENAME_SIZE];               // filename
    uint8_t     valid;                              // if the file exists
};

struct _Inode
{
    int16_t    fblocks[MAX_BLOCKS_PER_FILE];             // stores block numbers

    char     date[32];
    time_t   time;
    int      size;

    // any file written will be read only and visible by default unless changed
    uint8_t  attrib_r;
    uint8_t  attrib_h;
};


static uint8_t *inode_map;                       // stores inode avaialbility 0:unavailable 1:available
static uint8_t *block_map;                       // stores block avaialbility 0:unavailable 1:available

// an array of pointers to inodes
struct _Inode *inode_array[NUM_INODES+3];         // 0, 1 and 2 will be left empty
struct _DirectoryEntry *dir;

// this function returns the number of bytes free in the file system 
int df()
{
    int count = 0;
    int i;

    for( i = 131; i < NUM_BLOCKS; i++)
    {
        if(block_map[i] == 1)
        {
            count++;       // increment count if block is free
        }    
    }
    return count*BLOCK_SIZE;
}

// initalizes a 2D array into a file system image
void initailize_fs()
{

    dir = (struct _DirectoryEntry *)&blocks[0][0];
    inode_map = (uint8_t *)&blocks[1][0];
    block_map = (uint8_t *)&blocks[2][0];

    int i;
    for(i = 3; i < NUM_FILES+3; i++)
    {
        dir[i].valid = 0;                       // set files to not valid
        inode_map[i] = 1;                  // set inodes to free
        
        // initialize inode
        inode_array[i] = (struct _Inode *)&blocks[i][0];
        inode_array[i]->attrib_r = 0;
        inode_array[i]->attrib_h = 0;

        int j;
        for(j = 0; j < MAX_BLOCKS_PER_FILE; j++)
        {
            inode_array[i]->fblocks[j] = -1;
        }
    }

    for( i = 131; i < NUM_BLOCKS; i++)
    {
        // label all avaiable blocks as free
        block_map[i] = 1;
    }
}

// this function returns the index of a free block [131 - 4225]
int find_free_block()
{
    int i;
    for(i = 131; i < NUM_BLOCKS; i++)
    {
        if(block_map[i] == 1)
        {
            // returning index because it directly corresponds 
            // to the index of free block in blocks [131 - 4225]
            return i;
        }
    }
    return -1;
}

// this function returns the index of a free inode [3 - 130]
int find_free_inode()
{
    int i;
    for(i = 3; i < NUM_INODES+3; i++)
    {
        if(inode_map[i] == 1)
        {
            // returning index because it directly corresponds to the
            // location of the inode in blocks [3 - 130]
            return i;
        }
    }
    return -1;
}

// given a filename that exists in the image, this function returns the 
// inode_index of the file in inode_array
int find_file_inode_index(char *filename)
{
    int i;
    for(i = 3; i < NUM_FILES+3; i++)
    {
        if(strcmp(filename,dir[i].name) == 0)
        {
            // returning index because directory index corresponds to 
            // the file inode index i.e. dir[3] has info for inode at
            // inode_array[3]
            return i;
        }
    }
    return -1;
}

// this function lists the information of all files in file system image
void list(int attrib)
{
    int count = 0;
    int i;
    for( i = 3; i < NUM_FILES+3; i++)
    {
        if (dir[i].valid == 1)
        {
            // list all hidden files
            if (attrib == 1)
            {
                printf("%-7d %s    %s.\n",inode_array[i]->size, inode_array[i]->date, dir[i].name);
                count++;
            }
            // list only non-hidden files
            else if (attrib == 0 && inode_array[i]->attrib_h == 0)
            {
                printf("%-7d %s    %s.\n",inode_array[i]->size, inode_array[i]->date, dir[i].name);
                count++;
            }
        }
    }
    if (count == 0)
    {
        printf("list: no files found.\n");
    }
}

char *string_reverse(char *str, char *rev_str)
{
    int length = strlen(str);
    int j = length - 1;
    int i;
    rev_str = (char *)malloc(sizeof(char) * length+1);
    for(i = 0; i<length; i++)
    {
        rev_str[i] = str[j];
        j--;
    }
    rev_str[length] = '\0';
    return rev_str;
}

// changes file attribute
int attrib(char *filename, char *attrib)
{
    if(filename == NULL || attrib == NULL)
    {
        printf("Command not found.\n");
        return 1;
    }
    // attribute for a file is present in its inode
    int file_inode = find_file_inode_index(filename);
    if(file_inode == -1)
    {
        printf("attrib error: File does not exist.\n");
        return 1;
    }
    // if inode is in use
    if(inode_map[file_inode] == 0)
    {
        if(strcmp(attrib,"+h") == 0)
        {
            inode_array[file_inode]->attrib_h = 1;
        }
        else if(strcmp(attrib,"-h") == 0)
        {
            inode_array[file_inode]->attrib_h = 0;
        }
        else if(strcmp(attrib,"+r") == 0)
        {
            inode_array[file_inode]->attrib_r = 1;
        }
        else if(strcmp(attrib,"-r") == 0)
        {
            inode_array[file_inode]->attrib_r = 0;
        }
        else
        {
            printf("attrib error: Unknown attribute %s.\n",attrib);
        }
        return 1;
    }
}

// deletes a file from the file system image
int del(char *filename)
{
    int file_inode_index = find_file_inode_index(filename);
    if(file_inode_index == -1)
    {
        printf("del error: File does not exist.\n");
    }
    // if the file is read only
    else if(inode_array[file_inode_index]->attrib_r == 1)
    {
        printf("del error: File marked read only.\n");
    }
    else
    {
        // set directory as not valid
        dir[file_inode_index].valid = 0;

        // set inode as available
        inode_map[file_inode_index] = 1;
        int i;
        for( i = 0; i < MAX_BLOCKS_PER_FILE; i++ )
        {
            if (inode_array[file_inode_index]->fblocks[i] == -1)
            {
                break;
            }

            // set block as free
            block_map[inode_array[file_inode_index]->fblocks[i]] = 1;

            // restore inode fblocks for new files
            inode_array[file_inode_index]->fblocks[i] = -1;
        }
    }
    return 1;
}

// creates a new file with name = file name on the file system image
int createfs(char* name)
{
    if(name == NULL)
    {
        printf("Createfs error: file not found.\n");
        return 1;
    }
    else
    {
        // update fs_name
        strcpy(fs_name, name);
    }

    // initialize and open file system
    initailize_fs();
    fs_open = 1;

    return 1;
}

// copies a file from current working directory into the file system image
int put(char *filename, char* filepath)
{    
    // call stat to get file size and verify if the file exists
    int status;
    struct stat buffer;

    // if path provided search in path if not search in cwd
    if(filepath == NULL)
    {
        status = stat(filename, &buffer);
    }
    else
    {
        status = stat(filepath, &buffer);
    }
    
    // if file cannot be found
    if (status == -1)
    {
        printf("put error: File does not exist.\n",filename);
        return 1;
    }
    else
    {   
        // if file size is greater than disk space available
        if ((int)buffer.st_size > df())
        {
            printf("put error: Not enough disk space.\n",filename);
        }
        // if filename is too long
        else if(strlen(filename) > MAX_FILENAME_SIZE)
        {
            printf("put error: File name too long.\n");
        }
        else
        {     
            
            // find a free inode for the file and update inode map
            int free_inode_index = find_free_inode();

            // save file size
            int copy_size = (int)buffer.st_size;

            // if no inodes available
            if (free_inode_index == -1)
            {
                printf("put error: No free inodes available.\n");
                return 1;
            }

            // set inode index as unavailable and direcotry entry as valid
            inode_map[free_inode_index] = 0;
            dir[free_inode_index].valid = 1;

            // save timestamp
            time_t curr_time = buffer.st_mtim.tv_sec;
            
            // start adding info to inode
            inode_array[free_inode_index]->time = curr_time;
            
            strncpy(inode_array[free_inode_index]->date, ctime( &curr_time), strlen(ctime( &curr_time))-1);
            inode_array[free_inode_index]->size = copy_size;

            // save file name
            strncpy(dir[free_inode_index].name, filename, strlen(filename));

            FILE *fp = NULL;

            // read the file and store it in blocks into the file system image
            if(filepath == NULL)
            {
                fp = fopen(filename, "r");
            }
            else
            {
                fp = fopen(filepath, "r");
            }

            int b = 0;
            int offset = 0;
            int free_block_index = 0;

            while (copy_size > 0)
            {
                
                // we have to move the file pointer acoording to the block size 
                fseek(fp, offset, SEEK_SET);
                free_block_index = find_free_block();
                
                // read block from file and store into disk image
                int bytes = fread(blocks[free_block_index], BLOCK_SIZE, 1, fp);
                
                // set block as unavailable abd store block info
                block_map[free_block_index] = 0;
                inode_array[free_inode_index]->fblocks[b] = free_block_index;
                b++;

                if (bytes == 0 && !feof(fp))
                {
                    printf("An error occured while reading from input file.\n");
                    return 1;
                }

                clearerr(fp);
                copy_size -= BLOCK_SIZE;
                offset += BLOCK_SIZE;
            }
            fclose(fp);
        }
        return 1;
    }
}

// saves file in system image to local directory
int get(char *filename, char *local_filename)
{
    
    int file_inode_index;

    if(filename != NULL)
    {
        file_inode_index = find_file_inode_index(filename);
    }
    else
    {
        printf("Command not found.\n");
        return 1;
    }
    // if file does not exist in the image
    if (file_inode_index == -1)
    {
        printf("get error: File not found.\n");
        return 1;
    }

    FILE *ofp;
    int copy_size = 0;
    int block_index = 0;
    int offset = 0;

    // if new filename not provided
    if(local_filename == NULL)
    {
        // use filename
        ofp = fopen(filename, "w");
    }
    else
    {
        ofp = fopen(local_filename, "w");
    }

    if(ofp == NULL)
    {
        printf("Could not open output file: %s\n", local_filename );
        return 1;
    }

    copy_size = inode_array[file_inode_index]->size;

    int num_bytes;
    int i = 0;

    while(copy_size > 0)
    {
        
        // only copy required number of bytes
        if( copy_size < BLOCK_SIZE )
        {
            num_bytes = copy_size;
        }
        else 
        {
            num_bytes = BLOCK_SIZE;
        }

        block_index = inode_array[file_inode_index]->fblocks[i];

        // no more blocks remaining
        if(block_index == -1)
        {
            break;
        }

        fwrite(blocks[block_index], num_bytes, 1, ofp);

        // decrease remaining number of bytes and increase offset
        copy_size -= BLOCK_SIZE;
        offset += BLOCK_SIZE;
        
        // move to next block
        i++;

        clearerr(ofp);
        fseek(ofp, offset, SEEK_SET);
    }
    fclose(ofp);
    return 1;
}

// saves file system image to the disk in a directory
int save(char *arg1)
{
    int ret1, ret2, ret3, ret4, i;
    struct dirent *file;
    
    if(arg1 != NULL)
    {
        printf("Command not found.\n");
        return 1;
    }

    strcpy(path,cwd);
    strcat(path,"/");
    strcat(path,fs_name);
    strcat(path,"/");

    // store directory ptr to check if directory already exists
    DIR* fs_dir = opendir(fs_name);

    // directory exists
    if(fs_dir)
    {
        while( (file = readdir(fs_dir)) )
        {
            if(strcmp(file->d_name,".") != 0 && strcmp(file->d_name,"..") != 0)
            {
                strcat(path,file->d_name);
                ret1 = remove(path);
                if(ret1 == -1)
                {
                    printf("save error: File system could not be overwritten.\n");
                    return 1;
                }
            }
            strcpy(path,cwd);
            strcat(path,"/");
            strcat(path,fs_name);
            strcat(path,"/");
        }
         // close and remove direcotry
        closedir(fs_dir);
        ret2 = rmdir(fs_name);

        if(ret2 == -1)
        {
            printf("save error: File system could not be overwritten.\n");
            return 1;
        }

        // create direcotry
        ret3 = mkdir(fs_name,0777);
        if(ret1 == -1)
        {
            printf("save error: File system could not be saved to disk.\n");
            return 1;
        }
    }
    else
    {
        // create direcotry
        ret3 = mkdir(fs_name,0777);
        if(ret1 == -1)
        {
            printf("save error: File system could not be saved to disk.\n");
            return 1;
        }
    }

    for(i = 3; i < NUM_FILES+3; i++)
    {
        strcpy(path,cwd);
        strcat(path,"/");
        strcat(path,fs_name);
        strcat(path,"/");

        // if inode is in use
        if(inode_map[i] == 0)
        {
            strcat(path,dir[i].name);
            ret4 = get(dir[i].name, path);
        }
    }
    return 1;
}

int open(char* dir_name)
{

    DIR* fs_dir = opendir(dir_name);
    char filepath[1000];
    char filename[32];

    // if name not provided or directory does not exist
    if(dir_name == NULL || fs_dir == NULL)
    {
        printf("open error: file not found.\n");
        return 1;
    }

    // call stat to get directory size
    int status;
    struct stat buffer;
    status = stat(dir_name, &buffer);

    // if too large to be loaded
    if(buffer.st_size > NUM_FILES*BLOCK_SIZE)
    {
        printf("open error: File system could not be opened: Insufficient memory.\n");
        return 1;
    }

    // initualise fs and update fs name
    initailize_fs();
    strcpy(fs_name, dir_name);

    struct dirent *file;

    while( (file = readdir(fs_dir)) )
    {
        if(strcmp(file->d_name,".") != 0 && strcmp(file->d_name,"..") != 0)
        {
            strcpy(filename, file->d_name);
            strcpy(filepath, cwd);
            strcat(filepath, "/");
            strcat(filepath, dir_name);
            strcat(filepath, "/");
            strcat(filepath, filename);
            put(filename, filepath);
        }      
    }

    // set file system as open
    fs_open = 1;
    closedir(fs_dir);
    return 1;
}

// closes current file system image without saving
int close_fs(char *dir_name)
{
    if(dir_name == NULL || strcmp(fs_name,  dir_name) != 0)
    {
        printf("close error: This file system is not open.\n");
        return 1;
    }
    else
    {
        // re initualize close file system
        fs_open = 0;
    }
    return 1;
}

// reads single line of command from the shell
char *read_single_line()
{
    // save number of characters to resize buffer
    int no_of_chars = 0;
    char c;

    //set initial buffer size
    size_t buffersize = MAX_COMMAND_SIZE;
    char *buffer = (char *)malloc(sizeof(char) * buffersize);

    if (buffer == NULL)
    {
        fprintf(stderr,"allocation failure\n");
        exit(EXIT_FAILURE);
    }

    do
    {   
        c = getchar();
        // if the first char entered is return, the user will 
        // be prompted to re-enter the command 
        if(c == '\n' && no_of_chars == 0)
        {
            printf("mfs> ");
            // start loop from top as no command has been entered
            continue;
        }
        else if(c == '\n')
        {
            // exit loop as command is complete
            break;
        } 
        buffer[no_of_chars] = c;
        no_of_chars++;

        // if command size is equal to buffer size, resize
        if(no_of_chars == buffersize)
        {
            buffersize = buffersize * 2;
            buffer = (char *)realloc(buffer, sizeof(char) * buffersize);
        }
    }while(1);

    // add null character to end of string
    buffer[no_of_chars] = '\0';
    buffer = strtok(buffer,"\n");
    return buffer;
}

//this function takes a single line of command line arguments and breaks it into a array of arguments
char **get_args(char *single_line)
{
    int str_len, i, j;
    // stores a copy of single_line
    char *single_line_copy;
    char **args;

    // command will only be parsed if no of parameters is within limit
    // parse status will be set to 1 if no of parameters is < 11
    int parse_status = 0;   

    do
    {
        // count number of arguments so it does not exceed MAX_ARGS
        int no_of_args = 0;
        str_len = strlen(single_line);

        // allocate memory to copy single_line as strtok destroys
        // the string being parsed and we will use strtok twice 
        single_line_copy = (char *)malloc(sizeof(char) * (str_len+1));
        strcpy(single_line_copy,single_line);

        // use strtok on single_line_copy to count number of arguments
        char *token = NULL;
        char *token2 = NULL;
        token = strtok(single_line_copy,TOK_DELIM);
        while (token != NULL)
        {
            no_of_args++;
            token = strtok(NULL,TOK_DELIM);     
        }

        // if number of arguments exceeds MAX_ARGS, prompt user to re-enter the command
        if(no_of_args > MAX_ARGS)
        {
            ("msh: too many arguments: max allowed 11\n");
            printf("mfs> ");
            single_line = read_single_line();
            single_line = strtok(single_line,"\n");
            free(single_line_copy);
        }
        else
        {
            // allocate memory to save arguments as an array of
            // strings followed by NULL characters
            args = malloc(sizeof(char *) * (MAX_ARGS + no_of_args));   
            token2 = strtok(single_line,TOK_DELIM);
            args[0] = (char *)malloc(sizeof(char) * SINGLE_COMMAND_SIZE);
            strcpy(args[0],token2);
            for(i = 1; i <= no_of_args; i++)
            {
                token2 = strtok(NULL,TOK_DELIM);
                if(token2 == NULL)   // if current argument is the last argument
                {
                    // add as many NULL characters at end as
                    // the number of arguments
                    for(j = 0; j < no_of_args; j++)
                    {
                        args[i+j] = NULL;
                    }
                    // set parse status as complete
                    parse_status = 1;  
                }
                else{
                    args[i] = (char *)malloc(sizeof(char) * SINGLE_COMMAND_SIZE);
                    strcpy(args[i],token2);
                }
            }
        }    
    }while(!parse_status);

    free(single_line_copy);

    return args;
}

// this function runs the command line arguments passed as an array of strings, keeps track of the last 15 pids and commands
int run_args(char **args)
{
    
    if(strcmp(args[0], "exit") == 0 || strcmp(args[0], "quit") == 0)
    {
        // exit code here
        return 0;
    }
    else if(strcmp(args[0], "open") == 0)
    {
        return open(args[1]);
    }
    else if(strcmp(args[0], "createfs") == 0)
    {
        return createfs(args[1]);
    }
    else
    {
        if(fs_open == 1)
        {
            if(strcmp(args[0], "put") == 0)
            {
                // display history and exit normally
                return put(args[1], NULL);
            }
            else if(strcmp(args[0], "df") == 0)
            {
                if(args[1] == NULL)
                {
                    // display pids and exit normally
                    printf("%d bytes free.\n",df());
                }
                else
                {
                    printf("df: Command not found.\n");
                }
            }
            else if(strcmp(args[0], "list") == 0)
            {
                
                if(args[1] == NULL)
                {
                    // list regular
                    list(0);
                }
                else if(strcmp(args[1], "-h") == 0)
                {
                    // list hidden files
                    list(1);
                }
                else
                {
                    printf("list: Command not found.\n");
                }
            }
            else if(strcmp(args[0], "del") == 0)
            {
                return del(args[1]);
            }
            else if(strcmp(args[0], "attrib") == 0)
            {
                return attrib(args[2], args[1]);
            }
            else if(strcmp(args[0], "get") == 0)
            {
                return get(args[1], args[2]);
            }
            else if(strcmp(args[0], "save") == 0)
            {
                return save(args[1]);
            }
            else if(strcmp(args[0], "close") == 0)
            {
                return close_fs(args[1]);
            }
            else
            {
                printf("Command not found.\n");
            }
        }
        else
        {
            printf("No open file systems found.\ncreate one using: 'createfs <file system image name>'\nor open an existing file system using: 'open <file system imaeg name>'.\n");
        }
    }   
    return 1;
}

// frees all elements of a 2D array allocated dynamically
int free_array(char **arr)
{
    int i;
    for(i = 0;arr[i] != NULL;i++)
    {
        free(arr[i]);
    }
    free(arr);
    return 1;
}

int main(int argc, char *argv[])
{
    
    int   status;                     // will remain 1 until "exit" or "quit"
    char *single_line;                // will hold single line of command enterted
    char **args;                      // single line command will be parsed and stored in args         

    // store current working directory
    getcwd(cwd,sizeof(cwd));            

    status = 1;
    fs_open = 0;

    do
    {
        printf("mfs> ");

        // read single line of command
        single_line = read_single_line();

        // break commands into an array of strings
        args = get_args(single_line);

        status = run_args(args);
        
        // free dyamically allocated variables
        free(single_line);

        free_array(args);

    }while(status);

    return 0;
}
