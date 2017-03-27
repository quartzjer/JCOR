#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "jscn.h"

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
  if(!bin || !len) return 1;

  FILE *fd = fopen(file,"wb");
  if(!fd)
  {
    printf("Error writing to file %s: %s\n",file,strerror(errno));
    return errno;
  }

  int wrote = fwrite(bin,1,len,fd);
  fclose(fd);
  return (wrote == (int)len)?0:2;
}

int main(int argc, char **argv)
{
  if(argc < 3)
  {
    printf("Usage (args may be swapped): jscn file.json file.jscn\n");
    return -1;
  }
  char *file_in = argv[1];
  char *file_out = argv[2];
  char *file_dict = (argc == 4)?argv[3]:NULL;


  size_t lin = 0, lout = 0;
  uint8_t *bin = load(file_in,&lin);
  if(!bin || lin <= 0) return -1;

  cb0r_t dict = NULL;
  cb0r_s dres = {0,}, resv = {0,};
  if(file_dict)
  {
    size_t dlen = 0;
    uint8_t *dbin = load(file_dict,&dlen);
    uint8_t index = 0;
    if(!dbin || dlen <= 0 || !(index = jscn2cbor(dbin, dlen, &dres, JSCN_KEY_DATA, &resv)) || resv.type != CB0R_ARRAY) {
      printf("dictionary file invalid: %s\n", file_dict);
      return -1;
    }
    // ugh, jscn2cbor needs refactor
    cb0r(dres.start, dres.end, index - 1, &dres); 
    dres.start = dres.end;
    dres.end = resv.end;
    dres.length = dres.end - dres.start;
    dict = &dres;
  }

  // just bulk buffer working space
  uint8_t *bout = malloc(4*lin);

  if(strstr(file_in,".json"))
  {
    lout = json2cn(bin,lin,bout,dict);
    printf("serialized json[%ld] to cbor[%ld]\n",lin,lout);
  }else if(strstr(file_in,".jwt")){
    lout = jwt2cn(bin,lin,bout,dict);
    printf("serialized jwt[%ld] to cbor[%ld]\n",lin,lout);
  }else if(strstr(file_in,".jscn")){
    lout = jscn2on(bin,lin,(char*)bout,dict);
    printf("serialized cbor[%ld] to json[%ld]\n",lin,lout);
  }else{
    printf("file must be .json or .jscn: %s\n",file_in);
    return 1;
  }

  int ret = save(file_out,bout,lout);
  free(bin);
  free(bout);

  return ret;
}

