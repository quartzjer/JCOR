#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
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

size_t jwt2json(uint8_t *in, size_t inlen, uint8_t *out, size_t outlen)
{
  char *encoded = (char *)in;
  char *end = encoded + (inlen - 1);

  // make sure the dot separators are there
  char *dot1 = memchr(encoded, '.', end - encoded);
  if(!dot1 || dot1 + 1 >= end) return 0;
  dot1++;
  char *dot2 = memchr(dot1, '.', end - dot1);
  if(!dot2 || (dot2 + 1) >= end) return 0;
  dot2++;
  char *dot3 = memchr(dot2, '.', end - dot2);
  if(!dot3 || (dot3 + 1) >= end) {
    // it's a JWS
    return snprintf((char *)out, outlen, "{\"protected\":\"%.*s\",\"payload\":\"%.*s\",\"signature\":\"%.*s\"}", (int)((dot1 - encoded) - 1), encoded, (int)((dot2 - dot1) - 1), dot1, (int)((end - dot2) + 1), dot2);
  }
  dot3++;
  char *dot4 = memchr(dot3, '.', end - dot3);
  if(!dot4 || (dot4 + 1) >= end) return 0;
  dot4++;

  // it's a JWE
  return snprintf((char *)out, outlen, "{\"protected\":\"%.*s\",\"encrypted_key\":\"%.*s\",\"iv\":\"%.*s\",\"ciphertext\":\"%.*s\",\"tag\":\"%.*s\"}", (int)((dot1 - encoded) - 1), encoded, (int)((dot2 - dot1) - 1), dot1, (int)((dot3 - dot2) - 1), dot2, (int)((dot4 - dot3) - 1), dot3, (int)((end - dot4) + 1), dot4);
}

size_t json2jwt(uint8_t *in, size_t inlen, uint8_t *out, size_t outlen)
{
  return 0;
}

int main(int argc, char **argv)
{
  if(argc < 3)
  {
    printf("Usage (args may be swapped): jwt2json file.jwt file.json\n");
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
    if(!(lout = json2jwt(bin, lin, bout, lout))) {
      printf("JSON parsing failed: %s\n",file_in);
      return 3;
    }
    printf("serialized json[%ld] to jwt[%ld]: %s\n", lin, lout, bout);
  }else if(strstr(file_in,".jwt")){
    if(!(lout = jwt2json(bin, lin, bout, 4*lin))) {
      printf("JWT parsing failed: %s\n", file_in);
      return 4;
    }
    printf("serialized jwt[%ld] to json[%ld]: %s\n", lin, lout, bout);
  } else {
    printf("file must be .json or .jwt: %s\n",file_in);
    return 1;
  }

  int ret = save(file_out,bout,lout);
  free(bin);
  free(bout);

  return ret;
}

