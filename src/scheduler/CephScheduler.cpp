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
  config = _config;
  stateMachine = new StateMachine(_config);
  eventLoop = new EventLoop();
}


CephScheduler::~CephScheduler()
{
  stateMachine = NULL;
  eventLoop = NULL;
  delete stateMachine;
  delete eventLoop;
}

void CephScheduler::registered(
      SchedulerDriver* driver,
      const FrameworkID& frameworkId,
      const MasterInfo& masterInfo)
{
  LOG(INFO) << "Registered! FrameworkID=" << frameworkId.value();
}

void CephScheduler::statusUpdate(
      SchedulerDriver* driver,
      const TaskStatus& status)
{
  LOG(INFO) << "Got status update from " << status.source();
  string taskId = status.task_id().value();
  if (status.state() == TASK_RUNNING) {
    LOG(INFO) << taskId << " is Running!";
    stateMachine->updateTaskToRunning(taskId);
    if (status.has_message()){
      vector<string> tokens = StringUtil::explode(status.message(), '.');
      if ((MessageToScheduler)lexical_cast<int>(tokens[0])
          == MessageToScheduler::CONSUMED_OSD_ID
          ){
        string consumedOSDId = tokens[1];
        LOG(INFO) << "Got message of \"consumed_OSD_ID\": "<<consumedOSDId;

      }
    }
  } else if (status.state() == TASK_STARTING) {
    LOG(INFO) << taskId << " is Waiting OSDID, ready for assign osd id!";
    stateMachine->updateTaskToWaitingOSDID(taskId);
  } else if (status.state() == TASK_FAILED) {
    LOG(INFO) << taskId << " failed";
    stateMachine->updateTaskToFailed(taskId);
    //TODO: if has message , add the OSD ID back to StateMachine
  }
}

void CephScheduler::frameworkMessage(
      SchedulerDriver* driver,
      const ExecutorID& executorId,
      const SlaveID& slaveId,
      const string& data)
{
  ceph::TaskState initialMon = stateMachine->getInitialMon();
  if (initialMon.executorId == executorId.value()) {
    LOG(INFO) << "Got osd id from inital MON: " << data;
    stateMachine->addPendingOSDID(lexical_cast<int>(data));
  }
}

void CephScheduler::resourceOffers(
      SchedulerDriver* driver,
      const vector<Offer>& offers)
{
  LOG(INFO) << "Received " << offers.size() << " offers! ";
  TaskType taskType;
  int token;
  int isInitialMonNode = 0;
  //handle waiting OSD task, give them osdID to start docker
  handleWaitingOSDTasks(driver);
  Phase currentPhase = stateMachine->getCurrentPhase();
  //try start new node
  foreach (const Offer& offer, offers) {
    bool accept = stateMachine->nextMove(taskType,token,offer.hostname());
    if (!accept) {
      LOG(INFO) << "In the "
          << static_cast<int>(currentPhase)
          << " Staging Phase, cannot accept offer from "
          << offer.hostname()
          << " in this phase";
      driver->declineOffer(offer.id());
      continue;
    }
    LOG(INFO) << "Check offer's resources from " <<offer.hostname();
    if (offerNotEnoughResources(offer,taskType)) {
      LOG(INFO) << "Not enough, decline it from " << offer.hostname();
      driver->declineOffer(offer.id());
      continue;
    }
    if (currentPhase == Phase::WAINTING_REQUEST){
      accept = fetchPendingRESTfulRequest();
      if (!accept){
        LOG(INFO) << "No pending OSD RESTful request.";
        driver->declineOffer(offer.id());
        stateMachine->decreaseOSDIndex();
        continue;
      }
    }

    LOG(INFO) << "Accepted offer from" << offer.hostname() << ", launch "
        << static_cast<int>(taskType) <<":" << token << " node";
    if (taskType == TaskType::MON && token == 0) {
        LOG(INFO) << "This is the initial MON";
        isInitialMonNode = 1;
    }
    string taskId;
    string executorId;
    launchNode(
        driver,
        offer,
        taskType,
        token,
        isInitialMonNode,
        taskId,
        executorId);
    stateMachine->addStagingTask(
        taskId,
        executorId,
        taskType,
        offer.hostname(),
        offer.slave_id().value());
    if (!isInitialMonNode && taskType == TaskType::OSD) {
      ceph::TaskState initialMon = stateMachine->getInitialMon();
      const string m = lexical_cast<string>(static_cast<int>(MessageToExecutor::REGISTER_OSD));
      ExecutorID eId;
      eId.set_value(initialMon.executorId);
      SlaveID sId;
      sId.set_value(initialMon.slaveId);
      driver->sendFrameworkMessage(
          eId,
          sId,
          m);
    }//end if

  }//end foreach
}

string CephScheduler::getFileServerIP()
{
  return NetUtil::getLocalIP();
}

// token is used to distinguish same executorName with same taskType
// should be maintained by StateMachine
// TODO: add resources limit as a param
string CephScheduler::createExecutor(
      string executorName,
      TaskType taskType,
      int token,
      int isInitialMonNode,
      ExecutorInfo& executor)
{
  string name = executorName + "." +
      lexical_cast<string>(static_cast<int>(taskType)) + "." +
      lexical_cast<string>(token);
  time_t timestamp;
  time(&timestamp);
  string id = name + "." + lexical_cast<string>(timestamp);
  string binary_uri = "http://"+ getFileServerIP() + ":" +
      lexical_cast<string>(config->fileport) +
      "/" + executorName;
  executor.mutable_executor_id()->set_value(id);
  CommandInfo::URI* uri = executor.mutable_command()->add_uris();
  uri->set_value(binary_uri);
  uri->set_executable(true);
  //config files:
  //ceph.client.admin.keyring
  //ceph.conf
  //ceph.mon.keyring
  //monmap
  if (!isInitialMonNode) {
    ceph::TaskState initialMon = stateMachine->getInitialMon();
    string prefix = "http://" + initialMon.hostName + ":7777/";
    string configUriClientKeyring = prefix + "ceph.client.admin.keyring";
    string configUriCephConf = prefix + "ceph.conf";
    string configUriMonKeyring = prefix + "ceph.mon.keyring";
    string configMonMap = prefix + "monmap";
    uri = executor.mutable_command()->add_uris();
    uri->set_value(configUriClientKeyring);
    uri = executor.mutable_command()->add_uris();
    uri->set_value(configUriCephConf);
    uri = executor.mutable_command()->add_uris();
    uri->set_value(configUriMonKeyring);
    uri = executor.mutable_command()->add_uris();
    uri->set_value(configMonMap);
  }
  executor.mutable_command()->set_value(
      "./" + executorName);

  executor.set_name(name);
  return id;
}

string CephScheduler::createTaskName(TaskType taskType, int token)
{
  string prefix;
  switch (taskType) {
    case TaskType::MON:
      prefix = "mon";
      break;
    case TaskType::OSD:
      prefix = "osd";
      break;
    case TaskType::RADOSGW:
      prefix = "radosgw";
      break;
  }
  return prefix + lexical_cast<string>(token);
}
//assign one OSD ID to one OSD task
//TODO: we can give one OSD tasks multiple OSDID to supprot
//runnig multiple OSD in one container. But if so, current task id and name
//osd<id>.<hostname>.<slaveid> maybe confusing. Need to change too
void CephScheduler::handleWaitingOSDTasks(SchedulerDriver* driver)
{
  vector<int>* OSDIDs = stateMachine->getPendingOSDID();
  vector<ceph::TaskState> OSDTasks = stateMachine->getWaitingOSDTask();
  ExecutorID eId;
  SlaveID sId;
  string m;
  ceph::TaskState taskState;
  for(size_t i = 0; i < OSDTasks.size(); i++) {
    taskState = OSDTasks[i];
    if (!OSDIDs->empty()){
      int Id = OSDIDs->back();
      //OSD executor will send status with message containing OSD
      OSDIDs->pop_back();
      m = lexical_cast<string>(static_cast<int>(MessageToExecutor::LAUNCH_OSD)) +
          "." + lexical_cast<string>(Id);
      eId.set_value(taskState.executorId);
      sId.set_value(taskState.slaveId);
      driver->sendFrameworkMessage(
          eId,
          sId,
          m);
      LOG(INFO) << "Assign: OSD ID " << Id << " to this task: ";
      LOG(INFO) << taskState.toString();
      //if set the task to Starting, scheduler will know 
      //it has assigned OSDID to it
      stateMachine->updateTaskToStarting(taskState.taskId);
    } else {
      LOG(INFO) << "No OSD ID available!";
      break;
    }
  }//end foreach

}

//currently only support OSD REST request
bool CephScheduler::fetchPendingRESTfulRequest()
{
  //fetch a request gets from REST server
  LOG(INFO) << "FetchPending requests..";
  vector<int>* OSDRequests = eventLoop->getPendingOSD();
  if (OSDRequests->empty()){
    return false;
  } else {
    LOG(INFO) << "Total received " << OSDRequests->size() << " requests!";
    int OSDInstanceCount = (*OSDRequests)[0];
    OSDInstanceCount--;
    if (0 == OSDInstanceCount){
      LOG(INFO) << "InstanceCount is 0, remove this request";
      OSDRequests->erase(OSDRequests->begin());
    } else {
      (*OSDRequests)[0] = OSDInstanceCount;
    }
    LOG(INFO) << "Consumed 1 OSD instance of first request,"
        << "remaining " << OSDInstanceCount << " instance(s)";
    return true;
  }
}

void CephScheduler::launchNode(
      SchedulerDriver* driver,
      const Offer& offer,
      TaskType taskType,
      int token,
      int isInitialMonNode,
      string& taskId,
      string& executorId)
{

  vector<TaskInfo> tasks;

  ExecutorInfo executor;
  executorId = createExecutor(
      "ceph-mesos-executor",
      taskType,
      token,
      isInitialMonNode,
      executor);

  TaskInfo task;
  task.set_name(createTaskName(taskType,token));
  //set this to indicate executor whether to download shared config
  task.set_data(
      lexical_cast<string>(isInitialMonNode) +
      "." +
      lexical_cast<string>(static_cast<int>(taskType)));
  taskId = task.name() + "."
      + offer.hostname() + "."
      + offer.slave_id().value();
  task.mutable_task_id()->set_value(taskId);
  task.mutable_slave_id()->MergeFrom(offer.slave_id());

  task.mutable_executor()->MergeFrom(executor);

  Resource* resource;
  resource = task.add_resources();
  resource->set_name("cpus");
  resource->set_type(Value::SCALAR);
  //TODO: use resource limit in config, like "mesos.ceph.mon.cpus"
  resource->mutable_scalar()->set_value(1);

  resource = task.add_resources();
  resource->set_name("mem");
  resource->set_type(Value::SCALAR);
  //TODO: use resource limit in config, like "mesos.ceph.mon.mem"
  resource->mutable_scalar()->set_value(1024);

  tasks.push_back(task);

  driver->launchTasks(offer.id(), tasks);
}

bool CephScheduler::offerNotEnoughResources(const Offer& offer, TaskType taskType)
{
  //TODO: check cpus and mems based on taskType
  int cpus = 1;
  int mems = 1024;
  static const Resources neededResources = Resources::parse(
      "cpus:" + stringify(cpus) +
      ";mem:" + stringify(mems)).get();
  Resources res = offer.resources();
  if (res.flatten().contains(neededResources)) {
    return false;
  } else {
    return true;
  }
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
