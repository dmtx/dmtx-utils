/*
 * libdmtx - Data Matrix Encoding/Decoding Library
 *
 * Copyright (C) 2011 Mike Laughton
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact: mike@dragonflylogic.com
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <dmtx.h>
#include "dmtxutil.h"

#undef ISDIGIT
#define ISDIGIT(n) (n > 47 && n < 58)

extern char *programName;

/**
 * @brief  Display error message and exit with error status
 * @param  errorCode error code returned to OS
 * @param  fmt error message format for printing
 * @return void
 */
extern void
FatalError(int errorCode, char *fmt, ...)
{
   va_list va;

   va_start(va, fmt);
   fprintf(stderr, "%s: ", programName);
   vfprintf(stderr, fmt, va);
   va_end(va);
   fprintf(stderr, "\n\n");
   fflush(stderr);

   exit(errorCode);
}

/**
 * @brief  Convert character string to integer
 * @param  numberInt pointer to converted integer
 * @param  numberString string to be converted
 * @param  terminate pointer to first invalid address
 * @return DmtxPass | DmtxFail
 */
extern DmtxPassFail
StringToInt(int *numberInt, char *numberString, char **terminate)
{
   long numberLong;

   if(!ISDIGIT(*numberString)) {
      *numberInt = DmtxUndefined;
      return DmtxFail;
   }

   errno = 0;
   numberLong = strtol(numberString, terminate, 10);

   while(isspace((int)**terminate))
      (*terminate)++;

   if(errno != 0 || (**terminate != '\0' && **terminate != '%')) {
      *numberInt = DmtxUndefined;
      return DmtxFail;
   }

   *numberInt = (int)numberLong;

   return DmtxPass;
}

/**
 * @brief  XXX
 * @param  path
 * @return pointer to adjusted path string
 */
extern char *
Basename(char *path)
{
   assert(path);

   if(strrchr(path, '/'))
      path = strrchr(path, '/') + 1;

   if(strrchr(path, '\\'))
      path = strrchr(path, '\\') + 1;

   return path;
}
