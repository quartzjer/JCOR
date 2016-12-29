#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include "cb0r.h"
#include "js0n.h"

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
  return (wrote == len)?0:-1;
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

  size_t len = 0;
  uint8_t *bin = load(file_in,&len);
  if(!bin) return -1;
  uint8_t *bout = NULL;

  if(strstr(file_in,".json"))
  {
    const char *ret = js0n(0,1,(char*)bin,len,&len);
    printf("first len %lu: %.*s\n",len,(int)len,ret);
  }else if(strstr(file_in,".cjs")){
    cb0r_s res = {0,};
    uint8_t *end = cb0r(bin,bin+len,0,&res);
    printf("type %d len %ld\n",res.type,end-bin);
  }else{
    printf("file must be .json or .cjs: %s\n",file_in);
    return -1;
  }

  int ret = save(file_out,bout,len);
  free(bin);
  free(bout);

  return ret;
}

