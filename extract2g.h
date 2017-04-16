
#define ADDR_DIR_DEFAULT 0x4800
#define ADDR_DIR_1G      0x4200
#define ADDR_DIR_2G      0x4800
#define ADDR_DIR_3G      0x5000


#define LEN_HEADER 0x800

#define LEN_DIR sizeof(DIRECTORY)

typedef struct _DIRECTORY
{
  char dev[4];
  char type[4];

  unsigned int id;
  unsigned int devOffset;
  unsigned int len; 
  unsigned int addr;

  unsigned int entryOffset;
  unsigned int checksum; 
  unsigned int version;
  unsigned int loadAddr;

}DIRECTORY;
