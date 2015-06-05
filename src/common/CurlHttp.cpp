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

#include "CurlHttp.hpp"

CurlHttp::CurlHttp() : curl(NULL){}

CurlHttp::~CurlHttp()
{
  curl_global_cleanup();
}

void CurlHttp::initialize()
{
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
}

int CurlHttp::get(const char* url, const char* filepath) const
{
  FILE *fp;
  fp = fopen(filepath, "wb");
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CurlHttp::writeCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  fclose(fp);
  return res;
}

size_t CurlHttp::writeCallback(
    void *ptr,
    size_t size,
    size_t nmemb,
    FILE *stream)
{
  size_t written = fwrite(ptr, size, nmemb, stream);
  return written;
}

