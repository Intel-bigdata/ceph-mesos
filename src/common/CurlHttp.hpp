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

#include <curl/curl.h>
#include <string>

class CurlHttp
{
public:
  CurlHttp();

  ~CurlHttp();

  void initialize();

  int get(const char* url, const char* filepath) const;

  static size_t writeCallback(
      void *ptr,
      size_t size,
      size_t nmemb,
      FILE *stream);

private:
  CURL* curl;
};

#endif
