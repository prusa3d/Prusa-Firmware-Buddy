/**
 * @file ssi.c
 * @brief SSI (Server Side Includes)
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
 * @section Description
 *
 * Server Side Includes (SSI) is a simple interpreted server-side scripting
 * language used to generate dynamic content to web pages
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL HTTP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "http/http_server.h"
#include "http/http_server_misc.h"
#include "http/mime.h"
#include "http/ssi.h"
#include "str.h"
#include "debug.h"

//File system support?
#if (HTTP_SERVER_FS_SUPPORT == ENABLED)
   #include "fs_port.h"
#else
   #include "resource_manager.h"
#endif

//Check TCP/IP stack configuration
#if (HTTP_SERVER_SUPPORT == ENABLED && HTTP_SERVER_SSI_SUPPORT == ENABLED)


/**
 * @brief Execute SSI script
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] uri NULL-terminated string containing the file to process
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t ssiExecuteScript(HttpConnection *connection, const char_t *uri, uint_t level)
{
   error_t error;
   size_t length;

#if (HTTP_SERVER_FS_SUPPORT == ENABLED)
   bool_t more;
   uint_t pos;
   uint_t n;
   char_t *buffer;
   FsFile *file;
#else
   uint_t i;
   uint_t j;
   const char_t *data;
#endif

   //Recursion limit exceeded?
   if(level >= HTTP_SERVER_SSI_MAX_RECURSION)
      return NO_ERROR;

   //Retrieve the full pathname
   httpGetAbsolutePath(connection, uri,
      connection->buffer, HTTP_SERVER_BUFFER_SIZE);

#if (HTTP_SERVER_FS_SUPPORT == ENABLED)
   //Open the file for reading
   file = fsOpenFile(connection->buffer, FS_FILE_MODE_READ);
   //Failed to open the file?
   if(file == NULL)
      return ERROR_NOT_FOUND;

   //Allocate a memory buffer
   buffer = osAllocMem(HTTP_SERVER_BUFFER_SIZE);
   //Failed to allocate memory?
   if(buffer == NULL)
   {
      //Close the file
      fsCloseFile(file);
      //Report an error
      return ERROR_OUT_OF_MEMORY;
   }
#else
   //Get the resource data associated with the URI
   error = resGetData(connection->buffer, (const uint8_t **) &data, &length);
   //The specified URI cannot be found?
   if(error)
      return error;
#endif

   //Send the HTTP response header before executing the script
   if(!level)
   {
      //Format HTTP response header
      connection->response.statusCode = 200;
      connection->response.contentType = mimeGetType(uri);
      connection->response.chunkedEncoding = TRUE;

      //Send the header to the client
      error = httpWriteHeader(connection);
      //Any error to report?
      if(error)
      {
#if (HTTP_SERVER_FS_SUPPORT == ENABLED)
         //Close the file
         fsCloseFile(file);
         //Release memory buffer
         osFreeMem(buffer);
#endif
         //Return status code
         return error;
      }
   }

#if (HTTP_SERVER_FS_SUPPORT == ENABLED)
   //Point to the beginning of the buffer
   pos = 0;
   length = 0;

   //This flag indicates whether data should be read
   more = TRUE;

   //Parse the specified file
   while(1)
   {
      //Read more data if needed
      if(more)
      {
         //Check whether the current position is aligned on 32-bit boundaries
         n = 4 - ((pos + length) % 4);

         //Maintain proper alignment
         if(n != 4)
         {
            osMemmove(buffer + pos + n, buffer + pos, length);
            pos += n;
         }

         //Read data from the specified file
         error = fsReadFile(file, buffer + pos + length,
            HTTP_SERVER_BUFFER_SIZE - (pos + length), &n);

         //End of input stream?
         if(error)
         {
            //Purge data buffer
            error = httpWriteStream(connection, buffer + pos, length);
            //Exit immediately
            break;
         }

         //Adjust the length of the buffer
         length += n;
         //Clear flag
         more = FALSE;
      }

      //Search for any SSI tags
      error = ssiSearchTag(buffer + pos, length, "<!--#", 5, &n);

      //Full match?
      if(error == NO_ERROR)
      {
         //Send the part of the file that precedes the tag
         error = httpWriteStream(connection, buffer + pos, n);
         //Failed to send data?
         if(error)
            break;

         //Advance data pointer
         pos += n;
         length -= n;

         //Search for the comment terminator
         error = ssiSearchTag(buffer + pos + 5, length - 5, "-->", 3, &n);

         //Full match?
         if(error == NO_ERROR)
         {
            //Advance data pointer over the opening identifier
            pos += 5;
            length -= 5;

            //Process SSI directive
            error = ssiProcessCommand(connection, buffer + pos, n, uri, level);
            //Any error to report?
            if(error)
               break;

            //Advance data pointer over the SSI tag
            pos += n + 3;
            length -= n + 3;
         }
         //No match or partial match?
         else
         {
            if(pos > 0)
            {
               //Move the remaining bytes to the start of the buffer
               osMemmove(buffer, buffer + pos, length);
               //Rewind to the beginning of the buffer
               pos = 0;
               //More data are needed
               more = TRUE;
            }
            else
            {
               //Send data to the client
               error = httpWriteStream(connection, buffer + pos, length);
               //Any error to report?
               if(error)
                  break;

               //Rewind to the beginning of the buffer
               pos = 0;
               length = 0;
               //More data are needed
               more = TRUE;
            }
         }
      }
      //Partial match?
      else if(error == ERROR_PARTIAL_MATCH)
      {
         //Send the part of the file that precedes the tag
         error = httpWriteStream(connection, buffer + pos, n);
         //Failed to send data?
         if(error)
            break;

         //Advance data pointer
         pos += n;
         length -= n;

         //Move the remaining bytes to the start of the buffer
         osMemmove(buffer, buffer + pos, length);
         //Rewind to the beginning of the buffer
         pos = 0;
         //More data are needed
         more = TRUE;
      }
      //No match?
      else
      {
         //Send data to the client
         error = httpWriteStream(connection, buffer + pos, length);
         //Any error to report?
         if(error)
            break;

         //Rewind to the beginning of the buffer
         pos = 0;
         length = 0;
         //More data are needed
         more = TRUE;
      }
   }

   //Close the file
   fsCloseFile(file);
   //Release memory buffer
   osFreeMem(buffer);

   //Properly close the output stream
   if(!level && error == NO_ERROR)
      error = httpCloseStream(connection);
#else
   //Parse the specified file
   while(length > 0)
   {
      //Search for any SSI tags
      error = ssiSearchTag(data, length, "<!--#", 5, &i);

      //Opening identifier found?
      if(!error)
      {
         //Search for the comment terminator
         error = ssiSearchTag(data + i + 5, length - i - 5, "-->", 3, &j);
      }

      //Check whether a valid SSI tag has been found?
      if(!error)
      {
         //Send the part of the file that precedes the tag
         error = httpWriteStream(connection, data, i);
         //Failed to send data?
         if(error)
            return error;

         //Advance data pointer over the opening identifier
         data += i + 5;
         length -= i + 5;

         //Process SSI directive
         error = ssiProcessCommand(connection, data, j, uri, level);
         //Any error to report?
         if(error)
            return error;

         //Advance data pointer over the SSI tag
         data += j + 3;
         length -= j + 3;
      }
      else
      {
         //Send the rest of the file
         error = httpWriteStream(connection, data, length);
         //Failed to send data?
         if(error)
            return error;

         //Advance data pointer
         data += length;
         length = 0;
      }
   }

   //Properly close the output stream
   if(!level)
      error = httpCloseStream(connection);
#endif

   //Return status code
   return error;
}


/**
 * @brief Process SSI directive
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] tag Pointer to the SSI tag
 * @param[in] length Total length of the SSI tag
 * @param[in] uri NULL-terminated string containing the file being processed
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t ssiProcessCommand(HttpConnection *connection,
   const char_t *tag, size_t length, const char_t *uri, uint_t level)
{
   error_t error;

   //Include command found?
   if(length > 7 && !osStrncasecmp(tag, "include", 7))
   {
      //Process SSI include directive
      error = ssiProcessIncludeCommand(connection, tag, length, uri, level);
   }
   //Echo command found?
   else if(length > 4 && !osStrncasecmp(tag, "echo", 4))
   {
      //Process SSI echo directive
      error = ssiProcessEchoCommand(connection, tag, length);
   }
   //Exec command found?
   else if(length > 4 && !osStrncasecmp(tag, "exec", 4))
   {
      //Process SSI exec directive
      error = ssiProcessExecCommand(connection, tag, length);
   }
   //Unknown command?
   else
   {
      //The server is unable to decode the SSI tag
      error = ERROR_INVALID_TAG;
   }

   //Invalid SSI directive?
   if(error == ERROR_INVALID_TAG)
   {
      //Report a warning to the user
      error = httpWriteStream(connection, "Warning: Invalid SSI Tag", 24);
   }

   //Return status code
   return error;
}


/**
 * @brief Process SSI include directive
 *
 * This include directive allows the content of one document to be included
 * in another. The file parameter defines the included file as relative to
 * the document path. The virtual parameter defines the included file as
 * relative to the document root
 *
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] tag Pointer to the SSI tag
 * @param[in] length Total length of the SSI tag
 * @param[in] uri NULL-terminated string containing the file being processed
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t ssiProcessIncludeCommand(HttpConnection *connection,
   const char_t *tag, size_t length, const char_t *uri, uint_t level)
{
   error_t error;
   char_t *separator;
   char_t *attribute;
   char_t *value;
   char_t *path;
   char_t *p;

   //Discard invalid SSI directives
   if(length < 7 || length >= HTTP_SERVER_BUFFER_SIZE)
      return ERROR_INVALID_TAG;

   //Skip the SSI include command (7 bytes)
   osMemcpy(connection->buffer, tag + 7, length - 7);
   //Ensure the resulting string is NULL-terminated
   connection->buffer[length - 7] = '\0';

   //Check whether a separator is present
   separator = osStrchr(connection->buffer, '=');
   //Separator not found?
   if(!separator)
      return ERROR_INVALID_TAG;

   //Split the tag
   *separator = '\0';

   //Get attribute name and value
   attribute = strTrimWhitespace(connection->buffer);
   value = strTrimWhitespace(separator + 1);

   //Remove leading simple or double quote
   if(value[0] == '\'' || value[0] == '\"')
      value++;

   //Get the length of the attribute value
   length = osStrlen(value);

   //Remove trailing simple or double quote
   if(length > 0)
   {
      if(value[length - 1] == '\'' || value[length - 1] == '\"')
         value[length - 1] = '\0';
   }

   //Check the length of the filename
   if(osStrlen(value) > HTTP_SERVER_URI_MAX_LEN)
      return ERROR_INVALID_TAG;

   //The file parameter defines the included file as relative to the document path
   if(!osStrcasecmp(attribute, "file"))
   {
      //Allocate a buffer to hold the path to the file to be included
      path = osAllocMem(osStrlen(uri) + osStrlen(value) + 1);
      //Failed to allocate memory?
      if(path == NULL)
         return ERROR_OUT_OF_MEMORY;

      //Copy the path identifying the script file being processed
      osStrcpy(path, uri);
      //Search for the last slash character
      p = strrchr(path, '/');

      //Remove the filename from the path if applicable
      if(p)
         osStrcpy(p + 1, value);
      else
         osStrcpy(path, value);
   }
   //The virtual parameter defines the included file as relative to the document root
   else if(!osStrcasecmp(attribute, "virtual"))
   {
      //Copy the absolute path
      path = strDuplicate(value);
      //Failed to duplicate the string?
      if(path == NULL)
         return ERROR_OUT_OF_MEMORY;
   }
   //Unknown parameter...
   else
   {
      //Report an error
      return ERROR_INVALID_TAG;
   }

   //Use server-side scripting to dynamically generate HTML code?
   if(httpCompExtension(value, ".stm") ||
      httpCompExtension(value, ".shtm") ||
      httpCompExtension(value, ".shtml"))
   {
      //SSI processing (Server Side Includes)
      error = ssiExecuteScript(connection, path, level + 1);
   }
   else
   {
#if (HTTP_SERVER_FS_SUPPORT == ENABLED)
      FsFile *file;

      //Retrieve the full pathname
      httpGetAbsolutePath(connection, path,
         connection->buffer, HTTP_SERVER_BUFFER_SIZE);

      //Open the file for reading
      file = fsOpenFile(connection->buffer, FS_FILE_MODE_READ);

      //Successful operation?
      if(file)
      {
         //Send the contents of the requested file
         while(1)
         {
            //Read data from the specified file
            error = fsReadFile(file, connection->buffer, HTTP_SERVER_BUFFER_SIZE, &length);
            //End of input stream?
            if(error)
               break;

            //Send data to the client
            error = httpWriteStream(connection, connection->buffer, length);
            //Any error to report?
            if(error)
               break;
         }

         //Close the file
         fsCloseFile(file);

         //Successful file transfer?
         if(error == ERROR_END_OF_FILE)
            error = NO_ERROR;
      }
      else
      {
         //The specified URI cannot be found
         error = ERROR_NOT_FOUND;
      }
#else
      const uint8_t *data;

      //Retrieve the full pathname
      httpGetAbsolutePath(connection, path,
         connection->buffer, HTTP_SERVER_BUFFER_SIZE);

      //Get the resource data associated with the file
      error = resGetData(connection->buffer, &data, &length);

      //Send the contents of the requested file
      if(!error)
         error = httpWriteStream(connection, data, length);
#endif
   }

   //Cannot found the specified resource?
   if(error == ERROR_NOT_FOUND)
      error = ERROR_INVALID_TAG;

   //Release previously allocated memory
   osFreeMem(path);
   //return status code
   return error;
}


/**
 * @brief Process SSI echo directive
 *
 * This echo directive displays the contents of a specified
 * HTTP environment variable
 *
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] tag Pointer to the SSI tag
 * @param[in] length Total length of the SSI tag
 * @return Error code
 **/

error_t ssiProcessEchoCommand(HttpConnection *connection, const char_t *tag, size_t length)
{
   error_t error;
   char_t *separator;
   char_t *attribute;
   char_t *value;

   //Discard invalid SSI directives
   if(length < 4 || length >= HTTP_SERVER_BUFFER_SIZE)
      return ERROR_INVALID_TAG;

   //Skip the SSI echo command (4 bytes)
   osMemcpy(connection->buffer, tag + 4, length - 4);
   //Ensure the resulting string is NULL-terminated
   connection->buffer[length - 4] = '\0';

   //Check whether a separator is present
   separator = osStrchr(connection->buffer, '=');
   //Separator not found?
   if(!separator)
      return ERROR_INVALID_TAG;

   //Split the tag
   *separator = '\0';

   //Get attribute name and value
   attribute = strTrimWhitespace(connection->buffer);
   value = strTrimWhitespace(separator + 1);

   //Remove leading simple or double quote
   if(value[0] == '\'' || value[0] == '\"')
      value++;

   //Get the length of the attribute value
   length = osStrlen(value);

   //Remove trailing simple or double quote
   if(length > 0)
   {
      if(value[length - 1] == '\'' || value[length - 1] == '\"')
         value[length - 1] = '\0';
   }

   //Enforce attribute name
   if(osStrcasecmp(attribute, "var"))
      return ERROR_INVALID_TAG;

   //Remote address?
   if(!osStrcasecmp(value, "REMOTE_ADDR"))
   {
      //The IP address of the host making this request
      ipAddrToString(&connection->socket->remoteIpAddr, connection->buffer);
   }
   //Remote port?
   else if(!osStrcasecmp(value, "REMOTE_PORT"))
   {
      //The port number used by the remote host when making this request
      osSprintf(connection->buffer, "%" PRIu16, connection->socket->remotePort);
   }
   //Server address?
   else if(!osStrcasecmp(value, "SERVER_ADDR"))
   {
      //The IP address of the server for this URL
      ipAddrToString(&connection->socket->localIpAddr, connection->buffer);
   }
   //Server port?
   else if(!osStrcasecmp(value, "SERVER_PORT"))
   {
      //The port number on this server to which this request was directed
      osSprintf(connection->buffer, "%" PRIu16, connection->socket->localPort);
   }
   //Request method?
   else if(!osStrcasecmp(value, "REQUEST_METHOD"))
   {
      //The method used for this HTTP request
      osStrcpy(connection->buffer, connection->request.method);
   }
   //Document root?
   else if(!osStrcasecmp(value, "DOCUMENT_ROOT"))
   {
      //The root directory
      osStrcpy(connection->buffer, connection->settings->rootDirectory);
   }
   //Document URI?
   else if(!osStrcasecmp(value, "DOCUMENT_URI"))
   {
      //The URI for this request relative to the root directory
      osStrcpy(connection->buffer, connection->request.uri);
   }
   //Document name?
   else if(!osStrcasecmp(value, "DOCUMENT_NAME"))
   {
      //The full physical path and filename of the document requested
      httpGetAbsolutePath(connection, connection->request.uri,
         connection->buffer, HTTP_SERVER_BUFFER_SIZE);
   }
   //Query string?
   else if(!osStrcasecmp(value, "QUERY_STRING"))
   {
      //The information following the "?" in the URL for this request
      osStrcpy(connection->buffer, connection->request.queryString);
   }
   //User name?
   else if(!osStrcasecmp(value, "AUTH_USER"))
   {
#if (HTTP_SERVER_BASIC_AUTH_SUPPORT == ENABLED || HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
      //The username provided by the user to the server
      osStrcpy(connection->buffer, connection->request.auth.user);
#else
      //Basic access authentication is not supported
      connection->buffer[0] = '\0';
#endif
   }
   //GMT time?
   else if(!osStrcasecmp(value, "DATE_GMT"))
   {
      //The current date and time in Greenwich Mean Time
      connection->buffer[0] = '\0';
   }
   //Local time?
   else if(!osStrcasecmp(value, "DATE_LOCAL"))
   {
      //The current date and time in the local timezone
      connection->buffer[0] = '\0';
   }
   //Unknown variable?
   else
   {
      //Report an error
      return ERROR_INVALID_TAG;
   }

   //Get the length of the resulting string
   length = osStrlen(connection->buffer);

   //Send the contents of the specified environment variable
   error = httpWriteStream(connection, connection->buffer, length);
   //Failed to send data?
   if(error)
      return error;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process SSI exec directive
 *
 * This exec directive executes a program, script, or shell command on
 * the server. The cmd parameter specifies a server-side command. The
 * cgi parameter specifies the path to a CGI script
 *
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] tag Pointer to the SSI tag
 * @param[in] length Total length of the SSI tag
 * @return Error code
 **/

error_t ssiProcessExecCommand(HttpConnection *connection, const char_t *tag, size_t length)
{
   char_t *separator;
   char_t *attribute;
   char_t *value;

   //First, check whether CGI is supported by the server
   if(connection->settings->cgiCallback == NULL)
      return ERROR_INVALID_TAG;

   //Discard invalid SSI directives
   if(length < 4 || length >= HTTP_SERVER_BUFFER_SIZE)
      return ERROR_INVALID_TAG;

   //Skip the SSI exec command (4 bytes)
   osMemcpy(connection->buffer, tag + 4, length - 4);
   //Ensure the resulting string is NULL-terminated
   connection->buffer[length - 4] = '\0';

   //Check whether a separator is present
   separator = osStrchr(connection->buffer, '=');
   //Separator not found?
   if(!separator)
      return ERROR_INVALID_TAG;

   //Split the tag
   *separator = '\0';

   //Get attribute name and value
   attribute = strTrimWhitespace(connection->buffer);
   value = strTrimWhitespace(separator + 1);

   //Remove leading simple or double quote
   if(value[0] == '\'' || value[0] == '\"')
      value++;

   //Get the length of the attribute value
   length = osStrlen(value);

   //Remove trailing simple or double quote
   if(length > 0)
   {
      if(value[length - 1] == '\'' || value[length - 1] == '\"')
         value[length - 1] = '\0';
   }

   //Enforce attribute name
   if(osStrcasecmp(attribute, "cgi") && osStrcasecmp(attribute, "cmd") && osStrcasecmp(attribute, "cmd_argument"))
      return ERROR_INVALID_TAG;
   //Check the length of the CGI parameter
   if(osStrlen(value) > HTTP_SERVER_CGI_PARAM_MAX_LEN)
      return ERROR_INVALID_TAG;

   //The scratch buffer may be altered by the user-defined callback.
   //So the CGI parameter must be copied prior to function invocation
   osStrcpy(connection->cgiParam, value);

   //Invoke user-defined callback
   return connection->settings->cgiCallback(connection, connection->cgiParam);
}


/**
 * @brief Search a string for a given tag
 * @param[in] s String to search
 * @param[in] sLen Length of the string to search
 * @param[in] tag String containing the tag to search for
 * @param[in] tagLen Length of the tag
 * @param[out] pos The index of the first occurrence of the tag in the string,
 * @retval NO_ERROR if the specified tag has been found
 * @retval ERROR_PARTIAL_MATCH if a partial match occurs
 * @retval ERROR_NO_MATCH if the tag does not appear in the string
 **/

error_t ssiSearchTag(const char_t *s, size_t sLen, const char_t *tag, size_t tagLen, uint_t *pos)
{
   uint_t i;
   uint_t j;

   //Parse the input string
   for(i = 0; i <= sLen; i++)
   {
      //Compare current substring with the given tag
      for(j = 0; (i + j) < sLen && j < tagLen; j++)
      {
         if(s[i + j] != tag[j])
            break;
      }

      //Check whether a full match occurred
      if(j == tagLen)
      {
         //Save the position of the first character
         *pos = i;
         //The specified tag has been found
         return NO_ERROR;
      }
      //Check whether a partial match occurred
      else if((i + j) == sLen && j > 0)
      {
         //Save the position of the first character
         *pos = i;
         //The beginning of the tag matches the end of the string
         return ERROR_PARTIAL_MATCH;
      }
   }

   //The tag does not appear in the string
   return ERROR_NO_MATCH;
}

#endif
