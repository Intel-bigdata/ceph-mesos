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

#ifndef _CEPHMESOS_STATE_NODESTATE_HPP_
#define _CEPHMESOS_STATE_NODESTATE_HPP_

#include "Status.hpp"
#include "common/TaskType.hpp"

using std::string;
using ceph::Status;

namespace ceph{

class TaskState
{
public:
  TaskState(){}

  TaskState(
      string _taskId,
      string _executorId,
      TaskType _taskType,
      string _hostName,
      string _slaveId,
      Status _currentStatus) :
          taskId(_taskId),
          executorId(_executorId),
          taskType(_taskType),
          hostName(_hostName),
          slaveId(_slaveId),
          currentStatus(_currentStatus){}

  string toString()
  {
    string type;
    switch (taskType) {
      case TaskType::MON:
        type = "MON";
        break;
      case TaskType::OSD:
        type = "OSD";
        break;
      case TaskType::RADOSGW:
        type = "RADOSGW";
    }
    string status;
    switch (currentStatus) {
      case Status::STAGING:
        status = "STAGING";
        break;
      case Status::STARTING:
        status = "STARTING";
        break;
      case Status::RUNNING:
        status = "RUNNING";
        break;
      case Status::FAILED:
        status = "FAILED";
        break;
      case Status::FINISHED:
        status = "FINISHED";
        break;
      case Status::UNKNOWN:
        status = "UNKNOWN";
        break;
    }
    string r = "taskId: " + taskId
        + ", executorId: " + executorId
        + ", taskType: " + type
        + ", hostName: " + hostName
        + ", slaveId: " + slaveId
        + ", currentStatus: " + status;
    return r;
  }
  ~TaskState(){}

  string taskId;

  string executorId;

  TaskType taskType;

  string hostName;

  string slaveId;

  Status currentStatus;

};

}//end of namespace ceph
#endif
