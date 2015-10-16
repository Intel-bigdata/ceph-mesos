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

#include "RestServer.hpp"

string RestServer::jsonCheck(const char* json)
{
  JsonUtil jsonRest;
  if (jsonRest.read(json)) {
    int instances = jsonRest.instances();
    if (instances<1 || instances>maxInstancesSize){
      return string("[error]:instances");
    }
    string profile = jsonRest.profile();
    if (!(profile == osd || profile == mon)) {
      return string("[error]:profile");
    }
    
    message_queue flexUpMQ
     (open_only //open or create
     ,"flexUp"  //name
     );
    flexUpMQ.send(json, strlen(json), 0);
    return "[success]";
  } else {
    return string("[error]:json");
  }
}

int RestServer::rest_request(
    void *cls,
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls)
{
  if (0 != strcmp(method, MHD_HTTP_METHOD_POST)) {
    return MHD_NO;              /* unexpected method */
  }
  if (0 != strcmp(url, "/api/cluster/flexup")) {
    return MHD_HTTP_NOT_FOUND;
  }
  struct MHD_Response *response;
  string content_type = "text/plain";
  int ret;
  string output;
  ConnectionData* connection_data = NULL;
  connection_data = static_cast<ConnectionData*>(*con_cls);
  if (NULL == connection_data) {
    connection_data = new ConnectionData();
    connection_data->is_parsing = false;
    *con_cls = connection_data;
  }
  if (!connection_data->is_parsing) {
    // First this method gets called with *upload_data_size == 0
    // just to let us know that we need to start receiving POST data
    connection_data->is_parsing = true;
    return MHD_YES;
  } else {
    if (*upload_data_size != 0) {
      // Receive the post data and write them into the bufffer
      connection_data->read_post_data << string(upload_data, *upload_data_size);
      *upload_data_size = 0;
      return MHD_YES;
    } else {
      // *upload_data_size == 0 so all data have been received
      output = "Received data:\n";
      output += connection_data->read_post_data.str();
      output += "\n";
      string outpost = connection_data->read_post_data.str();
      const char* json = outpost.c_str();
      output+=jsonCheck(json);
      delete connection_data;
      connection_data = NULL;
      *con_cls = NULL;
    }
  }

  const char* output_const = output.c_str();
  response = MHD_create_response_from_buffer(
      strlen(output_const),
      (void*)output_const,
      MHD_RESPMEM_MUST_COPY);

  MHD_add_response_header(
      response,
      MHD_HTTP_HEADER_CONTENT_TYPE,
      content_type.c_str());

  ret = MHD_queue_response(connection, MHD_HTTP_OK, response);

  MHD_destroy_response(response);

  return ret;

}

RestServer::RestServer()
{
  int port = 8889;
  daemon = MHD_start_daemon(
      MHD_USE_THREAD_PER_CONNECTION,
      port,
      NULL,
      NULL,
      &rest_request,
      NULL, MHD_OPTION_END);
}

RestServer::RestServer(int port)
{
  daemon = MHD_start_daemon(
  MHD_USE_THREAD_PER_CONNECTION,
      port,
      NULL,
      NULL,
      &rest_request,
      NULL,
      MHD_OPTION_END);
}

RestServer::~RestServer()
{
	MHD_stop_daemon(daemon);
}

bool RestServer::check()
{
  return daemon;
}


int restServer(int port = 8889)
{
  RestServer server(port);
  if (!server.check()) {
    return 1;
  }
  while (1){usleep(3*1000000);};
}
