#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
getname(char *path)
{
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;
  return p;
}

int checkname(char* path){
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;
  if(strcmp(p,".")==0 || strcmp(p,"..")==0)
    return -1;
  return 0;    
}

void findfile(char* path,char* filename){
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  if(checkname(path) < 0) return;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    if(strcmp(getname(path),filename)==0)
        printf("%s\n", path);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      findfile(buf,filename);
    }
    break;
  }
  close(fd);      
}

int
main(int argc, char *argv[])
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if(argc != 3){
    fprintf(2, "usage: find [path] [filename]\n");
    exit(1);
  }
  char *path=argv[1];

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    exit(1);
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    exit(1);
  }
  switch(st.type){
  case T_FILE:
    fprintf(2, "find: path should be DIR type, find FILE type instead.\n");
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("find: path too long\n");
      break;
    }
    //p和buf的关系可以理解为p改动了buf的内存，获取了目的目录下的文件名信息
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      findfile(buf,argv[2]);
    }
    break;
  }  

  exit(0);
}
