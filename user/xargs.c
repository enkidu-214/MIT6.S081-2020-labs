#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"


int
main(int argc, char *argv[])
{
  if(argc < 2){
    fprintf(2, "usage: xargs exec [args] \n");
    exit(1);
  }
  char buf[512];
  int size = read(0,buf,sizeof(buf));
  int line=0;
  for(int i=0;i<size;i++){
    if(buf[i]=='\n')
      line++;
  }
  char linearg[line][128];
  for(int i=0,j=0,k=0;i<size;i++,k++){  
    if(buf[i]!='\n')
        linearg[j][k]=buf[i];
    else{
        linearg[j][k]='\0';
        j++;
        k=-1;
    }
  }
  for(int i=0;i<line;i++){
    char *inputarg[MAXARG];
    for(int j=0;j<argc-1;j++){
        inputarg[j]=argv[j+1];
    }
    inputarg[argc-1] = linearg[i];
    if(fork()==0){
      exec(argv[1],inputarg);
      exit(0);
    }
    else{
      wait(0);
    }
  }
  exit(0);
}