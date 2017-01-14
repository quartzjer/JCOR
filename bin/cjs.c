#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "cjs.h"

uint8_t *load(char *file, size_t *len)
{
  uint8_t *buf;
  struct stat fs;
  FILE *fd;
  
  fd = fopen(file,"rb");
  if(!fd)
  {
    printf("fopen error %s: %s\n",file,strerror(errno));
    return NULL;
  }
  if(fstat(fileno(fd),&fs) < 0)
  {
    fclose(fd);
    printf("fstat error %s: %s\n",file,strerror(errno));
    return NULL;
  }
  
  if(!(buf = malloc((size_t)fs.st_size)))
  {
    fclose(fd);
    printf("OOM: %lld\n",fs.st_size);
    return NULL;
  }
  *len = fread(buf,1,(size_t)fs.st_size,fd);
  fclose(fd);
  if(*len != (size_t)fs.st_size)
  {
    free(buf);
    printf("fread %lu != %lld for %s: %s\n",*len,fs.st_size,file,strerror(errno));
    return NULL;
  }

  return buf;
}

int save(char *file, uint8_t *bin, size_t len)
{
  if(!bin || !len) return -1;

  FILE *fd = fopen(file,"wb");
  if(!fd)
  {
    printf("Error writing to file %s: %s\n",file,strerror(errno));
    return -1;
  }

  int wrote = fwrite(bin,1,len,fd);
  fclose(fd);
  return (wrote == (int)len)?0:-1;
}

int main(int argc, char **argv)
{
  if(argc != 3)
  {
    printf("Usage (args may be swapped): cjs file.json file.cjs\n");
    return -1;
  }
  char *file_in = argv[1];
  char *file_out = argv[2];

  size_t lin = 0, lout = 0;
  uint8_t *bin = load(file_in,&lin);
  if(!bin || lin <= 0) return -1;

  // just bulk buffer working space
  uint8_t *bout = malloc(4*lin);

  if(strstr(file_in,".json"))
  {
    lout = js2cb(bin,lin,bout,false,NULL);
    printf("serialized json[%ld] to cbor[%ld]\n",lin,lout);
  }else if(strstr(file_in,".jwt")){
    lout = jwt2cb(bin,lin,bout);
    printf("serialized jwt[%ld] to cbor[%ld]\n",lin,lout);
  }else if(strstr(file_in,".cjs")){
    lout = cb2js(bin,lin,(char*)bout,0);
    printf("serialized cbor[%ld] to json[%ld]\n",lin,lout);
  }else{
    printf("file must be .json or .cjs: %s\n",file_in);
    return -1;
  }

  int ret = save(file_out,bout,lout);
  free(bin);
  free(bout);

  return ret;
}

