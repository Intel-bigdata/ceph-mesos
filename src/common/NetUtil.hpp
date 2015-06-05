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

#ifndef _CEPHMESOS_COMMON_NETUTIL_HPP_
#define _CEPHMESOS_COMMON_NETUTIL_HPP_

#include <netdb.h>
#include <arpa/inet.h>

class NetUtil
{
public:
  static string getLocalIP()
  {
    char hostname[1024];
    int r = gethostname(hostname, sizeof(hostname));
    if (0 != r){
      LOG(ERROR) << "Cannot get host's local IP, quit";
    }
    hostent * record = gethostbyname(hostname);
    in_addr * address = (in_addr * )record->h_addr;
    string ip = inet_ntoa(* address);
    return ip;
  }

  static string getIPByHostname(string hostname)
  {
    hostent * record = gethostbyname(hostname.c_str());
    in_addr * address = (in_addr * )record->h_addr;
    string ip = inet_ntoa(* address);
    return ip;
  }

};
#endif
