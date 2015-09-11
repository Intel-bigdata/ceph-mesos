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

#ifndef _CEPHMESOS_COMMON_STRINGUTIL_HPP_
#define _CEPHMESOS_COMMON_STRINGUTIL_HPP_

#include <sstream>
#include <vector>
#include <boost/lexical_cast.hpp>

using std::vector;
using boost::lexical_cast;

class StringUtil
{
public:
  static vector<string> explode(string const& s, char delim)
  {
    std::vector<std::string> r;
    std::istringstream iss(s);
    for (string token; std::getline(iss, token, delim);) {
        r.push_back(std::move(token));
    }
    return r;
  }

  static vector<char*> explode(string const& s, char delim, int flag)
  {
    std::vector<char*> r;
    std::istringstream iss(s);
    for (string token; std::getline(iss, token, delim);) {
        char *pc = new char[token.size()+1];
        std::strcpy(pc, token.c_str());
        r.push_back(pc);
    }
    return r;
  }
  static bool matchIPToCIDR(string ip, string CIDR)
  {
    vector<string> ipTokens = explode(ip, '.');
    vector<string> tmpTokens = explode(CIDR, '/');
    string subnet = tmpTokens[0];
    vector<string> subnetTokens = explode(subnet, '.');
    int netmaskLength = lexical_cast<int>(tmpTokens[1]);
    for (int i = 0; i < netmaskLength/8; i++) {
      if (ipTokens[i] != subnetTokens[i]) {
        return false;
      }
    }
    return true;
  }
};
#endif
