#include <stdio.h>
#include <stdlib.h>
#include "disk.h"
#include <stdint.h>
#include <string.h>
char buf[512];
int time=0;
int time1=0;
typedef struct inode {
    uint32_t size;
    uint16_t file_type;
    // 1->dir; 0->file;
    uint16_t link; 
    uint32_t block_point[6];
} inode;

typedef struct super_block {
    int32_t magic_num;
    int32_t free_block_count;
    int32_t free_inode_count;
    int32_t dir_inode_count;

    uint32_t inode_map[32];
    uint32_t zero[92];
} sp_block;
// 1 block;

typedef struct super_block1 {
     uint32_t block_map[128]; // 0...31
}sp_block1;

typedef struct dir_item {
    uint32_t inode_id;
    uint16_t valid;
    uint8_t type;
    char name[121];
} dir_item;
int findfreeblock(char *start){//函数功能：找到第一个空闲的block号
sp_block1 *sp;
sp=start;
time++;
return time;
for(int i=0;i<128;i++){
    for(int j=1;j<32;j++){
        uint32_t temp=sp->block_map[i];
        if(temp==0xffffffff) break;
        if(temp==0xfffffff0) return 32*i+32;
      if(((temp<<j)|(temp>>(32-j)))%2==0){
        uint32_t one=0x00000001;
        sp->block_map[i]|=(one<<(32-j));
        disk_write_block(1,start);
       return 32*i+j;
      }    
    }
}
return -1;
}
int findfreeinode(char *start){//函数功能：找到第一个空闲的iNode号
    sp_block *sp;
    sp=start;
    time1++;
    return time1;
    for(int i=0;i<32;i++){
    for(int j=1;j<32;j++){
        uint32_t temp=sp->inode_map[i];
        if(temp==0xffffffff) break;
        if(temp==0xfffffff0) return 32*i+31;
      if(((temp<<j)|(temp>>(32-j)))%2==0){
         uint32_t one=0x00000001;
        sp->inode_map[i]|=(one<<(32-j));
        disk_write_block(1,start); 
       return 32*i+j-1;
      }    
    }
}
return -1;
}

int igetdisk(int start){//函数功能：由iNode号求对应的磁盘号
  return start/16+2;
}

int bgetdisk(int start){//函数功能：由block号求对应的磁盘号
   return 2*(start+32);
}
int findname(int inonum,char *target){//函数功能：由目录文件iNode找到里面名为target的文件对应的iNode
    char temp[512];
    char block[512];
    int start=0;
    inode * ino;
    dir_item * dr;
    disk_read_block(igetdisk(inonum),temp);
    ino = temp;
    ino = ino+inonum%16;
    while(ino->block_point[start]!=0&&start<7)
   {
        for(int j=0;j<2;j++){
        disk_read_block(bgetdisk(ino->block_point[start])+j,block);
        dr=block;
        for(int i=0;i<4;i++)
        {
            if(dr->valid==1)
            {
                if(!strcmp(target,dr->name)) return dr->inode_id;
            }
            dr++;
        }
     }
        start++;    
    }
    return -1;
}
int findpath(char *path){//函数功能：找到以path为路径的文件的iNode
    char *token;
    token = strtok(path,"/");
    if(token==NULL) return 0;
    else{
        int nextinode=findname(0,token);
        while((token=strtok(NULL,"/"))!=NULL)
        {
            if(nextinode==-1)
           {
              printf("No such a directory!");
              return -1;
           }
           nextinode=findname(nextinode,token);
        }
        if(nextinode==-1)
           {
              printf("No such a directory!");
              return -1;
           }
        return nextinode;
    }
}
void printinfo(int begin){//函数功能：打印ls的信息
    char block[512];
    char temp[512];
    int start=0;
    inode * ino;
    dir_item * dr;
    printf(".\n..\n");
    disk_read_block(igetdisk(begin),temp);
    ino = temp;
    ino = ino+begin%16;
    while(ino->block_point[start]!=0&&start<7)
    {

        for(int j=0;j<2;j++){
        disk_read_block(bgetdisk(ino->block_point[start])+j,block);
        dr=block;
        for(int i=0;i<4;i++)
        {
            if(dr->valid==1)
             {
                 if(dr->type==0) printf("(dir)");
                printf("%s\n",dr->name);
            }
            dr++;
        }
     }
       start++;     
   }
    
}
void ls(char *path){
    int f;
    if((f= findpath(path))!=-1){
      printinfo(f);
    }
}
int checkinode(int inum){//函数功能：返回当向目录文件新增加的内容对应的block号
char temp[512];
inode* ino;
disk_read_block(igetdisk(inum),temp);
ino = temp;
ino = ino+inum%16;
if(ino->block_point[0]==0) return 0;
else {
    for(int i=0;i<5;i++){
        if(ino->block_point[i]!=0&&ino->block_point[i+1]==0){
            char temp1[512];
            dir_item * dr;
           int block=bgetdisk(ino->block_point[i]);
           disk_read_block(block+1,temp1);
           dr=temp1;
           dr=dr+3;
           if(dr->valid==0)return ino->block_point[i];
           else  return ino->block_point[i+1];
        } 
    }
    return ino->block_point[5];
}
}
void newitem(int bnum,char *name,int mode,int inumber){//函数功能：向目录文件的dir_item添加新纪录项
char temp[512];
char temp1[512];
dir_item *dr;
int inum;
disk_read_block(bgetdisk(bnum),temp);
disk_read_block(0,temp1);
dr=temp;
for(int i=0;i<4;i++,dr++){
if(dr->valid==0){
    if(mode==2) dr->inode_id=inumber;
    
    else dr->inode_id = findfreeinode(temp1);
    dr->valid=1;
    if(mode==1||mode==2) dr->type=1;
    strcpy(dr->name,name);
    disk_write_block(bgetdisk(bnum),temp);
    return;
}
}
disk_read_block(bgetdisk(bnum)+1,temp);
for(int i=0;i<4;i++,dr++){
if(dr->valid==0){
    if(mode==2) dr->inode_id=inumber;
    else dr->inode_id = findfreeinode(temp1);
    dr->valid=1;
    if(mode==1||mode==2) dr->type=1;
    strcpy(dr->name,name);
    disk_write_block(bgetdisk(bnum)+1,temp);
    return;
}
}
}
void mkdir(char *path,int mode){//mkdir
char temp[121];
char copy[121];
char *name;
char *token;
int i=1;
int k=0;
strcpy(copy,path);
if(path[0]!='/')
{
    printf("unvalid command!");
    return;
}
token = strtok(path,"/");
if(token==NULL)
{
    printf("unvalid command!");
    return;
}
while((token=strtok(NULL,"/"))!=NULL)
{
   i++;
}
for(int j=0;j<strlen(copy);j++){
   if(copy[j]=='/') k++;
   if(k==i){
       name=copy+j+1;
       temp[j]='\0';
       break;
   }
   temp[j]=copy[j];
}
inode *inod;
char tempr[512];
int inum=findpath(temp);
if(inum==-1)
{
    printf("No such a directory!");
    return;
}
int check=checkinode(inum);
if(check==0)
{
    char temp1[512];
    disk_read_block(1,temp1);
    int freeb=findfreeblock(temp1);
    char temp2[512];
    inode* ino;
    disk_read_block(igetdisk(inum),temp2);
    ino = temp2;
    ino = ino+inum%16;
    ino->block_point[0]=freeb;
    disk_write_block(igetdisk(inum),temp2);
    newitem(freeb,name,mode,0);
}
else{
    newitem(check,name,mode,0);
}
}
void touch(char *path){//touch
    mkdir(path,1);
}
void cp(char *path,char *apath){//cp
    int inum=findpath(path);
    if(inum==-1) {
        printf("unvaild command!");
        return;
    }
    mkdir(apath,2);
}
int main()//函数功能：根据输入来调用其他函数
{
    char command[121];
    char *token;
    char path[121];
    char another_path[121];
    int size = 0;
    char rebuild;
    char zero[512];
    memset(zero,0,512*sizeof(char));
    int fp = open_disk();
    if (fp == -1)
    {
        printf("disk does not exit!\n");
        return -1;
    }
    printf("*****************welcome to use,do you want to rebuild the system?************\n");
    scanf("%c",&rebuild);
    getchar();
    if(rebuild=='y'){
       for(int r=0;r<8192;r++){
           disk_write_block(r,zero);
       }
    char *temp;
    sp_block * spBlock = (sp_block *)malloc(sizeof(sp_block));
    memset(spBlock,0,sizeof(sp_block));
    sp_block1 * spBlock1 = (sp_block1 *)malloc(sizeof(sp_block1));
    memset(spBlock1,0,sizeof(sp_block1));
    spBlock->inode_map[0]=0x80000000;
    temp=spBlock;
    disk_write_block(0,temp);
    temp=spBlock1;
    disk_write_block(1,temp);
    }


    while (1)
    {
        // read input to command
        printf("\n");
        printf("==>");
        // 10 = 8char + '\n' + '\0'
        fgets(command, sizeof(command), stdin);
        // justify whether len(command)>512
        if (command[strlen(command) - 1] != '\n')
        {
            printf("too long command!\n");
            while (getchar() != '\n')
                ;
            continue;
        }
        else
        {
            command[strlen(command) - 1] = '\0';
        }

        // splite command
        token = strtok(command, " ");
        // ls
        if (strcmp("ls", token) == 0)
        {
            token = strtok(NULL, " ");
            if (token == NULL)
            {
                ls("/");
                continue;
            }

            strcpy(path, token);

            // have more than three argument
            if ((token = strtok(NULL, " ")) != NULL)
            {
                printf("Too much argument!\n");
                continue;
            }
            ls(path);
        }
        // create
        else if (strcmp("mkdir", token) == 0)
        {
            token = strtok(NULL, " ");
            if (token == NULL)
            {
                printf("unvalid command!\n");
                continue;
            }

            strcpy(path, token);

            // have more than three argument
            if ((token = strtok(NULL, " ")) != NULL)
            {
                printf("Too much argument!\n");
                continue;
            }
    
                mkdir(path,0);
         }
            // file
        else if (strcmp("touch", token) == 0)
            {
                token = strtok(NULL, " ");
                if (token == NULL)
                {
                    printf("unvalid command!\n");
                    continue;
                }

                strcpy(path, token);

                // have more than three argument
                if ((token = strtok(NULL, " ")) != NULL)
                {
                    printf("Too much argument!\n");
                    continue;
                }

                touch(path);
            }

        else if (strcmp("shutdown", token) == 0)
        {
            printf("\n");
            break;
        }
        else if (strcmp("cp", token) == 0)
        {
            if ((token = strtok(NULL, " ")) == NULL)
            {
                printf("unvalid command!\n");
                continue;
            }
            strcpy(path, token);

            if ((token = strtok(NULL, " ")) == NULL)
            {
                printf("unvalid command!\n");
                continue;
            }
            strcpy(another_path, token);

            if ((token = strtok(NULL, " ")) != NULL)
            {
                printf("unvalid command!\n");
                continue;
            }
            cp(path, another_path);
        }
        else
        {
            printf("unvalid command!\n");
        }
    }

    close_disk();
    return 0;
}
