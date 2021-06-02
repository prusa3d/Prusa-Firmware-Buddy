/**
 * @file str.c
 * @brief String manipulation helper functions
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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "str.h"


/**
 * @brief Duplicate a string
 * @param[in] s Pointer to a constant NULL-terminated character string
 * @return Address of the string that was copied, or NULL if the string cannot be copied
 **/

char_t *strDuplicate(const char_t *s)
{
   uint_t n;
   char_t *p;

   //Pointer to the newly created string
   p = NULL;

   //Valid string?
   if(s != NULL)
   {
      //Calculate the length occupied by the input string
      n = osStrlen(s) + 1;

      //Allocate memory to hold the new string
      p = osAllocMem(n);

      //Successful memory allocation?
      if(p != NULL)
      {
         //Make a copy of the input string
         osMemcpy(p, s, n);
      }
   }

   //Return a pointer to the newly created string
   return p;
}


/**
 * @brief Removes all leading and trailing whitespace from a string
 * @param[in] s The string that will be trimmed
 * @return String with whitespace stripped from the beginning and end
 **/

char_t *strTrimWhitespace(char_t *s)
{
   char_t *end;
   char_t *result;

   //Trim whitespace from the beginning
   while(isspace((uint8_t) *s))
   {
      s++;
   }

   //Save the current position
   result = s;

   //Search for the first whitespace to remove
   //at the end of the string
   for(end = NULL; *s != '\0'; s++)
   {
      if(!isspace((uint8_t) *s))
         end = NULL;
      else if(!end)
         end = s;
   }

   //Trim whitespace from the end
   if(end)
      *end = '\0';

   //Return the string with leading and
   //trailing whitespace omitted
   return result;
}


/**
 * @brief Removes all trailing whitespace from a string
 * @param[in,out] s Pointer to a NULL-terminated character string
 **/

void strRemoveTrailingSpace(char_t *s)
{
   char_t *end;

   //Search for the first whitespace to remove
   //at the end of the string
   for(end = NULL; *s != '\0'; s++)
   {
      if(!isspace((uint8_t) *s))
         end = NULL;
      else if(!end)
         end = s;
   }

   //Trim whitespace from the end
   if(end)
      *end = '\0';
}


/**
 * @brief Replace all occurrences of the specified character
 * @param[in,out] s Pointer to a NULL-terminated character string
 * @param[in] oldChar The character to be replaced
 * @param[in] newChar The character that will replace all occurrences of oldChar
 **/

void strReplaceChar(char_t *s, char_t oldChar, char_t newChar)
{
   //Parse the specified string
   while(*s != '\0')
   {
      //Remplace all occurrences of the specified character
      if(*s == oldChar)
         *s = newChar;

      //Next character
      s++;
   }
}


/**
 * @brief Copy string
 * @param[out] dest Pointer to the destination string
 * @param[in] src Pointer to the source string
 * @param[in] destSize Size of the buffer allocated for the destination string
 * @return Error code
 **/

error_t strSafeCopy(char_t *dest, const char_t *src, size_t destSize)
{
   size_t n;

   //Check parameters
   if(dest == NULL || src == NULL || destSize < 1)
      return ERROR_INVALID_PARAMETER;

   //Get the length of the source name
   n = osStrlen(src);
   //Limit the number of characters to be copied
   n = MIN(n, destSize - 1);

   //Copy the string
   osStrncpy(dest, src, n);
   //Properly terminate the string with a NULL character
   dest[n] = '\0';

   //Successful processing
   return NO_ERROR;
}
