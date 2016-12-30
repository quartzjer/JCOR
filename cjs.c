#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include "cb0r.h"
#include "js0n.h"

// copied from https://gist.github.com/yinyin/2027912
#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#endif	/* __APPLE__ */

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

size_t ctype(uint8_t *out, cb0r_e type, uint64_t value)
{
  out[0] = type << 5;
  if(value <= 23)
  {
    out[0] |= value;
    return 1;
  }
  if(value >= UINT32_MAX)
  {
    out[0] |= 27;
    value = htobe64(value);
    memcpy(out+1,&value,8);
    return 9;
  }
  if(value > UINT16_MAX)
  {
    out[0] |= 26;
    value = htobe32(value);
    memcpy(out+1,&value,4);
    return 5;
  }
  if(value >= UINT8_MAX)
  {
    out[0] |= 25;
    value = htobe16(value);
    memcpy(out+1,&value,2);
    return 3;
  }
  out[0] |= 24;
  out[1] = value;
  return 2;
}

size_t js2cb(uint8_t *in, size_t inlen, uint8_t *out)
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
      outlen += js2cb((uint8_t*)str,len,out+outlen);
    }

  }else if(in[inlen] == '"'){ // hack around js0n wart
    // write cbor UTF8
    outlen = ctype(out,CB0R_UTF8,inlen);
    memcpy(out+outlen,in,inlen);
    outlen += inlen;
  }else if(memcmp(in,"false",inlen) == 0){
    outlen = ctype(out,CB0R_SIMPLE,CB0R_FALSE);
  }else if(memcmp(in,"true",inlen) == 0){
    outlen = ctype(out,CB0R_SIMPLE,CB0R_TRUE);
  }else if(memcmp(in,"null",inlen) == 0){
    outlen = ctype(out,CB0R_SIMPLE,CB0R_NULL);
  }else if(memchr(in,'.',inlen) || memchr(in,'e',inlen) || memchr(in,'E',inlen)){
    // write cbor JSON tag to preserve fractions/exponents
  }else{
    // parse number, write cbor int
    long long num = strtoll((const char*)in,NULL,10);
    if(num < 0) outlen = ctype(out,CB0R_NEG,(uint64_t)(~num));
    else outlen = ctype(out,CB0R_INT,(uint64_t)num);
  }

  return outlen;
}

size_t cb2js(uint8_t *in, size_t inlen, char *out, uint32_t skip)
{
  size_t outlen = 0;
  cb0r_s res = {0,};
  uint8_t *end = cb0r(in,in+inlen,skip,&res);
  switch(res.type)
  {
    case CB0R_INT: {
      // TODO 64bit nums
      outlen = sprintf(out,"%u",res.value);
    } break;
    case CB0R_NEG: {
      outlen = sprintf(out,"-%u",res.value+1);
    } break;
    case CB0R_BYTE: {
      // TODO for dictionary
    } break;
    case CB0R_UTF8: {
      outlen = sprintf(out,"\"%.*s\"",res.length,res.start);
    } break;
    case CB0R_ARRAY: {
      outlen += sprintf(out+outlen,"[");
      for(uint32_t i=0;i<res.count;i++)
      {
        if(i) outlen += sprintf(out+outlen,",");
        outlen += cb2js(res.start,end-res.start,out+outlen,i);
      }
      outlen += sprintf(out+outlen,"]");
    } break;
    case CB0R_MAP: {
      outlen += sprintf(out+outlen,"{");
      for(uint32_t i=0;i<res.count;i++)
      {
        if(i) outlen += sprintf(out+outlen,",");
        outlen += cb2js(res.start,end-res.start,out+outlen,i*2);
        outlen += sprintf(out+outlen,":");
        outlen += cb2js(res.start,end-res.start,out+outlen,(i*2)+1);
      }
      outlen += sprintf(out+outlen,"}");
    } break;
    case CB0R_TAG: {
      // TODO for raw/b64u
    } break;
    case CB0R_FALSE: {
      outlen = sprintf(out,"false");
    } break;
    case CB0R_TRUE: {
      outlen = sprintf(out,"true");
    } break;
    case CB0R_NULL: {
      outlen = sprintf(out,"null");
    } break;
    default: {
      printf("unsupported type: %u\n",res.type);
    } break;

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
  uint8_t *bout = NULL;

  if(strstr(file_in,".json"))
  {
    bout = malloc(lin);
    lout = js2cb(bin,lin,bout);
    printf("serialized json[%ld] to cbor[%ld]\n",lin,lout);
  }else if(strstr(file_in,".cjs")){
    bout = malloc(4*lin); // just bulk buffer space for now
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

