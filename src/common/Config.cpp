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

Config* get_config(int* argc, char*** argv)
{
  gflags::ParseCommandLineFlags(argc, argv, true);

  char* filename = (char*)FLAGS_config.c_str();
  string input = get_file_contents(filename);
  Config* config = parse_config_string(input);
  Config cfg = {
      (FLAGS_id.empty() ? config->id : FLAGS_id),
      (FLAGS_role.empty() ? config->role : FLAGS_role),
      (FLAGS_master.empty() ? config->master : FLAGS_master),
      (FLAGS_zookeeper.empty() ? config->zookeeper : FLAGS_zookeeper),
      (FLAGS_restport == 0 ? config->restport : FLAGS_restport),
      (FLAGS_fileport == 0 ? config->fileport : FLAGS_fileport),
      (FLAGS_fileroot.empty() ? config->fileroot : FLAGS_fileroot),
      config->mgmtdev,
      config->datadev,
      config->osddevs,
      config->jnldevs,
  };
  Config* cfg_p = new Config(cfg);
  free(config);
  return cfg_p;
}

Config* get_config_by_hostname(string hostname)
{
  char*  defaultConfigFile = (char*)FLAGS_config.c_str();
  string defaultContents = get_file_contents(defaultConfigFile);
  Config* defaultConfig = parse_config_string(defaultContents);

  string hostConfigPath = get_config_path_by_hostname(hostname);
  char*  hostConfigFile = (char*)hostConfigPath.c_str();
  string hostContents = get_file_contents(hostConfigFile);
  Config* hostConfig = parse_config_string(hostContents);

  Config* hostCfg_p = merge_config(defaultConfig, hostConfig);
  return hostCfg_p;
}

bool is_host_config(const char *filename)
{
  string file = (filename);
  return (file.find("cephmesos.d") != file.npos) ? true : false;
}

string get_file_contents(const char *filename)
{
  ifstream in(filename, ios::in | ios::binary);
  if (in){
      ostringstream contents;
      contents << in.rdbuf();
      in.close();
      return(contents.str());
  }
  else{
      if (!is_host_config(filename)){
          throw(errno);
      }
      else{
          return "";
      }
  }
}

string get_config_path_by_hostname(string hostname)
{
  string path;
  int pathIndex = FLAGS_config.find_last_of('/');
  
  if (pathIndex != -1)
  {
      path = FLAGS_config.substr(0, pathIndex) + "/";
  }
  string configPath = path + "cephmesos.d/" + hostname + ".yml";
  return configPath;
}

Config* parse_config_string(string input)
{
  YAML::Node config = YAML::Load(input);
  string empty_s;
  vector<string> empty_v;
  Config cfg = {
      (config["id"] ? config["id"].as<string>() : FLAGS_id),
      (config["role"] ? config["role"].as<string>() : FLAGS_role),
      (config["master"] ? config["master"].as<string>() : FLAGS_master),
      (config["zookeeper"] ? config["zookeeper"].as<string>() : FLAGS_zookeeper),
      (config["restport"] ? config["restport"].as<int>() : FLAGS_restport),
      (config["fileport"] ? config["fileport"].as<int>() : FLAGS_fileport),
      (config["fileroot"] ? config["fileroot"].as<string>() : FLAGS_fileroot),
      (config["mgmtdev"] ? config["mgmtdev"].as<string>() : empty_s),
      (config["datadev"] ? config["datadev"].as<string>() : empty_s),
      (config["osddevs"] ? config["osddevs"].as<vector<string>>() : empty_v),
      (config["jnldevs"] ? config["jnldevs"].as<vector<string>>() : empty_v),
  };
  Config* cfg_p = new Config(cfg);
  return cfg_p;
}

Config* merge_config(Config* defaultConfig, Config* hostConfig)
{
  Config config = {
      FLAGS_id,
      FLAGS_role,
      FLAGS_master,
      FLAGS_zookeeper,
      FLAGS_restport,
      FLAGS_fileport,
      FLAGS_fileroot,
      (hostConfig->mgmtdev.empty() ? defaultConfig->mgmtdev : hostConfig->mgmtdev),
      (hostConfig->datadev.empty() ? defaultConfig->datadev : hostConfig->datadev),
      (hostConfig->osddevs.empty() ? defaultConfig->osddevs : hostConfig->osddevs),
      (hostConfig->jnldevs.empty() ? defaultConfig->jnldevs : hostConfig->jnldevs),
  };
  Config* config_p = new Config(config);
  free(defaultConfig);
  free(hostConfig);
  return  config_p;
}
