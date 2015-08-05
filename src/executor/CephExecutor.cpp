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

#include "CephExecutor.hpp"

#include <sys/wait.h>
#include <glog/logging.h>
#include <boost/lexical_cast.hpp>
#include <thread>
#include <time.h>

#include "common/FrameworkMessage.hpp"
#include "common/StringUtil.hpp"
#include "common/NetUtil.hpp"
#include "common/TaskType.hpp"
#include "httpserver/FileServer.hpp"

using ceph::TaskType;
using boost::lexical_cast;
using std::thread;

void CephExecutor::reregistered(
      ExecutorDriver* driver,
      const SlaveInfo& slaveInfo)
{
  LOG(INFO) << "reregisted";
}

void CephExecutor::disconnected(ExecutorDriver* driver){}

void CephExecutor::killTask(ExecutorDriver* driver, const TaskID& taskId){}

void CephExecutor::error(ExecutorDriver* driver, const string& message){}

void CephExecutor::deleteConfigDir(string localSharedConfigDir)
{
  string cmd = "rm -rf " +
      localSharedConfigDirRoot + "/" + localSharedConfigDir;
  string r = runShellCommand(cmd);
  LOG(INFO) << "Delete config dir with command: " << cmd;
}

bool CephExecutor::existsConfigFiles(string localSharedConfigDir)
{
  string cmd;
  string r;
  string pre = localSharedConfigDirRoot +
      "/" + localSharedConfigDir + "/etc/ceph/";
  string adminkeyring = pre + "ceph.client.admin.keyring";
  string cephconf = pre + "ceph.conf";
  string monkeyring = pre + "ceph.mon.keyring";
  string monmap = pre + "monmap";
  cmd = "[ -f "+ adminkeyring + " ]" + " && " +
      "[ -f "+ cephconf + " ]" + " && " +
      "[ -f "+ monkeyring + " ]" + " && " +
      "[ -f "+ monmap + " ]" +
      ";echo $?";
  r = runShellCommand(cmd);
  //already have the shared config
  if ( "0" == r ) {
    return true;
  }
  return false;
}

bool CephExecutor::createLocalSharedConfigDir(string localSharedConfigDir)
{
  if (existsConfigFiles(localSharedConfigDir)) {
    LOG(INFO) << "Ceph config files already exists";
    return true;
  }
  string cmd = "cd " + localSharedConfigDirRoot + " && " +
      "mkdir " + localSharedConfigDir + " && " +
      "cd " + localSharedConfigDir + " && " +
      "mkdir -p ./etc/ceph && " +
      "mkdir -p ./var/lib/ceph/bootstrap-mds && " +
      "mkdir -p ./var/lib/ceph/bootstrap-osd && " +
      "mkdir -p ./var/lib/ceph/mds && " +
      "mkdir -p ./var/lib/ceph/osd && " +
      "mkdir -p ./var/lib/ceph/mon && " +
      "mkdir -p ./var/lib/ceph/radosgw && " +
      "mkdir -p ./var/lib/ceph/tmp && " +
      "cd .. && chmod 777 -R " + localSharedConfigDir + " && " +
      "echo $?";
  LOG(INFO) << "Using this command to create local shared config directory:";
  LOG(INFO) << cmd;
  string r = runShellCommand(cmd);
  if ("0" == r) {
    return true;
  } else {
    return false;
  }
}

bool CephExecutor::copyWaitingNICEntryPoint(string localSharedConfigDir)
{
  string cmd = "[ -f " + localSharedConfigDir + "/wfn/wfn.sh ];" +
      "echo $?";
  string r = runShellCommand(cmd);
  if ("0" == r){
    LOG(INFO) << "wfn.sh already exists";
    return true;
  }
  cmd = "cd " + localSharedConfigDir + " && " +
      "mkdir wfn" + " && "
      "cd " + sandboxAbsolutePath + " && " +
      "cp ./wfn.sh " + localSharedConfigDir + "/wfn/" +
      " && chmod 777 -R " + localSharedConfigDir + "/wfn"
      ";echo $?";
  r = runShellCommand(cmd);
  if ("0" == r){
    return true;
  } else {
    return false;
  }
}

bool CephExecutor::copySharedConfigFiles(string localSharedConfigDir)
{
  string cmd = "cd " + sandboxAbsolutePath + " && " +
      "cp ./ceph.client.admin.keyring " +
      "./ceph.conf " +
      "./ceph.mon.keyring " +
      "./monmap " + localSharedConfigDir +
      ";echo $?";
  string r = runShellCommand(cmd);
  if ("0" == r){
    return true;
  } else {
    return false;
  }
}

string CephExecutor::constructMonCommand(
      string localMountDir,
      string _containerName)
{
  string cmd;
  string imagename = "ceph/mon";
  string local_wfn_dir = localMountDir + "/wfn";
  string local_config_dir = localMountDir + "/etc/ceph";
  string local_monfs_dir = localMountDir + "/var/lib/ceph";
  string docker_start_cmd = "docker run --rm --net=none --privileged --name " +
      _containerName;
  string docker_env_param = " -e MON_IP_AUTO_DETECT=4";
  string docker_new_entrypoint = " --entrypoint=/root/wfn.sh";
  //TODO: mount /etc/localtime means sync time with localhost, need at least Docker v1.6.2
  string docker_volume_param = " -v " + local_config_dir +
      ":/etc/ceph:rw" + " -v " + local_monfs_dir +
      ":/var/lib/ceph:rw" + " -v " + local_wfn_dir + ":/root ";
  string docker_new_entrypoint_param = " -e=/entrypoint.sh -n=eth-"+_containerName;
  string docker_command;
  docker_command.append(docker_start_cmd);
  docker_command.append(docker_env_param);
  docker_command.append(docker_new_entrypoint);
  docker_command.append(docker_volume_param);
  docker_command.append(imagename);
  docker_command.append(docker_new_entrypoint_param);
  return docker_command;
}

string CephExecutor::registerOSD()
{
  string cmd = "docker exec " + containerName +" ceph osd create";
  string osdID = runShellCommand(cmd);
  return osdID;
}

string CephExecutor::constructOSDCommand(
      string localMountDir,
      string osdId,
      string _containerName)
{
  string cmd;
  string imagename = "ceph/osd";
  string local_wfn_dir = localMountDir + "/wfn";
  string local_config_dir = localMountDir + "/etc/ceph";
  string local_fs_dir = localMountDir + "/var/lib/ceph";
  string osdid_dir_name = "ceph-" + osdId;
  string docker_start_cmd = "docker run --rm --net=none --privileged --name " +
      _containerName;
  string docker_env_param = " -e HOSTNAME=" + myHostname;
  string docker_new_entrypoint = " --entrypoint=/root/wfn.sh";
  //TODO: mount /etc/localtime means sync time with localhost, need at least Docker v1.6.2
  string docker_volume_param = " -v " + local_config_dir +
      ":/etc/ceph:rw" + " -v " + local_fs_dir + "/osd/" + osdid_dir_name +
      ":/var/lib/ceph/osd/" + osdid_dir_name + ":rw" +
      + " -v " + local_wfn_dir + ":/root ";
  string docker_new_entrypoint_param = " -e=/sbin/my_init -n=eth-"+_containerName;
  string docker_command;
  docker_command.append(docker_start_cmd);
  docker_command.append(docker_env_param);
  docker_command.append(docker_new_entrypoint);
  docker_command.append(docker_volume_param);
  docker_command.append(imagename);
  docker_command.append(docker_new_entrypoint_param);
  return docker_command;
}

string CephExecutor::constructRADOSGWCommand(
    string localMountDir,
    string _containerName)
{
  string imagename = "ceph/radosgw";
  string local_wfn_dir = localMountDir + "/wfn";
  string local_config_dir = localMountDir + "/etc/ceph";
  string docker_start_cmd = "docker run --rm --net=none --privileged --name " +
      _containerName;
  string docker_env_param = " -e RGW_NAME=" + _containerName;
  string docker_new_entrypoint = " --entrypoint=/root/wfn.sh";
  //TODO: mount /etc/localtime means sync time with localhost, need at least Docker v1.6.2
  string docker_volume_param = " -v " + local_config_dir +
      ":/etc/ceph:rw" + " -v " + local_wfn_dir + ":/root ";
  string docker_new_entrypoint_param = " -e=/entrypoint.sh -n=eth-" +
      _containerName;
  string docker_command;
  docker_command.append(docker_start_cmd);
  docker_command.append(docker_env_param);
  docker_command.append(docker_new_entrypoint);
  docker_command.append(docker_volume_param);
  docker_command.append(imagename);
  docker_command.append(docker_new_entrypoint_param);
  return docker_command;

}

bool CephExecutor::block_until_started(string _containerName, string timeout)
{
  time_t current = time(0);
  time_t deadline = current + lexical_cast<int>(timeout);
  //poll the result every 3 seconds until timeout
  string cmd;
  string r;
  while (current < deadline) {
    cmd = "docker ps |grep " + _containerName + " > /dev/null ;echo $?";
    r = runShellCommand(cmd);
    usleep(3*1000000);
    if ("0" == r) {
      break;
    }
  }
  if ("0" != r) {
    LOG(INFO) << "Docker container failed to start!";
    return false;
  }
  cmd = "docker exec " + _containerName +
      " timeout "+ timeout + " ceph mon stat > /dev/null 2>&1; echo $?";
  r = runShellCommand(cmd);
  if ("0" != r){
    LOG(INFO) << "Container's ceph daemon failed to start! " << r;
    return false;
  }
  LOG(INFO) << "error code: " << r;
  return true;
}
//TODO: support multiple nic creation
bool CephExecutor::createNICs(string _containerName, string physicalNIC, string containerNIC)
{
  time_t current = time(0);
  time_t deadline = current + lexical_cast<int>(30);
  //poll the result every 3 seconds until timeout
  string cmd;
  string r;
  while (current < deadline) {
    cmd = "docker ps |grep " + _containerName + " > /dev/null ;echo $?";
    r = runShellCommand(cmd);
    usleep(2*1000000);
    if ("0" == r) {
      break;
    }
  }
  if ("0" != r) {
    LOG(INFO) << "Docker container failed to start!";
    return false;
  }
  LOG(INFO) << "start creating " + containerNIC + "for " + _containerName;
  string tmpNIC = "mvb-" + _containerName;
  cmd = "docker inspect --format=\"{{.State.Pid}}\" " +
      _containerName;
  string container_pid = runShellCommand(cmd);
  LOG(INFO) << "container_pid:" << container_pid;
  cmd = "ip link add link " + physicalNIC +
      " name "+ tmpNIC + " type macvlan mode bridge" + ";" +
      "ip link set netns " + container_pid + " " + tmpNIC + ";" +
      "sleep 3" + ";" +
      "nsenter -t " + container_pid + " -n ip link set " + tmpNIC + " name " +
      containerNIC;
  LOG(INFO) << cmd;
  r = runShellCommand(cmd);
  LOG(INFO)<<r;
  int max_retry = 200;
  int has_tried = 0;
  while (has_tried < max_retry) {
    cmd = "nsenter -t " + container_pid + " -n dhclient " + containerNIC +
        ";echo $?";
    LOG(INFO) << cmd;
    r= runShellCommand(cmd);
    LOG(INFO) << r;
    if ("0"== r) {
      break;
    }
    has_tried++;
    sleep(2);
    LOG(INFO) << "retry dhclient..(" << has_tried << ")";
  }
  if (has_tried >= max_retry) {
    return false;
  }
  cmd = "dhclient -r " + containerNIC;
  LOG(INFO) << cmd;
  r = runShellCommand(cmd);
  LOG(INFO)<<r;
  //TODO: check this shell execution success
  LOG(INFO) << "Create NIC done.";
  return true;
}

void CephExecutor::startLongRunning(string binaryName, string cmd)
{
  if ("" != cmd) {
    vector<char*> tokens = StringUtil::explode(cmd,' ',0);
    tokens.push_back(NULL);
    if (execvp(binaryName.c_str(), &tokens[0]) < 0) {
      LOG(INFO) << "Failed to start this :" << cmd;
      exit(1);
    }
  } else {
    LOG(INFO) << "Empty command given, do nothing";
  }

}


bool CephExecutor::downloadDockerImage(string imageName)
{
  string cmd = "docker images|grep " + imageName +
      " > /dev/null 2>&1;echo $?";
  string r = runShellCommand(cmd);
  if ("0" == r) {
    LOG(INFO) << imageName <<" exists, so quit download";
    return true;
  }
  //TODO: fix me. Seems docker pull need proxy there.
  cmd = "docker pull " + imageName;
  LOG(INFO) << "Downloading " << imageName << " ...";
  LOG(INFO) << cmd;
  pid_t downloadPid = fork();
  if (-1 == downloadPid){
    LOG(ERROR) << "Failed to fork download thread";
  }
  if (0 == downloadPid){
    //child thread to download the docker
    startLongRunning("docker",cmd);
  } else {
    //parent ,wait util finish downloading
    int* status;
    waitpid(0,status,0);
    if(WIFEXITED(status)){
      LOG(INFO) << "Downloading done";
      return true;
    } else {
      LOG(INFO) << "Downloading done";
      return false;
    }
  }
  return false;
}

bool CephExecutor::parseHostConfig(TaskInfo taskinfo)
{
  if (taskinfo.has_labels()) {
    Labels labels = taskinfo.labels();
    int count = labels.labels_size();
    LOG(INFO) << "labels count: " <<count;
    if (0 == count) {
      LOG(ERROR) << "Zero host config data given";
      return false;
    }
    for (int i = 0; i < count; i++) {
      Label* one = labels.mutable_labels(i);
      if (0 == strcmp(CustomData::mgmtdevKey,one->key().c_str())) {
        LOG(INFO) << "got mgmt NIC: " <<one->value();
        mgmtNIC = one->value();
      }
      if (0 == strcmp(CustomData::datadevKey,one->key().c_str())) {
        LOG(INFO) << "got data NIC: " <<one->value();
        dataNIC = one->value();
      }
      if (0 == strcmp(CustomData::osddevsKey,one->key().c_str())) {
        LOG(INFO) << "got osd dev: " <<one->value();
        osddisk = one->value();
      }
      if (0 == strcmp(CustomData::jnldevsKey,one->key().c_str())) {
        LOG(INFO) << "got jnl dev: " <<one->value();
        jnldisk = one->value();
      }
    }
    return true;
  } else {
    LOG(ERROR)<<"No labels found in taskinfo!";
    return false;
  }
}

bool CephExecutor::prepareDisks()
{
  if ("" == osddisk) {
    LOG(INFO) << "No osd disk given, use system disk";
    return true;
  } else {
    if (!partitionDisk(osddisk, 1)) {
      return false;
    }
    string diskpath = "/dev/" + osddisk + "1";
    if (!mkfsDisk(diskpath, fsType, mkfsFLAGS)) {
      return false;
    }
    //create the mount dir
    string cmd = "mkdir -p " + localMountOSDDir;
    runShellCommand(cmd);
    if (!mountDisk(diskpath, localMountOSDDir, mountFLAGS)) {
      return false;
    }
  }
  //TODO: journal disk management
  return true;
}

bool CephExecutor::partitionDisk(string diskname, int partitionCount)
{
  string diskpath = "/dev/" + diskname;
  string cmd = "dd if=/dev/zero of=" + diskpath +
      " bs=4M count=1 oflag=direct 2>/dev/null;echo $?";
  LOG(INFO) << "dd..( "<<cmd<<" )";
  runShellCommand(cmd);
  LOG(INFO)<<"dd done.";
  cmd = "parted " + diskpath + "  mklabel gpt &>/dev/null 2>/dev/null";
  LOG(INFO) << "parted mklabel..( "<<cmd<<" )";
  runShellCommand(cmd);
  LOG(INFO)<<"parted mklabel done.";
  cmd = "parted " + diskpath + " p 2>/dev/null | grep \"Disk " +
      diskpath + "\" | awk '{print $3}'";
  LOG(INFO) << "get disk size..( "<<cmd<<" )";
  string disksize = runShellCommand(cmd);
  LOG(INFO)<<"disksize: "<<disksize;
  //TODO: support multiple partitions
  cmd = "parted "+diskpath+" mkpart data 0 " + disksize +
      " &>/dev/null 2>/dev/null";
  LOG(INFO) << "parted mkpart..( "<<cmd<<" )";
  runShellCommand(cmd);
  return true;
}

bool CephExecutor::mkfsDisk(string diskname, string type, string flags)
{
  string cmd = "mkfs." + type + flags + " " + diskname;
  LOG(INFO) << "mkfs..( "<<cmd<<" )";
  runShellCommand(cmd);
  return true;
}

bool CephExecutor::mountDisk(string diskname, string dir, string flags)
{
  string cmd = "mount" + flags + " " + diskname + " " + dir;
  LOG(INFO) << "mount..( "<<cmd<<" )";
  runShellCommand(cmd);
  return true;
}

string CephExecutor::getContainerName(string taskId)
{
  vector<string> tokens = StringUtil::explode(taskId,'.');
  return tokens[0];
}
// data format is:
// <MessageToExecutor>.<OSDID> for OSD executor
// or just <MessageToExecutor> for MON executor
void CephExecutor::frameworkMessage(ExecutorDriver* driver, const string& data)
{

  LOG(INFO) << "Got framework message: " << data;
  MessageToExecutor msg;
  vector<string> tokens = StringUtil::explode(data,'.');
  msg = (MessageToExecutor)lexical_cast<int>(tokens[0]);
  switch (msg){
    case MessageToExecutor::REGISTER_OSD:
      LOG(INFO) << "Will register an OSD, and return the OSD ID";
      driver->sendFrameworkMessage(registerOSD());
      break;
    case MessageToExecutor::LAUNCH_OSD:
      if (tokens.size() == 2){
        LOG(INFO) << "Will launch OSD docker with OSD ID: " << tokens[1];
        string localMountDir = localSharedConfigDirRoot +
            "/" + localConfigDirName;
        localMountOSDDir = localMountDir +
            "/var/lib/ceph/osd/ceph-" + tokens[1];
        TaskStatus status;
        status.mutable_task_id()->MergeFrom(myTaskId);
        //prepareOSDdisk and JournalDisk
        if (!prepareDisks()) {
          LOG(ERROR) << "Prepare Disks failed!";
          status.set_state(TASK_FAILED);
          driver->sendStatusUpdate(status);
          return;
        }
        string dockerCommand = constructOSDCommand(
            localMountDir,
            tokens[1],
            containerName);

        LOG(INFO) << "Stating OSD container with command: ";
        LOG(INFO) << dockerCommand;
        myPID = fork();
        if (0 == myPID){
            //child long running docker thread
            //TODO: we use fork here. Need to check why below line will hung the executor
            //thread(&CephExecutor::startLongRunning,*this,"docker", dockerCommand).detach();
            startLongRunning("docker",dockerCommand);
        } else {
          bool nicReady = createNICs(containerName, mgmtNIC, "eth-" + containerName);
          if (!nicReady) {
            LOG(INFO) << "Failed to create macvlan NIC for " << containerName;
            runShellCommand("docker rm -f " + containerName);
            status.set_state(TASK_FAILED);
            driver->sendStatusUpdate(status);
            return;
          }
          bool started = block_until_started(containerName, "30");
          if (started) {
            LOG(INFO) << "Starting OSD task " << myTaskId.value();
            //send the OSD id back to let scheduler remove it
            //format: <MessageToScheduler::CONSUMED_OSD_ID>.OSDID
            string msg =
              lexical_cast<string>(static_cast<int>(MessageToScheduler::CONSUMED_OSD_ID)) +
                "." + tokens[1];
            status.set_message(msg);
            status.set_state(TASK_RUNNING);
          } else {
            LOG(INFO) << "Failed to start OSD task " << myTaskId.value();
            status.set_state(TASK_FAILED);
          }
          driver->sendStatusUpdate(status);

        }//end else "0==pid"
      } else {
        LOG(INFO) << "No OSD ID given!";
      }
      break;
    default:
      LOG(INFO) << "unknown message from scheduler";
  }

}

void CephExecutor::shutdown(ExecutorDriver* driver)
{
  if ("" != localMountOSDDir) {
    LOG(INFO) << "Umount( "<<localMountOSDDir<<" )";
    LOG(INFO) << runShellCommand("umount -f " + localMountOSDDir);
  }
  LOG(INFO) << "Killing this container process";
  LOG(INFO) << runShellCommand("docker rm -f " + containerName);
  TaskStatus status;
  status.mutable_task_id()->MergeFrom(myTaskId);
  status.set_state(TASK_KILLED);
  driver->sendStatusUpdate(status);
}

string CephExecutor::runShellCommand(string cmd)
{
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) return "Run shell command ERROR!";
  char buffer[128];
  std::string result = "";
  while (!feof(pipe)) {
    if (fgets(buffer, 128, pipe) != NULL) {
      result += buffer;
    }
  }
  size_t last = result.find_last_not_of('\n');
  result = result.substr(0,(last+1));
  pclose(pipe);
  return result;
}

void CephExecutor::registered(
      ExecutorDriver* driver,
      const ExecutorInfo& executorInfo,
      const FrameworkInfo& frameworkInfo,
      const SlaveInfo& slaveInfo)
{
  //set class member myHostname
  myHostname = slaveInfo.hostname();
  LOG(INFO) << "Registered executor on " << myHostname;
  //sandboxAbsolutePath
  char temp[4096];
  sandboxAbsolutePath = getcwd(temp, 4096) ? string(temp) : std::string("");
  LOG(INFO) << "sandbox absolute path: " << sandboxAbsolutePath;
}

//when the task before starting,
//it should check task.data() to determin
//what it will do, whether copy config?
//whether start fileserver?
//
//task.data() here format is :
//<isInitialMonNode>.<TaskType>
void CephExecutor::launchTask(ExecutorDriver* driver, const TaskInfo& task)
{
  //set class member localSharedConfDirRoot
  string cmd = "echo ~";
  string r = runShellCommand(cmd);
  remove_if(r.begin(), r.end(), isspace);
  localSharedConfigDirRoot = r == "" ? r :"/root";
  LOG(INFO) << "localSharedConfigDirRoot is " << localSharedConfigDirRoot;

  bool needCopyConfig = true;
  bool needStartFileServer = false;
  int taskType;
  if (task.has_data()) {
    LOG(INFO) << "Got TaskInfo data: " << task.data();
    vector<string> tokens = StringUtil::explode(task.data(),'.');
    //split by '.', the first part is isInitialMonNode,
    //second part is used for task type
    if (tokens[0] == "1"){
      needCopyConfig = false;
      deleteConfigDir(localConfigDirName);
    }
    taskType = lexical_cast<int>(tokens[1]);
  }

  string localMountDir = localSharedConfigDirRoot +
      "/" +localConfigDirName;
  TaskStatus status;
  status.mutable_task_id()->MergeFrom(task.task_id());

  //parse custom data from scheduler, include mgmtdev, datadev and so on
  if(!parseHostConfig(task)) {
    status.set_state(TASK_FAILED);
    driver->sendStatusUpdate(status);
    return;
  }

  //make local shared dir, all type of task need this:
  if (!createLocalSharedConfigDir(localConfigDirName)) {
    LOG(INFO) << "created local shared directory failed!";
    status.set_state(TASK_FAILED);
    driver->sendStatusUpdate(status);
    return;
  }
  LOG(INFO) << "Create directory tree done.";
  //copy wfn.sh to local shared dir
  if (!copyWaitingNICEntryPoint(localMountDir)) {
    LOG(INFO) << "copy wfn.sh failed!";
    status.set_state(TASK_FAILED);
    driver->sendStatusUpdate(status);
    return;
  }
  LOG(INFO) << "Copy wfn.sh done.";
  //copy shared config file
  if (needCopyConfig) {
    string abPath = localMountDir + "/"
        + "/etc/ceph/";
    if (!copySharedConfigFiles(abPath)) {
      LOG(INFO) << "Copy shared config file failed!";
      status.set_state(TASK_FAILED);
      driver->sendStatusUpdate(status);
      return;
    }
    LOG(INFO) << "Copy config files done.";
  }

  //run docker command for MON and RADOSGW
  string cName = getContainerName(task.task_id().value());
  //set class member containerName, and myTaskId
  //TODO: see if put these in registed is more proper
  containerName = cName;
  myTaskId = task.task_id();
  //TODO: kill existing container in case conflict
  runShellCommand("docker rm -f " + containerName);

  string dockerCommand;
  switch (taskType) {
    case static_cast<int>(TaskType::MON):
      needStartFileServer = true;
      dockerCommand = constructMonCommand(
          localMountDir,
          cName);
      downloadDockerImage("ceph/mon");
      break;
    case static_cast<int>(TaskType::OSD):
      downloadDockerImage("ceph/osd");
      //Will get osdId in FrameworkMessage
      dockerCommand = "";
      status.set_state(TASK_STARTING);
      driver->sendStatusUpdate(status);
      return;
    case static_cast<int>(TaskType::RADOSGW):
      downloadDockerImage("ceph/radosgw");
      dockerCommand = constructRADOSGWCommand(
          localMountDir,
          cName);
      break;
  }

  if (needStartFileServer) {
    thread fileServerThread(fileServer,
        7777,
        localSharedConfigDirRoot + "/" + localConfigDirName + "/etc/ceph/");
    fileServerThread.detach();
    LOG(INFO) << "Mon fileserver started";
  }

  LOG(INFO) << "Stating container with command: ";
  LOG(INFO) << dockerCommand;

  //fork a thread to enable docker long running.
  //TODO: <thread> here seems not working, figure it out
  //to find a better way

  myPID = fork();
  if (0 == myPID){
    //child long running docker thread
    //TODO: we use fork here. Need to check why below line will hung the executor
    //thread(&CephExecutor::startLongRunning,*this,"docker", dockerCommand).detach();
    startLongRunning("docker",dockerCommand);
  } else {
    //parent thread
    //new a macvlan interface for this container

    bool nicReady = createNICs(containerName, mgmtNIC, "eth-" + containerName);
    if (!nicReady) {
      LOG(INFO) << "Failed to create macvlan NIC for " << containerName;
      runShellCommand("docker rm -f " + containerName);
      status.set_state(TASK_FAILED);
      driver->sendStatusUpdate(status);
      return;
    }

    //check if started normally
    bool started = block_until_started(cName, "60");
    if (started) {
      LOG(INFO) << "Starting task " << task.task_id().value();
      status.set_state(TASK_RUNNING);
    } else {
      LOG(INFO) << "Failed to start task " << task.task_id().value();
      status.set_state(TASK_FAILED);
    }
    driver->sendStatusUpdate(status);
  }
}

int main(int argc, const char* argv[])
{
  CephExecutor executor;
  MesosExecutorDriver driver(&executor);
  return driver.run() == DRIVER_STOPPED ? 0 : 1;
}
