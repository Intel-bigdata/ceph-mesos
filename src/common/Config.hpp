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

#ifndef _CEPHMESOS_COMMON_CONFIG_HPP_
#define _CEPHMESOS_COMMON_CONFIG_HPP_

#include <string>
#include <vector>

const std::string hostConfigFolder = "cephmesos.d";

struct Config
{
  std::string id;
  std::string role;
  std::string master;
  std::string zookeeper;
  int restport;
  int fileport;
  std::string fileroot;
  std::string mgmtdev;
  std::string datadev;
  std::vector <std::string> osddevs;
  std::vector <std::string> jnldevs;
  int jnlparts;
};

Config* get_config(int* argc, char*** argv);
Config* get_config_by_hostname(std::string hostname);

bool is_host_config(const char *filename);
std::string get_file_contents(const char *filename);
std::string get_config_path_by_hostname(std::string hostname);
Config* parse_config_string(std::string input);
Config* merge_config(Config* defaultConfig, Config* hostConfig);

#endif
