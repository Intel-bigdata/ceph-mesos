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

#include <thread>
#include <zmq.hpp>

#include "httpserver/FileServer.hpp"
#include "httpserver/RestServer.hpp"
#include "scheduler/CephScheduler.hpp"
#include "common/Config.hpp"

zmq::context_t context(1);

int main(int argc, char* argv[]) {
    Config* config;
    try{
       config = get_config(&argc, &argv);
    }
    catch(char const* strException){
      LOG(ERROR) <<"[Error Quit]: "<<strException<<endl;
      return 0;
    }
    thread fileServerThread(fileServer,config->fileport,config->fileroot);
    thread restServerThread(restServer,config->restport);
    thread frameworkThread(framework, config);
    fileServerThread.join();
    restServerThread.join();
    frameworkThread.join();
    free(config);
    return 0;
}
