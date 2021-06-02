/**
 * @file resource_manager.c
 * @brief Embedded resource management
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2021 Oryx Embedded SARL. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Dependencies
#include <string.h>
#include "os_port.h"
#include "resource_manager.h"
#include "cyclone_debug.h"

//Resource data
extern const uint8_t res[];


error_t resGetData(const char_t *path, const uint8_t **data, size_t *length)
{
   bool_t found;
   bool_t match;
   uint_t n;
   uint_t dirLength;
   ResEntry *resEntry;

   //Point to the resource header
   ResHeader *resHeader = (ResHeader *) res;

   //Make sure the resource data is valid
   if(resHeader->totalSize < sizeof(ResHeader))
      return ERROR_INVALID_RESOURCE;

   //Retrieve the length of the root directory
   dirLength = resHeader->rootEntry.dataLength;
   //Point to the contents of the root directory
   resEntry = (ResEntry *) (res + resHeader->rootEntry.dataStart);

   //Parse the entire path
   for(found = FALSE; !found && path[0] != '\0'; path += n + 1)
   {
      //Search for the separator that terminates the current token
      for(n = 0; path[n] != '\\' && path[n] != '/' && path[n] != '\0'; n++);

      if(n == 0 && path[n] != '\0')
      {
         path++;
         for(n = 0; path[n] != '\\' && path[n] != '/' && path[n] != '\0'; n++);
      }

      //Loop through the directory
      for(match = FALSE; !match && dirLength > 0; )
      {
         //Check the number of remaining bytes
         if(dirLength < sizeof(ResEntry))
            return ERROR_INVALID_RESOURCE;
         //Make sure the entry is valid
         if(dirLength < (sizeof(ResEntry) + resEntry->nameLength))
            return ERROR_INVALID_RESOURCE;

         //Compare current entry name against the expected one
         if(resEntry->nameLength == n && !strncasecmp(resEntry->name, path, n))
         {
            //Check the type of the entry
            if(resEntry->type == RES_TYPE_DIR)
            {
               //Save the length of the directory
               dirLength = resEntry->dataLength;
               //Point to the contents of the directory
               resEntry = (ResEntry *) (res + resEntry->dataStart);
            }
            else
            {
               //A file may only appear at the end of the path
               if(path[n] != '\0')
                  return ERROR_NOT_FOUND;

               //The search process is complete
               found = TRUE;
            }
            //The current entry matches the specified path
            match = TRUE;
         }
         else
         {
            //Remaining bytes to process
            dirLength -= sizeof(ResEntry) + resEntry->nameLength;
            //Point to the next entry
            resEntry = (ResEntry *) ((uint8_t *) resEntry + sizeof(ResEntry) + resEntry->nameLength);
         }
      }

      //Unable to find the specified file?
      if(!match)
         return ERROR_NOT_FOUND;
   }

   //Unable to find the specified file?
   if(!found)
      return ERROR_NOT_FOUND;
   //Enforce the entry type
   if(resEntry->type != RES_TYPE_FILE)
      return ERROR_NOT_FOUND;

   //Return the location of the specified resource
   *data = res + resEntry->dataStart;
   //Return the length of the resource
   *length = resEntry->dataLength;

   //Successful processing
   return NO_ERROR;
}


error_t resSearchFile(const char_t *path, DirEntry *dirEntry)
{
   bool_t found;
   bool_t match;
   uint_t n;
   uint_t length;
   ResEntry *resEntry;

   //Point to the resource header
   ResHeader *resHeader = (ResHeader *) res;

   //Make sure the resource data is valid
   if(resHeader->totalSize < sizeof(ResHeader))
      return ERROR_INVALID_RESOURCE;

   //Retrieve the length of the root directory
   length = resHeader->rootEntry.dataLength;
   //Point to the contents of the root directory
   resEntry = (ResEntry *) (res + resHeader->rootEntry.dataStart);

   //Parse the entire path
   for(found = FALSE; !found && path[0] != '\0'; path += n + 1)
   {
      //Search for the separator that terminates the current token
      for(n = 0; path[n] != '\\' && path[n] != '/' && path[n] != '\0'; n++);

      if(n == 0 && path[n] != '\0')
      {
         path++;
         for(n = 0; path[n] != '\\' && path[n] != '/' && path[n] != '\0'; n++);
      }

      //Loop through the directory
      for(match = FALSE; !match && length > 0; )
      {
         //Check the number of remaining bytes
         if(length < sizeof(ResEntry))
            return ERROR_INVALID_RESOURCE;
         //Make sure the entry is valid
         if(length < (sizeof(ResEntry) + resEntry->nameLength))
            return ERROR_INVALID_RESOURCE;

         //Compare current entry name against the expected one
         if(resEntry->nameLength == n && !strncasecmp(resEntry->name, path, n))
         {
            //Check the type of the entry
            if(resEntry->type == RES_TYPE_DIR)
            {
               //Save the length of the directory
               length = resEntry->dataLength;
               //Point to the contents of the directory
               resEntry = (ResEntry *) (res + resEntry->dataStart);
            }
            else
            {
               //A file may only appear at the end of the path
               if(path[n] != '\0')
                  return ERROR_INVALID_PATH;

               //The search process is complete
               found = TRUE;
            }
            //The current entry matches the specified path
            match = TRUE;
         }
         else
         {
            //Remaining bytes to process
            length -= sizeof(ResEntry) + resEntry->nameLength;
            //Point to the next entry
            resEntry = (ResEntry *) ((uint8_t *) resEntry + sizeof(ResEntry) + resEntry->nameLength);
         }
      }

      //Unable to find the specified file?
      if(!match)
         return ERROR_NOT_FOUND;
   }

   //Unable to find the specified file?
   if(!found)
      return ERROR_NOT_FOUND;

   //Return information about the file
   dirEntry->type = resEntry->type;
   dirEntry->volume = 0;
   dirEntry->dataStart = resEntry->dataStart;
   dirEntry->dataLength = resEntry->dataLength;
   dirEntry->nameLength = 0; //resEntry->nameLength;
   //Copy the filename
   //osStrncpy(dirEntry->name, resEntry->name, dirEntry->nameLength);
   //Properly terminate the filename
   //dirEntry->name[dirEntry->nameLength] = '\0';

   //Successful processing
   return NO_ERROR;
}

#if 0

error_t resOpenFile(FsFile *file, const DirEntry *dirEntry, uint_t mode)
{
   file->mode = mode;
   file->offset = 0;
   file->start = dirEntry->dataStart;
   file->size = dirEntry->dataLength;

   return NO_ERROR;
}


error_t resSeekFile(FsFile *file, uint32_t *position)
{
   return ERROR_NOT_IMPLEMENTED;
}


uint_t resReadFile(FsFile *file, void *data, size_t length)
{
   length = MIN(length, file->size - file->offset);
   osMemcpy(data, res + file->start + file->offset, length);
   file->offset += length;
   return length;
}

FILE *fopen(const char_t *filename, const char_t *mode)
{
   error_t error;
   DirEntry dirEntry;
   FsFile *file;

   error = resSearchFile(filename, &dirEntry);
   if(error)
      return NULL;

   file = osAllocMem(sizeof(FsFile));
   if(!file)
      return NULL;

   error = resOpenFile(file, &dirEntry, MODE_BINARY);
   if(error)
   {
      osFreeMem(file);
      return NULL;
   }

   return (FILE *) file;
}


size_t fread(void *ptr, size_t size, size_t count, FILE *stream)
{
   uint_t n;

   n = resReadFile((FsFile *) stream, ptr, size * count);

   return n / size;
}


int_t fclose(FILE * stream)
{
   osFreeMem(stream);
   //The stream is successfully closed
   return 0;
}


uint_t fileGetSize(FILE *stream)
{
   uint_t n;
   n = ((FsFile *) stream)->size;
   return n;
}

#endif
