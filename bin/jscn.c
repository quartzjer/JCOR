#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "../src/jscn.h"

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

  jscn_s dbuf = {0,};
  jscn_t dict = NULL;
  if(file_dict) {
  // if(result->jscn.count > 1 && cb0r_find(&(result->jscn), CB0R_INT, JSCN_KEY_DICT, NULL, &res)) {

    size_t dlen = 0;
    uint8_t *dbin = load(file_dict,&dlen);
    if(!dbin || dlen <= 0 || !jscn_load(dbin, dlen, &dbuf) || dbuf.data.type != CB0R_ARRAY) {
      printf("dictionary file invalid: %s %u\n", file_dict, dbuf.data.type);
      return -1;
    }
    dict = &dbuf;
  }

  // just bulk buffer working space
  uint8_t *bout = malloc(4*lin);
  jscn_s jscn = {0,};
  jscn.dict = dict;

  if(strstr(file_in,".json"))
  {
    if(!jscn_parse((char *)bin, lin, bout, &jscn)) {
      printf("JSON parsing failed: %s\n",file_in);
      return 3;
    }
    lout = jscn.jscn.end - bout;
    printf("serialized json[%ld] to cbor[%ld]\n", lin, lout);
  }else if(strstr(file_in,".jwt")){
//    lout = jwt2cn(bin,lin,bout,dict);
    printf("serialized jwt[%ld] to cbor[%ld]\n",lin,lout);
  }else if(strstr(file_in,".jscn")){
    if(!jscn_load(bin,lin,&jscn)) {
      printf("jscn file failed to load: %s\n",file_in);
      return 4;
    }
    // TODO dict lookup
    jscn.dict = dict;
    lout = jscn_stringify(&jscn,(char*)bout);
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

