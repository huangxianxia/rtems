/*
 * bin2c.c
 *
 * convert a binary file into a C source array.
 *
 * THE "BEER-WARE LICENSE" (Revision 3.1415):
 * sandro AT sigala DOT it wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff.  If we meet some
 * day, and you think this stuff is worth it, you can buy me a beer in
 * return.  Sandro Sigala
 *
 * Subsequently modified by Joel Sherrill <joel.sherrill@oarcorp.com>
 * to add a number of capabilities not in the original.
 *
 * syntax:  bin2c [-c] [-z] <input_file> <output_file>
 *
 *    -c    do NOT add the "const" keyword to definition
 *    -s    add the "static" keywork to definition
 *    -v    verbose
 *    -z    terminate the array with a zero (useful for embedded C strings)
 *
 * examples:
 *     bin2c -c myimage.png myimage_png.cpp
 *     bin2c -z sometext.txt sometext_txt.cpp
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

int useconst = 1;
int usestatic = 0;
int verbose = 0;
int zeroterminated = 0;
int createC = 1;
int createH = 1;

int myfgetc(FILE *f)
{
  int c = fgetc(f);
  if (c == EOF && zeroterminated) {
    zeroterminated = 0;
    return 0;
  }
  return c;
}

void process(const char *ifname, const char *ofname)
{
  FILE *ifile, *ocfile, *ohfile;
  char buf[PATH_MAX], *p;
  char obasename[PATH_MAX];
  char ocname[PATH_MAX];
  char ohname[PATH_MAX];
  const char *cp;
  size_t len;

  /* Error check */
  if ( !ifname || !ofname ) {
    fprintf(stderr, "process has NULL filename\n");
    exit(1);
  }

  strncpy( obasename, ofname, PATH_MAX );
  len = strlen( obasename );
  if ( len >= 2 ) {
    if ( obasename[len-2] == '.' ) {
      if ( (obasename[len-1] == 'c') || (obasename[len-1] == 'h') )
        obasename[len-2] = '\0';
    }
  }

  sprintf( ocname, "%s.c", obasename );
  sprintf( ohname, "%s.h", obasename );

  if ( verbose ) {
    fprintf(
      stderr,
      "in file: %s\n"
      "c file: %s\n"
      "h file: %s\n",
      ifname,
      ocname,
      ohname
    );
  }

  /* Open input and output files */
  ifile = fopen(ifname, "rb");
  if (ifile == NULL) {
    fprintf(stderr, "cannot open %s for reading\n", ifname);
    exit(1);
  }
  
  if ( createC ) {
  ocfile = fopen(ocname, "wb");
  if (ocfile == NULL) {
    fprintf(stderr, "cannot open %s for writing\n", ocname);
    exit(1);
  }
  }
  
  if ( createH ) {
  ohfile = fopen(ohname, "wb");
  if (ohfile == NULL) {
    fprintf(stderr, "cannot open %s for writing\n", ohname);
    exit(1);
  }
  }
  
  /* find basename */
  char *ifbasename = strdup(ifname);
  ifbasename = basename(ifbasename);
  
  strcpy(buf, ifbasename);
  for (p = buf; *p != '\0'; ++p)
    if (!isalnum(*p))
      *p = '_';

  if ( createC ) {
  /* print C file header */
  fprintf(
    ocfile,
    "/*\n"
    " *  Declarations for C structure representing binary file %s\n"
    " *\n"
    " *  WARNING: Automatically generated -- do not edit!\n"
    " */\n"
    "\n"
    "#include <sys/types.h>\n"
    "\n",
    ifbasename
  );

  /* print structure */
  fprintf(
    ocfile,
    "%s%sunsigned char %s[] = {\n  ",
    ((usestatic) ? "static " : ""),
    ((useconst) ? "const " : ""),
    buf
  );
  int c, col = 1;
  while ((c = myfgetc(ifile)) != EOF) {
    if (col >= 78 - 6) {
      fprintf(ocfile, "\n  ");
      col = 1;
    }
    fprintf(ocfile, "0x%.2x, ", c);
    col += 6;

  }
  fprintf(ocfile, "\n};\n");

  /* print sizeof */
  fprintf(
    ocfile,
    "\n"
    "%s%ssize_t %s_size = sizeof(%s);\n",
    ((usestatic) ? "static " : ""),
    ((useconst) ? "const " : ""),
    buf,
    buf
  );
  } /* createC */
  
  /*****************************************************************/
  /******                    END OF C FILE                     *****/
  /*****************************************************************/

  if ( createH ) {
  /* print H file header */
  fprintf(
    ohfile,
    "/*\n"
    " *  Extern declarations for C structure representing binary file %s\n"
    " *\n"
    " *  WARNING: Automatically generated -- do not edit!\n"
    " */\n"
    "\n"
    "#ifndef __%s_h\n"
    "#define __%s_h\n"
    "\n"
    "#include <sys/types.h>\n"
    "\n",
    ifbasename,  /* header */
    obasename,  /* ifndef */
    obasename   /* define */
  );

  /* print structure */
  fprintf(
    ohfile,
    "extern %s%sunsigned char %s[];",
    ((usestatic) ? "static " : ""),
    ((useconst) ? "const " : ""),
    buf
  );
  /* print sizeof */
  fprintf(
    ohfile,
    "\n"
    "extern %s%ssize_t %s_size;\n",
    ((usestatic) ? "static " : ""),
    ((useconst) ? "const " : ""),
    buf
  );

  fprintf(
    ohfile,
    "\n"
    "#endif\n"
  );
  } /* createH */
  
  /*****************************************************************/
  /******                    END OF H FILE                     *****/
  /*****************************************************************/

  fclose(ifile);
  if ( createC ) { fclose(ocfile); }
  if ( createH ) { fclose(ohfile); }
}

void usage(void)
{
  fprintf(
     stderr,
     "usage: bin2c [-csvzCH] <input_file> <output_file>\n"
     "  <input_file> is the binary file to convert\n"
     "  <output_file> should not have a .c or .h extension\n"
     "\n"
     "  -c - do NOT use const in declaration\n"
     "  -s - do use static in declaration\n"
     "  -v - verbose\n"
     "  -z - add zero terminator\n"
     "  -H - create c-header only\n"
     "  -C - create c-source file only\n"
    );
  exit(1);
}

int main(int argc, char **argv)
{
  while (argc > 3) {
    if (!strcmp(argv[1], "-c")) {
      useconst = 0;
      --argc;
      ++argv;
    } else if (!strcmp(argv[1], "-s")) {
      usestatic = 1;
      --argc;
      ++argv;
    } else if (!strcmp(argv[1], "-v")) {
      verbose = 1;
      --argc;
      ++argv;
    } else if (!strcmp(argv[1], "-z")) {
      zeroterminated = 1;
      --argc;
      ++argv;
    } else if (!strcmp(argv[1], "-C")) {
      createH = 0;
      createC = 1;
      --argc;
      ++argv;
    } else if (!strcmp(argv[1], "-H")) {
      createC = 0;
      createH = 1;
      --argc;
      ++argv;
    } else {
      usage();
    }
  }
  if (argc != 3) {
    usage();
  }

  /* process( input_file, output_basename ) */
  process(argv[1], argv[2]);
  return 0;
}

