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

#include <fstream>
#include <sstream>
#include <string>
#include <cerrno>

#include <common/Config.hpp>

#include <gflags/gflags.h>
#include <yaml-cpp/yaml.h>

using namespace std;

DEFINE_string(config, "/etc/cephmesos/cephmesos.yml", "The config filepath");
DEFINE_string(id, "", "Framework ID");
DEFINE_string(role, "", "Framework role");
DEFINE_string(master, "", "Mesos master uri");
DEFINE_string(zookeeper, "", "Zookeeper uri");
DEFINE_int32(restport, 0, "The REST API server port");
DEFINE_int32(fileport, 0, "The static file server port");
DEFINE_string(fileroot, "", "The static file server rootdir");

string get_file_contents(const char *filename)
{
  ifstream in(filename, ios::in | ios::binary);
  if (in)
  {
    ostringstream contents;
    contents << in.rdbuf();
    in.close();
    return(contents.str());
  }
  throw(errno);
}

Config* parse_config_string(string input)
{
  YAML::Node config = YAML::Load(input);
  Config cfg = {
      config["id"].as<string>(),
      config["role"].as<string>(),
      config["master"].as<string>(),
      config["zookeeper"].as<string>(),
      config["restport"].as<int>(),
      config["fileport"].as<int>(),
      config["fileroot"].as<string>(),
      config["mgmtdev"].as<string>(),
      config["datadev"].as<string>(),
      config["osddevs"].as<vector<string>>(),
      config["jnldevs"].as<vector<string>>(),
  };
  Config* cfg_p = new Config(cfg);
  return cfg_p;
}

Config* get_config(int* argc, char*** argv)
{
  gflags::ParseCommandLineFlags(argc, argv, true);

  char* filename = (char*)FLAGS_config.c_str();
  string input = get_file_contents(filename);
  Config* asdf = parse_config_string(input);

  Config cfg = {
      (FLAGS_id.empty() ? asdf->id : FLAGS_id),
      (FLAGS_role.empty() ? asdf->role : FLAGS_role),
      (FLAGS_master.empty() ? asdf->master : FLAGS_master),
      (FLAGS_zookeeper.empty() ? asdf->zookeeper : FLAGS_zookeeper),
      (FLAGS_restport == 0 ? asdf->restport : FLAGS_restport),
      (FLAGS_fileport == 0 ? asdf->fileport : FLAGS_fileport),
      (FLAGS_fileroot.empty() ? asdf->fileroot : FLAGS_fileroot),
      asdf->mgmtdev,
      asdf->datadev,
      asdf->osddevs,
      asdf->jnldevs,
  };
  Config* cfg_p = new Config(cfg);
  free(asdf);
  return cfg_p;
}

string get_config_path_by_hostname(string hostname)
{
  int pathIndex = FLAGS_config.find_last_of('/');
  string path = FLAGS_config.substr(0, pathIndex);
  string configPath = path + "/cephmesos.d/" + hostname + ".yml";
  return configPath;
}

Config* get_config_by_hostname(string hostname)
{
  char*  defaultConfigFile = (char*)FLAGS_config.c_str();
  string defaultContents = get_file_contents(defaultConfigFile);
  Config* defaultConfig = parse_config_string(defaultContents);
  
  string hostConfigPath = get_config_path_by_hostname(hostname);
  char*  hostConfigFile = (char*)hostConfigPath.c_str();
  string hostContents = get_file_contents(hostConfigFile);

  YAML::Node hostConfig = YAML::Load(hostContents);
  Config hostCfg = {
      (hostConfig["id"] ? hostConfig["id"].as<string>() : (FLAGS_id.empty() ? defaultConfig->id : FLAGS_id)),
      (hostConfig["role"] ? hostConfig["role"].as<string>() : (FLAGS_role.empty() ? defaultConfig->role : FLAGS_role)),
      (hostConfig["master"] ? hostConfig["master"].as<string>() : (FLAGS_master.empty() ? defaultConfig->master : FLAGS_master)),
      (hostConfig["zookeeper"] ? hostConfig["zookeeper"].as<string>() : (FLAGS_zookeeper.empty() ? defaultConfig->zookeeper : FLAGS_zookeeper)),
      (hostConfig["restport"] ? hostConfig["restport"].as<int>() : (FLAGS_restport == 0 ? defaultConfig->restport : FLAGS_restport)),
      (hostConfig["fileport"] ? hostConfig["fileport"].as<int>() : (FLAGS_fileport == 0 ? defaultConfig->fileport : FLAGS_fileport)),
      (hostConfig["fileroot"] ? hostConfig["fileroot"].as<string>() : (FLAGS_fileroot.empty() ? defaultConfig->fileroot : FLAGS_fileroot)),
      (hostConfig["mgmtdev"] ? hostConfig["mgmtdev"].as<string>() : defaultConfig->mgmtdev),
      (hostConfig["datadev"] ? hostConfig["datadev"].as<string>() : defaultConfig->datadev),
      (hostConfig["osddevs"] ? hostConfig["osddevs"].as<vector<string>>() : defaultConfig->osddevs),
      (hostConfig["jnldevs"] ? hostConfig["jnldevs"].as<vector<string>>() : defaultConfig->jnldevs),
  };
  Config* hostCfg_p = new Config(hostCfg);
  free(defaultConfig);
  return hostCfg_p;
}
