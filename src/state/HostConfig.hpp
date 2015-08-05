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

#ifndef _CEPHMESOS_STATE_HOSTCONFIG_HPP_
#define _CEPHMESOS_STATE_HOSTCONFIG_HPP_

#include <vector>
#include <glog/logging.h>

#include "common/Config.hpp"

using std::string;
using std::vector;

namespace ceph{

class HostConfig
{
public:
  HostConfig(){}

  HostConfig(string _hostname) :
      hostname(_hostname)
  {
    originalConfig = get_config_by_hostname(hostname);
  }

  HostConfig(Config* _config)
  {
    originalConfig = _config;
    hostname = "";
  }

  ~HostConfig()
  {
    originalConfig = NULL;
    delete originalConfig;
  }

  string toString()
  {
    string r = "";
    r = "hostname:" + hostname +
        "; mgmtdev: " + getMgmtDev() +
        "; datadev: " + getDataDev();
    string osds_str = "";
    vector<string> osds = originalConfig->osddevs;
    for (size_t i = 0; i < osds.size(); i++) {
      osds_str = osds_str + osds[i] + ",";
    }
    string jnls_str = "";
    vector<string> jnls = originalConfig->jnldevs;
    for (size_t i = 0; i < jnls.size(); i++) {
      osds_str = osds_str + jnls[i] + ",";
    }
    r = r + "; osdsdev: " + osds_str +
        " jnldev: " + jnls_str;
    return r;
  }

  void reload(){}

  string popOSDDisk()
  {
    string osddisk = "";
    vector<string>* osds = &originalConfig->osddevs;
    if (!osds->empty()) {
      osddisk = osds->back();
      osds->pop_back();
    }
    return osddisk;
  }

  string popJournalDisk()
  {
    string jnldisk = "";
    vector<string>* jnls = &originalConfig->jnldevs;
    if (!jnls->empty()) {
      jnldisk = jnls->back();
      jnls->pop_back();
    }
    return jnldisk;
   
  }

  string getMgmtDev()
  {
    return originalConfig->mgmtdev;
  }
  
  string getDataDev()
  {
    return originalConfig->datadev;
  }
  
private:

  Config* originalConfig;

  string hostname;

};

}
#endif
