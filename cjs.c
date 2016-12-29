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

size_t ctype(uint8_t *out, cb0r_e type, uint32_t value)
{
  out[0] = type << 5;
  if(value <= 23)
  {
    out[0] |= value;
    return 1;
  }
  if(value >= 65536)
  {
    out[0] |= 26;
    out[1] = value & 0xff000000;
    out[2] = value & 0x00ff0000;
    out[3] = value & 0x0000ff00;
    out[4] = value & 0x000000ff;
    return 5;
  }
  if(value >= 256)
  {
    out[0] |= 25;
    out[1] = value & 0x0000ff00;
    out[2] = value & 0x000000ff;
    return 3;
  }
  out[0] |= 24;
  out[1] = value;
  return 2;
}

size_t js2c(uint8_t *in, size_t inlen, uint8_t *out)
{
  size_t outlen = 0;

  if(in[0] == '{' || in[0] == '[')
  {
    // count items, write cbor map/array+count, recurse each k/v
    uint16_t i=0;
    for(;js0n(NULL,i,(char*)in,inlen,&outlen);i++);
    if(in[0] == '{') outlen = ctype(out,CB0R_MAP,i/2);
    else outlen = ctype(out,CB0R_ARRAY,i);
    for(uint16_t j=0;j<i;j++)
    {
      size_t len = 0;
      const char *str = js0n(NULL,j,(char*)in,inlen,&len);
      outlen += js2c((uint8_t*)str,len,out+outlen);
    }

  }else if(in[inlen] == '"'){ // hack around js0n wart
    // write cbor UTF8
    outlen = ctype(out,CB0R_UTF8,inlen);
    memcpy(out+outlen,in,inlen);
    outlen += inlen;
  }else if(memchr(in,'.',inlen) || memchr(in,'e',inlen) || memchr(in,'E',inlen)){
    // write cbor JSON tag
  }else{
    // parse number, write cbor int
    long num = strtol((const char*)in,NULL,10);
    if(num < 0) outlen = ctype(out,CB0R_NEG,(uint32_t)(~num));
    else outlen = ctype(out,CB0R_INT,(uint32_t)num);
  }

  return outlen;
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
  uint8_t *bout = malloc(lin);

  if(strstr(file_in,".json"))
  {
    lout = js2c(bin,lin,bout);
    printf("serialized json[%ld] to cbor[%ld]\n",lin,lout);
  }else if(strstr(file_in,".cjs")){
    cb0r_s res = {0,};
    uint8_t *end = cb0r(bin,bin+lin,0,&res);
    printf("type %d len %ld\n",res.type,end-bin);
  }else{
    printf("file must be .json or .cjs: %s\n",file_in);
    return -1;
  }

  int ret = save(file_out,bout,lout);
  free(bin);
  free(bout);

  return ret;
}

