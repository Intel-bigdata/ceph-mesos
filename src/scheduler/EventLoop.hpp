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

#ifndef _CEPHMESOS_SCHEDULER_EVENTLOOP_HPP_
#define _CEPHMESOS_SCHEDULER_EVENTLOOP_HPP_

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <exception>
#include <thread>
#include <glog/logging.h>
#include "common/JsonUtil.hpp"
#include <boost/interprocess/ipc/message_queue.hpp>

using std::string;
using std::vector;
using std::thread;
using std::cout;
using std::endl;
using std::istringstream;
using std::exception;
using namespace boost::interprocess;

class EventLoop
{
public:
  EventLoop();
  ~EventLoop();
  vector<int>* getPendingOSD() { return &pendingOSD; }

private:
  void processData(string data);

  bool loopTag;
  vector<int> pendingOSD;
  void recvData();
  thread *recvThread;
};
#endif
