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

#include <common/Config.hpp>

#include <gflags/gflags.h>
#include <yaml-cpp/yaml.h>

using namespace std;

DEFINE_string(config, "/etc/ceph-mesos/ceph-mesos.yml", "The config filepath");
DEFINE_string(id, "", "Framework ID");
DEFINE_string(role, "", "Framework role");
DEFINE_string(master, "", "Mesos master uri");
DEFINE_string(zookeeper, "", "Zookeeper uri");
DEFINE_int32(restport, 0, "The REST API server port");
DEFINE_int32(fileport, 0, "The static file server port");
DEFINE_string(fileroot, "", "The static file server rootdir");

Config* get_config(int* argc, char*** argv)
{
  gflags::ParseCommandLineFlags(argc, argv, true);
  YAML::Node config = YAML::LoadFile(FLAGS_config);

  Config cfg = {
      (FLAGS_id.empty() ? config["id"].as<string>() : FLAGS_id),
      (FLAGS_role.empty() ? config["role"].as<string>() : FLAGS_role),
      (FLAGS_master.empty() ? config["master"].as<string>() : FLAGS_master),
      (FLAGS_zookeeper.empty() ? config["zookeeper"].as<string>() :
                                 FLAGS_zookeeper),
      (FLAGS_restport == 0 ? config["restport"].as<int>() : FLAGS_restport),
      (FLAGS_fileport == 0 ? config["fileport"].as<int>() : FLAGS_fileport),
      (FLAGS_fileroot.empty() ? config["fileroot"].as<string>() :
                                FLAGS_fileroot),
  };
  Config* cfg_p = new Config(cfg);
  return cfg_p;
}
