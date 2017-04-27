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
  char *file_refs = (argc == 4)?argv[3]:NULL;
  bool whitespace = (argc == 4)?false:true;


  size_t lin = 0;
  uint8_t *bin = load(file_in,&lin);
  if(!bin || lin <= 0) return -1;

  cb0r_t refs = NULL;
  if(file_refs) {
    size_t dlen = 0;
    uint8_t *dbin = load(file_refs,&dlen);
    jscn_t dref = NULL;
    if(!dbin || dlen <= 0 || !(dref = jscn_load(dbin, dlen)) || dref->data.type != CB0R_ARRAY) {
      printf("refs file invalid: %s %u\n", file_refs, dref->data.type);
      return -1;
    } else {
      refs = &(dref->data);
    }
  }

  jscn_t jscn = NULL;
  int ret = 0;
  size_t lout = 0;
  uint8_t *bout = NULL;

  if(strstr(file_in,".json"))
  {
    if(!(jscn = jscn_json2((char *)bin, lin, refs, whitespace))) {
      ret = printf("JSON parsing failed: %s\n",file_in);
    } else {
      bout = jscn->start;
      lout = jscn->length;
      printf("serialized json[%ld] to cbor[%ld]\n", lin, lout);
    }
  }else if(strstr(file_in,".jscn")){
    if(!(jscn = jscn_load(bin,lin))) {
      printf("jscn file failed to load: %s\n",file_in);
      return 4;
    }
    // TODO refs lookup on demand
    if(!(bout = (uint8_t *)jscn_2json(jscn, refs, true))) {
      ret = printf("JSON generation failed: %s\n", file_in);
    } else {
      lout = strlen((char*)bout);
      printf("serialized cbor[%ld] to json[%ld]\n", lin, lout);
    }
  }else{
    ret = printf("file must be .json or .jscn: %s\n",file_in);
  }

  free(bin);

  if(!ret && bout && lout) ret = save(file_out, bout, lout);

  return ret;
}

