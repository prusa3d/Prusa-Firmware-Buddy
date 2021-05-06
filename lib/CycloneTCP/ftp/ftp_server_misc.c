/**
 * @file ftp_server_misc.c
 * @brief Helper functions for FTP server
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
#define TRACE_LEVEL FTP_TRACE_LEVEL

//Dependencies
#include "ftp/ftp_server.h"
#include "ftp/ftp_server_control.h"
#include "ftp/ftp_server_data.h"
#include "ftp/ftp_server_misc.h"
#include "path.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (FTP_SERVER_SUPPORT == ENABLED)


/**
 * @brief Handle periodic operations
 * @param[in] context Pointer to the FTP server context
 **/

void ftpServerTick(FtpServerContext *context)
{
   uint_t i;
   systime_t time;
   FtpClientConnection *connection;

   //Get current time
   time = osGetSystemTime();

   //Loop through the connection table
   for(i = 0; i < context->settings.maxConnections; i++)
   {
      //Point to the current entry
      connection = &context->connections[i];

      //Check the state of the current connection
      if(connection->controlChannel.state != FTP_CHANNEL_STATE_CLOSED)
      {
         //Disconnect inactive client after idle timeout
         if(timeCompare(time, connection->timestamp + FTP_SERVER_TIMEOUT) >= 0)
         {
            //Debug message
            TRACE_INFO("FTP server: Closing inactive connection...\r\n");
            //Close connection with the client
            ftpServerCloseConnection(connection);
         }
      }
   }
}


/**
 * @brief Get a passive port number
 * @param[in] context Pointer to the FTP server context
 * @return Passive port number
 **/

uint16_t ftpServerGetPassivePort(FtpServerContext *context)
{
   uint_t port;

   //Retrieve current passive port number
   port = context->passivePort;

   //Invalid port number?
   if(port < context->settings.passivePortMin ||
      port > context->settings.passivePortMax)
   {
      //Generate a random port number
      port = context->settings.passivePortMin + netGetRand() %
         (context->settings.passivePortMax - context->settings.passivePortMin + 1);
   }

   //Next passive port to use
   if(port < context->settings.passivePortMax)
   {
      //Increment port number
      context->passivePort = port + 1;
   }
   else
   {
      //Wrap around if necessary
      context->passivePort = context->settings.passivePortMin;
   }

   //Return the passive port number
   return port;
}


/**
 * @brief Retrieve the full pathname
 * @param[in] connection Pointer to the client connection
 * @param[in] inputPath Relative or absolute path
 * @param[out] outputPath Resulting full path
 * @param[in] maxLen Maximum acceptable path length
 * @return Error code
 **/

error_t ftpServerGetPath(FtpClientConnection *connection,
   const char_t *inputPath, char_t *outputPath, size_t maxLen)
{
   size_t n;

   //Relative or absolute path?
   if(pathIsRelative(inputPath))
   {
      //Sanity check
      if(osStrlen(connection->currentDir) > maxLen)
         return ERROR_FAILURE;

      //Copy current directory
      osStrcpy(outputPath, connection->currentDir);
      //Append the specified path
      pathCombine(outputPath, inputPath, maxLen);
   }
   else
   {
      //Sanity check
      if(osStrlen(connection->homeDir) > maxLen)
         return ERROR_FAILURE;

      //Copy home directory
      osStrcpy(outputPath, connection->homeDir);
      //Append the specified path
      pathCombine(outputPath, inputPath, maxLen);
   }

   //Clean the resulting path
   pathCanonicalize(outputPath);
   pathRemoveSlash(outputPath);

   //Calculate the length of the home directory
   n = osStrlen(connection->homeDir);

   //Make sure the pathname is valid
   if(osStrncmp(outputPath, connection->homeDir, n))
      return ERROR_INVALID_PATH;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Get permissions for the specified file or directory
 * @param[in] connection Pointer to the client connection
 * @param[in] path Canonical path of the file
 * @return Access rights for the specified file
 **/

uint_t ftpServerGetFilePermissions(FtpClientConnection *connection,
   const char_t *path)
{
   size_t n;
   uint_t perm;
   FtpServerContext *context;

   //Point to the FTP server context
   context = connection->context;

   //Calculate the length of the home directory
   n = osStrlen(connection->homeDir);

   //Make sure the pathname is valid
   if(!osStrncmp(path, connection->homeDir, n))
   {
      //Strip root directory from the pathname
      path = ftpServerStripRootDir(context, path);

      //Invoke user-defined callback, if any
      if(context->settings.getFilePermCallback != NULL)
      {
         //Retrieve access rights for the specified file
         perm = context->settings.getFilePermCallback(connection,
            connection->user, path);
      }
      else
      {
         //Use default access rights
         perm = FTP_FILE_PERM_LIST | FTP_FILE_PERM_READ | FTP_FILE_PERM_WRITE;
      }
   }
   else
   {
      //The specified pathname is not valid
      perm = 0;
   }

   //Return access rights
   return perm;
}


/**
 * @brief Format a directory entry in UNIX-style format
 * @param[in] dirEntry Pointer to the directory entry
 * @param[in] perm Access rights for the specified file
 * @param[out] buffer Buffer where to format the directory entry
 * @return Length of resulting string, in bytes
 **/

size_t ftpServerFormatDirEntry(const FsDirEntry *dirEntry, uint_t perm,
   char_t *buffer)
{
   size_t n;
   time_t time;
   time_t modified;

   //Abbreviated months
   static const char_t months[13][4] =
   {
      "   ",
      "Jan",
      "Feb",
      "Mar",
      "Apr",
      "May",
      "Jun",
      "Jul",
      "Aug",
      "Sep",
      "Oct",
      "Nov",
      "Dec"
   };

   //Format links, owner, group and size fields
   n = osSprintf(buffer, "----------   1 owner    group    %10" PRIu32,
      dirEntry->size);

   //Check whether the current entry is a directory
   if((dirEntry->attributes & FS_FILE_ATTR_DIRECTORY) != 0)
   {
      buffer[0] = 'd';
   }

   //Read access permitted?
   if((perm & FTP_FILE_PERM_READ) != 0)
   {
      buffer[1] = 'r';
      buffer[4] = 'r';
      buffer[7] = 'r';
   }

   //Write access permitted?
   if((perm & FTP_FILE_PERM_WRITE) != 0)
   {
      //Make sure the file is not marked as read-only
      if((dirEntry->attributes & FS_FILE_ATTR_READ_ONLY) == 0)
      {
         buffer[2] = 'w';
         buffer[5] = 'w';
         buffer[8] = 'w';
      }
   }

   //Get current time
   time = getCurrentUnixTime();
   //Get modification time
   modified = convertDateToUnixTime(&dirEntry->modified);

   //Check whether the modification time is within the previous 180 days
   if(time > modified && time < (modified + FTP_SERVER_180_DAYS))
   {
      //The format of the date/time field is Mmm dd hh:mm
      n += osSprintf(buffer + n, " %s %02" PRIu8 " %02" PRIu8 ":%02" PRIu8,
         months[MIN(dirEntry->modified.month, 12)], dirEntry->modified.day,
         dirEntry->modified.hours, dirEntry->modified.minutes);
   }
   else
   {
      //The format of the date/time field is Mmm dd  yyyy
      n += osSprintf(buffer + n, " %s %02" PRIu8 "  %04" PRIu16,
         months[MIN(dirEntry->modified.month, 12)], dirEntry->modified.day,
         dirEntry->modified.year);
   }

   //Append filename
   n += osSprintf(buffer + n, " %s\r\n", dirEntry->name);

   //Return the length of the resulting string, in bytes
   return n;
}


/**
 * @brief Strip root dir from specified pathname
 * @param[in] context Pointer to the FTP server context
 * @param[in] path input pathname
 * @return Resulting pathname with root dir stripped
 **/

const char_t *ftpServerStripRootDir(FtpServerContext *context,
   const char_t *path)
{
   //Default directory
   static const char_t defaultDir[] = "/";

   //Local variables
   size_t m;
   size_t n;

   //Retrieve the length of the root directory
   n = osStrlen(context->settings.rootDir);
   //Retrieve the length of the specified pathname
   m = osStrlen(path);

   //Strip the root dir from the specified pathname
   if(n <= 1)
   {
      return path;
   }
   else if(n < m)
   {
      return path + n;
   }
   else
   {
      return defaultDir;
   }
}


/**
 * @brief Strip home directory from specified pathname
 * @param[in] connection Pointer to the client connection
 * @param[in] path input pathname
 * @return Resulting pathname with home directory stripped
 **/

const char_t *ftpServerStripHomeDir(FtpClientConnection *connection,
   const char_t *path)
{
   //Default directory
   static const char_t defaultDir[] = "/";

   //Local variables
   size_t m;
   size_t n;

   //Retrieve the length of the home directory
   n = osStrlen(connection->homeDir);
   //Retrieve the length of the specified pathname
   m = osStrlen(path);

   //Strip the home directory from the specified pathname
   if(n <= 1)
   {
      return path;
   }
   else if(n < m)
   {
      return path + n;
   }
   else
   {
      return defaultDir;
   }
}


/**
 * @brief Close client connection properly
 * @param[in] connection Pointer to the client connection to be closed
 **/

void ftpServerCloseConnection(FtpClientConnection *connection)
{
   //Close data connection
   ftpServerCloseDataChannel(connection);
   //Close control connection
   ftpServerCloseControlChannel(connection);

   //Valid file pointer?
   if(connection->file != NULL)
   {
      //Close file
      fsCloseFile(connection->file);
      connection->file = NULL;
   }

   //Valid directory pointer?
   if(connection->dir != NULL)
   {
      //Close directory
      fsCloseDir(connection->dir);
      connection->dir = NULL;
   }
}

#endif
