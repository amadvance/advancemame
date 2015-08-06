/*
  based on unix romident
*/

#include <stdio.h>
#include <malloc.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include "stdarg.h"

#include "unzip.h"
#include "zlib.h"

#include "roms.h"
#include "comp.h"

#ifndef CLIB_DECL
#define CLIB_DECL
#endif

#define ARRAY_LENGTH(a) (sizeof(a)/sizeof(a[0]))

enum { GOOD_SIZED_ROMS, ALL_ROMS };

void CLIB_DECL logerror(const char *text,...)
{
    va_list arg;
    va_start(arg,text);
    if (0)
        vfprintf(stderr,text,arg);
    va_end(arg);
}

void get_dirname(char *s, char *d)
{
   char* slash = strrchr(s,'/');
   if (slash) {
	   int len = slash-s;
	   strncpy(d,s,len);
	   d += len;
   } else {
	   *(d++)='.';
   }
   *d = 0;
}

void get_filename(char *s, char *d)
{
   char* slash = strrchr(s,'/');
   if (slash)
      ++slash;
   else
      slash = s;
   strcpy(d,slash);
}

int hex2int(char *crcstr)
{
   int s = 0, i, v = 0, m = 1;
   char d;
   do {
      s++;
   } while ((crcstr[s]!=0)&&(crcstr[s]!=' '));
   s--;
   for (i=s;i>=0;i--) {
      if (crcstr[i]>='a') crcstr[i]-=32;
      d = toupper(crcstr[i]);
      if ((d>='0') && (d<='9')) v+=m*(int)(d-'0');
      if ((d>='A') && (d<='F')) v+=m*(10+(int)(d-'A'));
      m = m * 16;
   }
   return v;
}

void print_computer(int crc_32)
{
	int i, j;

	for (i=0; i<ARRAY_LENGTH(computer); i++ ) {
		for (j=0; (j<ARRAY_LENGTH(computer[i].crc32))&&computer[i].crc32[j]; j++) {
			if (crc_32==computer[i].crc32[j]) {
				printf(" %s", computer[i].text);
				break;
			}
		}
	}
}


unsigned char *wbuf;
char unk = 0;

int romident(char *rom, unsigned int crc_32, int size, int mode)
{
   int i, f = 0;

   if (size%32 && mode!=ALL_ROMS) return -1;

   printf("%-12s [%08x] ", rom, crc_32);
   for (i=0;i!=ARRAY_LENGTH(roms);i++) {
      if (roms[i].crc32 == crc_32) {
         if (f) printf("                        ");
         f++;
         printf("= %-12s", roms[i].text);
		 print_computer(crc_32);
		 printf("\n");
//         printf("= %-12s from %s\n", rtab[i].rom, gtab[rtab[i].game].title);
      }
   }
   if (!f) {
      printf("NOT FOUND!\n");
      unk = 1;
   }
   return f;
}

int ident_crc(unsigned int crc_32)
{
   int i, f = 0;

   printf("Checking crc 0x%08x ... ", crc_32);
   for (i=0;i!=ARRAY_LENGTH(roms);i++) {
      if (roms[i].crc32 == crc_32) {
         if (f) printf("                  ");
         f++;
         printf("%-12s", roms[i].text);
		 print_computer(crc_32);
		 printf("\n");
//         printf("%-12s from %s\n", roms[i].name, gtab[rtab[i].game].title);
      }
   }
   if (!f) {
      printf("NOT FOUND IN DATABASE!\n");
      unk = 1;
   }
   return f;
}


int ident_file(char *path, char *fn, int size, int mode)
{
   unsigned int crc;
   FILE *f;
   char fpath[512];
   sprintf(fpath, "%s/%s", path, fn);
   if (size == -1) {
      struct stat st;
      if (stat(fpath,&st)!=0) {
         printf("Error, '%s' size incorrect !\n", fpath);
         return 1;
      }
//      printf("fpath = %s\n", fpath);
      size = st.st_size;
   }
   wbuf = (unsigned char *)malloc(size);
   if (!wbuf) {
      printf("Error, not enough memory to '%s' the file into memory !\n", fn);
      return 1;
   }
   f = fopen(fpath, "rb");
   if (!f) {
      printf("Error, cannot open file '%s' !\n", fpath);
      return 1;
   }
   fread(wbuf, 1, size, f);
   crc = crc32(0L, wbuf, size);
   fclose(f);
   free(wbuf);
   romident(fn, crc, size, mode);
   return 0;
}

int ident_zip(char *fn, int mode)
{
   ZIP* zip;
   zip_entry* zipf;
   printf("Zip file to ident = '%s'\n", fn);
   if ((zip = openzip(0, 0, fn)) == 0) {
      printf("Error, cannot open zip file '%s' !\n", fn);
      return 1;
   }
   while ((zipf = readzip(zip))) {
      romident(zipf->name, zipf->crc32, zipf->uncompressed_size, mode);
   }
   closezip(zip);
   return 0;
}

int ident_dir(char *fn, int mode)
{
   struct dirent* d;
   DIR* dd = opendir(fn);
   if (!dd) return 1;
   d = readdir(dd);
   while (d) {
      struct stat st;
      char path[512];
      sprintf(path,"%s/%s",fn,d->d_name);
      if (stat(path,&st)!=0) return 1;
      if (!S_ISDIR(st.st_mode)) {
         if (ident_file(fn, d->d_name, st.st_size, mode)) return 1;
      }
      d = readdir(dd);
   }
   return 0;
}

void ident(char *fn, int mode)
{
	int l;
	struct stat st;
	if (stat(fn,&st)!=0) {
		fprintf(stderr,"error in stat file %s\n", fn);
		return;
	}
	if (!S_ISDIR(st.st_mode)) {
		l = strlen(fn);
		if (l>4 && (strcmp(&fn[l-4], ".zip")==0 || strcmp(&fn[l-4], ".ZIP")==0)) {
			ident_zip(fn, mode);
		} else {
            char dir[512];
            char file[512];
            get_dirname(fn,dir);
			get_filename(fn,file);
            ident_file(dir, file, -1, mode);
		}
	} else {
		ident_dir(fn, mode);
	}
}

#if defined(_MSC_VER)
int DECL_SPEC main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
	int nf;
	int mode = GOOD_SIZED_ROMS;

	printf("MESSROMS alpha0.1\n");

	if (argc<2) {
		printf("Error, specify atleast one file name !\n");
		return 1;
	}

	for (nf=1;nf<argc;nf++)
	{
		if (argv[nf][0] == '-')
		{
			switch(argv[nf][1])
			{
            			case '-':
					ident_crc(hex2int(&argv[nf][2]));
					break;
				case 'a':
					mode = ALL_ROMS;
					break;
			}
		} 
		else
		{
			struct stat st;
			if (stat(argv[nf],&st)!=0)
			{
				printf("Error, '%s' doesn't exist !\n", argv[1]);
				return 1;
			}
			ident(argv[nf], mode);
		}
	}

	return 0;
}
