/*
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2.1 as
 published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation Inc., 59 Temple Place, Suite 330, Boston MA 02111-1307 USA
*/

/*
  $Id$

  Revision history:
  1.0 - 2007-01-25 - Brossillon J.Damien <brossill@enseirb.fr>
  -- Initial version

  1.1 - 2007-09-15
  -- Stuff added to guess the header versions.

  1.2 - 2010-08-15
  -- Added Nano 4G firmware support.

  Todo list:
  -- Debug
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>

#include "extract2g.h"

#define VERSION_H 1
#define VERSION_L 2

#define B (0x0FFFFFFF)

/* Change endian of a 4 bytes memory zone */
void change_endian( char* buf )
{
  
  char tmp[4];

  tmp[3] = buf[0];
  tmp[2] = buf[1];
  tmp[1] = buf[2];
  tmp[0] = buf[3];
  
  memcpy(buf,tmp,4*sizeof(char));

  return;
}

unsigned int hash_part(FILE* hIn, int start, int len, int header_len)
{
  unsigned int hash = 0;
  
  fseek(hIn, start, SEEK_SET);

  unsigned int tmp;

  int pos;
  for(pos=0;pos<(len+header_len);pos++)
  {
    tmp = fgetc(hIn);

    hash = (hash*B + tmp*pos);
  }

  return hash;
}

void extract_part(FILE* hIn, FILE* hOut,
                    char* name,
                    int start, int len,
                    int header_len)
{
  int pos;
  char tmp;

  char* filename = NULL;

  // No extract name specified
  if( !hOut )
  {
    filename = (char*)malloc( strlen(name) + 4 );

    if( !filename )
    {
      fprintf(stderr, "Memory allocation failure.");
      exit(EXIT_FAILURE);
    }

    strcpy(filename, name);
    strcat(filename, ".fw");

    hOut = fopen(filename, "wb");
  }
 
  fprintf(stdout,"Extracting from 0x%8.X to 0x%8.X in %s.\n",
           start, start+len+header_len, filename);
  
  // Go to the beginning of the part
  fseek(hIn, start, SEEK_SET);

  // Extracting
  for(pos=0;pos<(len+header_len);pos++)
  {
    tmp = fgetc(hIn);
    fputc(tmp,hOut);
  }

  fclose(hOut);

  free(filename);

  return;
}

char is_valid( DIRECTORY* p_dir )
{
  // Test if the directory contain a valid header for the part.
  return ( !strncmp( p_dir->dev, "NAND", 4) 
            || !strncmp( p_dir->dev, "ATA!", 4) );
}

char check_header(FILE* fh, unsigned int l)
{
  DIRECTORY dir;

  fseek(fh, l, SEEK_SET);
  fread( (void*)&dir, LEN_DIR, sizeof(char), fh);

  change_endian(dir.dev);
  
  return is_valid(&dir);
}

int guess_directory_address(FILE* fh)
{
  // Check for 1G header
  if(check_header(fh,ADDR_DIR_1G))
  {
    printf("> iPod nano 1g header detected.\n");
    return ADDR_DIR_1G;
  }

  // Check for 2G header
  if(check_header(fh,ADDR_DIR_2G))
  {
    printf("> iPod nano 2g header detected.\n");
    return ADDR_DIR_2G;
  }

  // Check for 3G header
  if(check_header(fh,ADDR_DIR_3G))
  {
    printf("> iPod nano 3g header detected.\n");
    return ADDR_DIR_3G;
  }

  // At least try to find an header
  fseek(fh, 0, SEEK_SET);

  DIRECTORY dir;

  while(!feof(fh))
  {
    fread( (void*)&dir, LEN_DIR, sizeof(char), fh);
    change_endian(dir.dev);

    if(is_valid(&dir))
    {
      int addr = ftell(fh) - LEN_DIR;
      printf("> Unknown header found at 0x%X\n",addr);
      return addr;
    }

    fseek(fh, -LEN_DIR + 1, SEEK_CUR);
  }
  
  return ADDR_DIR_DEFAULT;
}

void print_directory_infos(DIRECTORY* p_dir, char extended)
{
  if( extended )
    printf("dev: %.4s type: %.4s\n\
id: %X\n\
devOffset: %X\n\
len: %X\n\
addr: %X\n\
entryOffset: %X\n\
checksum: %X\n\
version: %X\n\
loadAddr: %X\n\n",
            p_dir->dev,
            p_dir->type,
            p_dir->id,
            p_dir->devOffset,
            p_dir->len,
            p_dir->addr,
            p_dir->entryOffset,
            p_dir->checksum,
            p_dir->version,
            p_dir->loadAddr );

   else
      printf("dev: %.4s type: %.4s devOffset: %X len: %X\n",
              p_dir->dev,
              p_dir->type,
              p_dir->devOffset,
              p_dir->len);

}


/* Display help */
void usage(int status)
{
  if (status != EXIT_SUCCESS)
    fputs("Try `extract_2g --help' for more information.\n", stderr);
  else
  {
    fputs("Usage: extract_2g [OPTION] [FILE]\n", stdout);
    fprintf(stdout,"Read & extract iPod Nano 2G data parts from a FILE dump.\n\
\n\
-l, --list               only list avaible parts according to the dump directory\n\n\
-H, --hash               do a hash on every part of the dump\n\n\
-A, --all                extract ALL founded parts from dump (default names used)\n\n\
-4, --nano4g-compat      forces iPod Nano 3G firmwares to be recognised as iPod Nano 4G's\n\n\
-e, --extract=NAME       select which part you want to extract (default none)\n\
-o, --output=FILE        put the extracted part into FILE (default NAME.fw)\n\n\
-d, --directory-address  specify the directory address (default 0x%X )\n\
-a, --header-length      specify the header length of each part (default 0x800)\n\n\
-h, --help               display this help and exit\n", ADDR_DIR_DEFAULT);
  }

  exit(status);
}

int main(int argc, char **argv)
{
  FILE *file = NULL;
  FILE *hExtract = NULL;

  char str_target[32];

  char extract_is_set = 0;
  char extract_all = 0;
  char nano4g_compat = 0;
  char list_directories = 0;
  char is_found = 0;
  char hash_parts = 0;

  /* REMOVE */
  char c;

  int tmp;

  int i;

  int directories_size = 0;
  DIRECTORY* directories = NULL;

  int addr_directory = -1;

  int length_header = LEN_HEADER;

  /* Getopt short options */
  static char const short_options[] = "e:o:d:a:AHl4hv";
  
  /* Getopt long options */
  static struct option const long_options[] = {
    {"extract", required_argument, NULL, 'e'},
    {"output", required_argument, NULL, 'o'},
    {"hash", no_argument, NULL, 'H'},
    {"list", no_argument, NULL, 'l'},
    {"all", no_argument, NULL, 'A'},
    {"directory-address", required_argument, NULL, 'd'},
    {"header-length", required_argument, NULL, 'a'},
    {"nano4g-compat", no_argument, NULL, '4'},
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
    {NULL, 0, NULL, 0}
  };

  /* Some informations */
  printf("%s compiled at %s %s.\n\n",argv[0],__TIME__,__DATE__);

  /* Parse command line options.  */
  while ((c = getopt_long(argc, argv,
                          short_options, long_options, NULL)) != -1) {
    switch (c) {
    case 'o':
        hExtract = fopen(optarg,"wb");

        if(!hExtract)
        {
          fprintf(stderr,"Cannot write/create %s.\n", optarg);
          usage(EXIT_FAILURE);
        }
        
      break;

    case 'a':
      if ( (tmp = strtol(optarg, NULL, 0)) > 0 )
        length_header = tmp;
      else {
        fprintf(stderr, "Invalid `header-length' value `%s'.\n", optarg);
        usage(EXIT_FAILURE);
      }
      break;

    case 'd':
      if ( (tmp = strtol(optarg, NULL, 0)) > 0 )
        addr_directory = tmp;
      else {
        fprintf(stderr, "Invalid `directory-address' value `%s'.\n", optarg);
        usage(EXIT_FAILURE);
      }
      break;

    case 'e':
        strncpy(str_target,optarg,32);
        extract_is_set = 1;
      break;

    case 'H':
        hash_parts = 1;
      break;

    case 'l':
        list_directories = 1;
      break;

    case 'A':
        extract_all = 1;
      break;

    case '4':
        nano4g_compat = 1;
      break;

    case 'h':
      usage(EXIT_SUCCESS);
      break;
      
    case 'v':
      fprintf(stdout,"extract_2g version %d.%d\n",VERSION_H,VERSION_L);
      fputs("Copyright (C) 2007 Brossillon J.Damien\n\
This is free software. You may redistribute copies of it under the terms\n\
of the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n\
There is NO WARRANTY, to the extent permitted by law.\n", stdout);
      exit(EXIT_SUCCESS);
      break;

    default:
      usage(EXIT_FAILURE);
    }
  }

  if (!argv[optind]) {
    fprintf(stderr,"Missing dump filename.\n");
    usage(EXIT_FAILURE);
  } else {
    if (!(file = fopen(argv[optind], "rb")))
    {
      fprintf(stderr,"Cannot open file `%s'.\n", argv[optind]);
      usage(EXIT_FAILURE);
    }
  }
 

  if( addr_directory < 0 )
    addr_directory = guess_directory_address(file);
  else
    addr_directory = ADDR_DIR_DEFAULT;

  if (addr_directory == ADDR_DIR_3G && nano4g_compat)
    printf("> iPod nano 4g compat. mode enabled\n");


  fseek(file, addr_directory, SEEK_SET);
   
  DIRECTORY dir;

  // List directories
  while(1)
  {
    /* FIXME
    for(i=0;i<(int)LEN_DIR;i++)
    {
      ((char*)&dir)[i] = fgetc(file);
    }*/
    fread( (void*)&dir, LEN_DIR, sizeof(char), file);
    
    change_endian(dir.dev);
    change_endian(dir.type);
    
    if( is_valid(&dir) )
    {
      directories_size++;
      directories = (DIRECTORY*)realloc(directories,
                                        directories_size*sizeof(DIRECTORY));

      if(!directories)
      {
        fprintf(stderr,"Memory allocation failure.\n");
        exit(EXIT_FAILURE);
      }
	  
	  if (addr_directory == ADDR_DIR_3G && nano4g_compat)
        dir.devOffset += 4096;
	  
      memcpy(directories + directories_size - 1, &dir, sizeof(DIRECTORY));
    }
    else
      break;
  }

  // Check if we have, at least, one valid part
  if( directories_size <= 0 )
  {
    fclose(file);

    if(directories)
      free(directories);

    fprintf(stderr, "Cannot find at least one valid part in the dump.\n");
    exit(EXIT_FAILURE);
  }

  // User want to hash every parts of the dump
  if( hash_parts )
  {

    for(i=0;i<directories_size;i++)
    {
      fprintf(stdout,"%.4s: 0x%X\n",directories[i].type,
                hash_part(file,
                           directories[i].devOffset, directories[i].len,
                           length_header) );
    }
  }
  else
  if( extract_all )
  {
    // User want to extract every part of the dump
    for(i=0;i<directories_size;i++)
    {
      char name[5];

      memcpy(name,directories[i].type,4*sizeof(char));
      name[4] = '\0';
      
      extract_part(file, NULL,
                    name,
                    directories[i].devOffset, directories[i].len,
                    length_header);
    }

    fprintf(stdout,"Done.\n");
    
          
  }
  else
  {
    if( list_directories )
    {
      // User ask a big directory listing 

      for(i=0;i<directories_size; i++)
        print_directory_infos( &(directories[i]), 1 );
    }
    else
    { 
      
      if( extract_is_set )
      {
        is_found = 0;

        for(i=0;i<directories_size;i++)
          if( !strcmp( directories[i].type, str_target ) )
          { // Extract & correct part name

            is_found = 1;
            
            // Lock & load, got name, file, position and length !         
            extract_part(file, hExtract,
                          directories[i].type,
                          directories[i].devOffset, directories[i].len,
                          length_header);
            
          }

        // Wrong part name, error message & small parts list
        if(!is_found)
        {
          fprintf(stderr,"No part named '%s' found.\n\n",str_target);

          for(i=0;i<directories_size;i++)
            print_directory_infos( &(directories[i]), 0);  
        }

      }
      else
      { // No actions, small parts list

        for(i=0;i<directories_size;i++)
          print_directory_infos( &(directories[i]), 0);   
      }

    }
  }

  fclose(file);
 
  free(directories);

  return EXIT_SUCCESS;
}
