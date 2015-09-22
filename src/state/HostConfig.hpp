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
#include <boost/lexical_cast.hpp>
#include "common/Config.hpp"

using std::string;
using std::vector;
using boost::lexical_cast;

namespace ceph{

class HostConfig
{
public:
  HostConfig(){}

  HostConfig(string _hostname) :
      hostname(_hostname)
  {
    originalConfig = get_config_by_hostname(hostname);
    pendingOSDDevs = originalConfig->osddevs;
    pendingJNLDevs = originalConfig->jnldevs;
    jnlPartsCount = originalConfig->jnlparts;
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
    for (size_t i = 0; i < pendingOSDDevs.size(); i++) {
      osds_str = osds_str + pendingOSDDevs[i] + ",";
    }
    string jnls_str = "";
    for (size_t i = 0; i < pendingJNLDevs.size(); i++) {
      osds_str = osds_str + pendingJNLDevs[i] + ",";
    }
    r = r + "; pendingosdsdev: " + osds_str +
        " pendingjnldev: " + jnls_str;
    osds_str = "";
    for (size_t i = 0; i < osdPartitions.size(); i++) {
      osds_str = osds_str + osdPartitions[i] + ",";
    }
    jnls_str = "";
    for (size_t i = 0; i < jnlPartitions.size(); i++) {
      osds_str = osds_str + jnlPartitions[i] + ",";
    }
    r = r + "; osdPartitions: " + osds_str +
        " jnlPartitions: " + jnls_str; 
    return r;
  }

  //TODO:implement this to update pending devs
  void reload()
  {
  }

  string popOSDPartition()
  {
    string p = "";
    if (!osdPartitions.empty()) {
      p = osdPartitions.back();
      osdPartitions.pop_back();
    }
    return p;
  }

  string popJournalPartition()
  {
    string p = "";
    if (!jnlPartitions.empty()) {
      p = jnlPartitions.back();
      jnlPartitions.pop_back();
    }
    return p;
   
  }

  void updateDiskPartition(vector<string> failedDevs, int jnlPartitionCount)
  {
    //osd
    for (size_t i = 0; i < pendingOSDDevs.size(); i++) {
      if (contains(failedDevs, pendingOSDDevs[i])) {
        failedOSDDevs.push_back(pendingOSDDevs[i]);
        continue;
      }
      addOSDPartition(pendingOSDDevs[i] + "1");
    }
    //jnl
    for (size_t i = 0; i < pendingJNLDevs.size(); i++) {
      if (contains(failedDevs, pendingJNLDevs[i])) {
        failedJNLDevs.push_back(pendingJNLDevs[i]);
        continue;
      }
      for (int j = 1; j < jnlPartitionCount+1; j++) {
        addJNLPartition(pendingJNLDevs[i] + lexical_cast<string>(j));
      }
    }
    pendingOSDDevs.clear();
    pendingJNLDevs.clear();
  }

  bool contains(vector<string> origin, string searchStr)
  {
    if (origin.empty()) {
      return false;
    }
    for (size_t i = 0; i < origin.size(); i++) {
      if (searchStr == origin[i]) {
        return true;
      }
    }
    return false;
  }

  void addOSDPartition(string par)
  {
    osdPartitions.push_back(par);
  }

  void addJNLPartition(string par)
  {
    jnlPartitions.push_back(par);
  }

  string getMgmtDev()
  {
    return originalConfig->mgmtdev;
  }
  
  string getDataDev()
  {
    return originalConfig->datadev;
  }

  bool havePendingOSDDevs()
  {
    return !pendingOSDDevs.empty();
  }

  bool havePendingJNLDevs()
  {
    return !pendingJNLDevs.empty();
  }

  vector<string>* getPendingOSDDevs()
  {
    return &pendingOSDDevs;
  }

  vector<string>* getPendingJNLDevs()
  {
    return &pendingJNLDevs;
  }

  bool isPreparingDisk()
  {
    return isPreparingDiskFlag;
  }

  bool haveDoneDiskPreparation()
  {
    return doneDiskPreparationFlag;
  }

  void setDiskPreparationOnGoing()
  {
    isPreparingDiskFlag = true;
    doneDiskPreparationFlag = false;
  }

  void setDiskPreparationDone()
  {
    isPreparingDiskFlag = false;
    doneDiskPreparationFlag = true;
  }

  int getJnlPartitionCount()
  {
    return jnlPartsCount;
  }

private:

  bool doneDiskPreparationFlag = false;

  bool isPreparingDiskFlag = false;

  vector<string> pendingOSDDevs;

  vector<string> pendingJNLDevs;

  int jnlPartsCount;

  vector<string> failedOSDDevs;

  vector<string> failedJNLDevs;

  vector<string> osdPartitions;

  vector<string> jnlPartitions;

  Config* originalConfig;

  string hostname;

};

}
#endif
