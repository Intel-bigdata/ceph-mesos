/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "FileServer.hpp"

static std::string path;

static ssize_t file_reader(void *cls, uint64_t pos, char *buf, size_t max)
{
  FILE *file = (FILE*)cls;

  (void)fseek(file, pos, SEEK_SET);
  return fread(buf, 1, max, file);
}

static void free_callback(void *cls)
{
  FILE *file = (FILE*)cls;
  fclose(file);
}

int file_request(
    void *cls,
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls)
{
  static int aptr;
  struct MHD_Response *response;
  int ret;
  FILE *file;
  struct stat buf;

  if (0 != strcmp(method, MHD_HTTP_METHOD_GET)) {
    return MHD_NO;              /* unexpected method */
  }
  if (&aptr != *con_cls) {
    /* do never respond on first call */
    *con_cls = &aptr;
    return MHD_YES;
  }
  *con_cls = NULL;                  /* reset when done */

  std::string surl(&url[1]);
  const char* filepath = (path + surl).c_str();

  if (0 == stat(filepath, &buf)) {
    file = fopen(filepath, "rb");
  } else {
    file = NULL;
  }
  if (file == NULL) {
    response = MHD_create_response_from_buffer(strlen(filepath),
        (void *)filepath,
        MHD_RESPMEM_PERSISTENT);
    ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
  } else {
    /* 32k page size */
    response = MHD_create_response_from_callback(
        buf.st_size,
        32 * 1024,
        &file_reader,
        file,
        &free_callback);
    if (response == NULL) {
      fclose(file);
      return MHD_NO;
    }
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
  }
  return ret;
}

int fileServer(const int port, const std::string & _path )
{
  path = _path;
  MHD_Daemon* daemon = MHD_start_daemon(
      MHD_USE_THREAD_PER_CONNECTION, port, NULL, NULL,
      &file_request, NULL, MHD_OPTION_END);
  if (!daemon) {
    return 1;
  }
  while (1);
}
