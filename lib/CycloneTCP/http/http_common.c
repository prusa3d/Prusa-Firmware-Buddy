/**
 * @file http_common.c
 * @brief Definitions common to HTTP client and server
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2021 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneTCP Open.
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

//Switch to the appropriate trace level
#define TRACE_LEVEL HTTP_TRACE_LEVEL

//Dependencies
#include <ctype.h>
#include "core/net.h"
#include "http/http_common.h"
#include "debug.h"


/**
 * @brief Check whether a string contains valid characters
 * @param[in] s Pointer to the string
 * @param[in] length Length of the string
 * @param[in] charset Acceptable charset
 * @return Error code
 **/

error_t httpCheckCharset(const char_t *s, size_t length, uint_t charset)
{
   error_t error;
   size_t i;
   uint8_t c;
   uint_t m;

   //Initialize status code
   error = NO_ERROR;

   //Parse string
   for(i = 0; i < length; i++)
   {
      //Get current character
      c = (uint8_t) s[i];

      //Any 8-bit sequence of data
      m = HTTP_CHARSET_OCTET;

      //Check if character is a control character
      if(iscntrl(c))
         m |= HTTP_CHARSET_CTL;

      //Check if character is printable
      if(isprint(c) && c <= 126)
         m |= HTTP_CHARSET_TEXT | HTTP_CHARSET_VCHAR;

      //Check if character is blank
      if(c == ' ' || c == '\t')
         m |= HTTP_CHARSET_TEXT | HTTP_CHARSET_LWS;

      //Check if character is alphabetic
      if(isalpha(c))
         m |= HTTP_CHARSET_TCHAR | HTTP_CHARSET_ALPHA;

      //Check if character is decimal digit
      if(osIsdigit(c))
         m |= HTTP_CHARSET_TCHAR | HTTP_CHARSET_DIGIT;

      //Check if character is hexadecimal digit
      if(isxdigit(c))
         m |= HTTP_CHARSET_HEX;

      //Check if character is in the extended character set
      if(c >= 128)
         m |= HTTP_CHARSET_TEXT | HTTP_CHARSET_OBS_TEXT;

      //Check if character is a token character
      if(osStrchr("!#$%&'*+-.^_`|~", c))
         m |= HTTP_CHARSET_TCHAR;

      //Invalid character?
      if((m & charset) == 0)
         error = ERROR_INVALID_SYNTAX;
   }

   //Return status code
   return error;
}


/**
 * @brief Parse a list of parameters
 * @param[in,out] pos Actual position if the list of parameters
 * @param[out] param Structure that contains the parameter name and value
 * @return Error code
 **/

error_t httpParseParam(const char_t **pos, HttpParam *param)
{
   error_t error;
   size_t i;
   uint8_t c;
   bool_t escapeFlag;
   bool_t separatorFound;
   const char_t *p;

   //Check parameters
   if(pos == NULL || param == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize structure
   param->name = NULL;
   param->nameLen = 0;
   param->value = NULL;
   param->valueLen = 0;

   //Initialize variables
   escapeFlag = FALSE;
   separatorFound = FALSE;

   //Initialize status code
   error = ERROR_IN_PROGRESS;

   //Point to the first character
   i = 0;
   p = *pos;

   //Loop through the list of parameters
   while(error == ERROR_IN_PROGRESS)
   {
      //Get current character
      c = (uint8_t) p[i];

      //Check current state
      if(param->name == NULL)
      {
         //Check current character
         if(c == '\0')
         {
            //The list of parameters is empty
            error = ERROR_NOT_FOUND;
         }
         else if(c == ' ' || c == '\t' || c == ',' || c == ';')
         {
            //Discard whitespace and separator characters
         }
         else if(isalnum(c) || osStrchr("!#$%&'*+-.^_`|~", c) || c >= 128)
         {
            //Point to the first character of the parameter name
            param->name = p + i;
         }
         else
         {
            //Invalid character
            error = ERROR_INVALID_SYNTAX;
         }
      }
      else if(param->nameLen == 0)
      {
         //Check current character
         if(c == '\0' || c == ',' || c == ';')
         {
            //Save the length of the parameter name
            param->nameLen = p + i - param->name;
            //Successful processing
            error = NO_ERROR;
         }
         else if(c == ' ' || c == '\t')
         {
            //Save the length of the parameter name
            param->nameLen = p + i - param->name;
         }
         else if(c == '=')
         {
            //The key/value separator has been found
            separatorFound = TRUE;
            //Save the length of the parameter name
            param->nameLen = p + i - param->name;
         }
         else if(isalnum(c) || osStrchr("!#$%&'*+-.^_`|~", c) || c >= 128)
         {
            //Advance data pointer
         }
         else
         {
            //Invalid character
            error = ERROR_INVALID_SYNTAX;
         }
      }
      else if(!separatorFound)
      {
         //Check current character
         if(c == '\0' || c == ',' || c == ';')
         {
            //Successful processing
            error = NO_ERROR;
         }
         else if(c == ' ' || c == '\t')
         {
            //Discard whitespace characters
         }
         else if(c == '=')
         {
            //The key/value separator has been found
            separatorFound = TRUE;
         }
         else if(c == '\"')
         {
            //Point to the first character that follows the parameter name
            i = param->name + param->nameLen - p;
            //Successful processing
            error = NO_ERROR;
         }
         else if(isalnum(c) || osStrchr("!#$%&'*+-.^_`|~", c) || c >= 128)
         {
            //Point to the first character that follows the parameter name
            i = param->name + param->nameLen - p;
            //Successful processing
            error = NO_ERROR;
         }
         else
         {
            //Invalid character
            error = ERROR_INVALID_SYNTAX;
         }
      }
      else if(param->value == NULL)
      {
         //Check current character
         if(c == '\0' || c == ',' || c == ';')
         {
            //Successful processing
            error = NO_ERROR;
         }
         else if(c == ' ' || c == '\t')
         {
            //Discard whitespace characters
         }
         else if(c == '\"')
         {
            //A string of text is parsed as a single word if it is quoted
            //using double-quote marks (refer to RFC 7230, section 3.2.6)
            param->value = p + i;
         }
         else if(isalnum(c) || osStrchr("!#$%&'*+-.^_`|~", c) || c >= 128)
         {
            //Point to the first character of the parameter value
            param->value = p + i;
         }
         else
         {
            //Invalid character
            error = ERROR_INVALID_SYNTAX;
         }
      }
      else
      {
         //Quoted string?
         if(param->value[0] == '\"')
         {
            //Check current character
            if(c == '\0')
            {
               //The second double quote is missing
               error = ERROR_INVALID_SYNTAX;
            }
            else if(escapeFlag)
            {
               //Recipients that process the value of a quoted-string must
               //handle a quoted-pair as if it were replaced by the octet
               //following the backslash
               escapeFlag = FALSE;
            }
            else if(c == '\\')
            {
               //The backslash octet can be used as a single-octet quoting
               //mechanism within quoted-string and comment constructs
               escapeFlag = TRUE;
            }
            else if(c == '\"')
            {
               //Advance pointer over the double quote
               i++;
               //Save the length of the parameter value
               param->valueLen = p + i - param->value;
               //Successful processing
               error = NO_ERROR;
            }
            else if(isprint(c) || c == '\t' || c >= 128)
            {
               //Advance data pointer
            }
            else
            {
               //Invalid character
               error = ERROR_INVALID_SYNTAX;
            }
         }
         else
         {
            //Check current character
            if(c == '\0' || c == ' ' || c == '\t' || c == ',' || c == ';')
            {
               //Save the length of the parameter value
               param->valueLen = p + i - param->value;
               //Successful processing
               error = NO_ERROR;
            }
            else if(isalnum(c) || osStrchr("!#$%&'*+-.^_`|~", c) || c >= 128)
            {
               //Advance data pointer
            }
            else
            {
               //Invalid character
               error = ERROR_INVALID_SYNTAX;
            }
         }
      }

      //Point to the next character of the string
      if(error == ERROR_IN_PROGRESS)
         i++;
   }

   //Check whether the parameter value is a quoted string
   if(param->valueLen >= 2 && param->value[0] == '\"')
   {
      //Discard the surrounding quotes
      param->value++;
      param->valueLen -= 2;
   }

   //Actual position if the list of parameters
   *pos = p + i;

   //Return status code
   return error;
}


/**
 * @brief Compare parameter name with the supplied string
 * @param[in] param Pointer to the parameter
 * @param[in] name NULL-terminated string
 * @return Comparison result
 **/

bool_t httpCompareParamName(const HttpParam *param, const char_t *name)
{
   bool_t res;
   size_t n;

   //Initialize flag
   res = FALSE;

   //Determine the length of the string
   n = osStrlen(name);

   //Check the length of the parameter name
   if(param->name != NULL && param->nameLen == n)
   {
      //Compare names
      if(!osStrncasecmp(param->name, name, n))
      {
         res = TRUE;
      }
   }

   //Return comparison result
   return res;
}


/**
 * @brief Compare parameter name with the supplied string
 * @param[in] param Pointer to the parameter
 * @param[in] value NULL-terminated string
 * @return Comparison result
 **/

bool_t httpCompareParamValue(const HttpParam *param, const char_t *value)
{
   bool_t res;
   size_t n;

   //Initialize flag
   res = FALSE;

   //Determine the length of the string
   n = osStrlen(value);

   //Check the length of the parameter value
   if(param->value != NULL && param->valueLen == n)
   {
      //Perform case-insensitive comparison
      if(!osStrncasecmp(param->value, value, n))
      {
         res = TRUE;
      }
   }

   //Return comparison result
   return res;
}


/**
 * @brief Copy the value of a parameter
 * @param[in] param Pointer to the parameter
 * @param[out] value Pointer to the buffer where to copy the parameter value
 * @param[out] maxLen Maximum number of characters the buffer can hold
 * @return Error code
 **/

error_t httpCopyParamValue(const HttpParam *param, char_t *value,
   size_t maxLen)
{
   error_t error;
   size_t n;

   //Initialize status code
   error = NO_ERROR;

   //Check the length of the parameter value
   if(param->valueLen <= maxLen)
   {
      //Get the length of the string
      n = param->valueLen;
   }
   else
   {
      //Limit the number of characters to copy
      n = maxLen;
      //Report an error
      error = ERROR_BUFFER_OVERFLOW;
   }

   //Copy the value of the parameter
   osMemcpy(value, param->value, n);
   //Properly terminate the string with a NULL character
   value[n] = '\0';

   //Return status code
   return error;
}


/**
 * @brief Convert byte array to hex string
 * @param[in] input Point to the byte array
 * @param[in] inputLen Length of the byte array
 * @param[out] output NULL-terminated string resulting from the conversion
 **/

void httpEncodeHexString(const uint8_t *input, size_t inputLen, char_t *output)
{
   int_t i;

   //Hex conversion table
   static const char_t hexDigit[16] =
   {
      '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
   };

   //Process byte array
   for(i = inputLen - 1; i >= 0; i--)
   {
      //Convert lower nibble
      output[i * 2 + 1] = hexDigit[input[i] & 0x0F];
      //Then convert upper nibble
      output[i * 2] = hexDigit[(input[i] >> 4) & 0x0F];
   }

   //Properly terminate the string with a NULL character
   output[inputLen * 2] = '\0';
}
