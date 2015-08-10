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

#include "CephScheduler.hpp"

#include <time.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "common/TaskType.hpp"
#include "common/FrameworkMessage.hpp"
#include "common/StringUtil.hpp"
#include "common/NetUtil.hpp"
#include "state/Phase.hpp"
#include "state/TaskState.hpp"

using boost::lexical_cast;
using ceph::Phase;

void CephScheduler::reregistered(
      SchedulerDriver* driver,
      const MasterInfo& masterInfo)
{
}

void CephScheduler::disconnected(SchedulerDriver* driver){}

void CephScheduler::offerRescinded(
      SchedulerDriver* driver,
      const OfferID& offerId)
{
}

void CephScheduler::slaveLost(
      SchedulerDriver* driver,
      const SlaveID& slaveId)
{
}

void CephScheduler::executorLost(
      SchedulerDriver* driver,
      const ExecutorID& executorId,
      const SlaveID& slaveId,
      int status)
{
}

void CephScheduler::error(
      SchedulerDriver* driver,
      const std::string& message)
{
}

CephScheduler::CephScheduler(Config* _config)
{
  agent = new CephSchedulerAgent(_config);
}


CephScheduler::~CephScheduler()
{
  agent = NULL;
  delete agent;
}

void CephScheduler::registered(
      SchedulerDriver* driver,
      const FrameworkID& frameworkId,
      const MasterInfo& masterInfo)
{
  agent->registered(driver,frameworkId,masterInfo);
}

void CephScheduler::statusUpdate(
      SchedulerDriver* driver,
      const TaskStatus& status)
{
  agent->statusUpdate(driver,status);
}

void CephScheduler::frameworkMessage(
      SchedulerDriver* driver,
      const ExecutorID& executorId,
      const SlaveID& slaveId,
      const string& data)
{
  agent->frameworkMessage(driver,executorId,slaveId,data);
}

void CephScheduler::resourceOffers(
      SchedulerDriver* driver,
      const vector<Offer>& offers)
{
  agent->resourceOffers(driver,offers);
}

//int main(int argc, char** argv)
int framework(Config* config)
{
  FrameworkInfo framework;

  framework.set_user("root");
  framework.set_name(config->id);
  framework.set_role(config->role);
  framework.set_checkpoint(true);
  string master = config->master;

  MesosSchedulerDriver* driver;
  CephScheduler scheduler(config);

  driver = new MesosSchedulerDriver(&scheduler,framework,master);

  int status = driver->run() == DRIVER_STOPPED ? 0 : 1;
  driver->stop();
  delete driver;
  return status;

}
