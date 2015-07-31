Ceph on Apache Mesos
======================
A Mesos framework utilizing Docker for scaling a Ceph cluster. It aims to be a fast and reliable solution.  
For now, it can only: 
  - Start 1 mon, 3 osd, 1 radosgw. Each on a slave. Based on [ceph-docker]
  - Accept Restful flexup request for launching a new osd to a new slave

Goal & Roadmap
--------------------------
Scaling and monitoring large Ceph cluster in production Mesos environment in an easy way is our goal. And it's in progress. Check below for updates ( Your ideas are welcome ).
- [ ] Multiple OSDs on one disk
- [ ] Multiple OSDs on dedicated disks
- [ ] Journals on dedicated disks
- [ ] Flexdown OSD
- [ ] Launch RBD, MDS
- [ ] TBD

Prerequisites
--------------------------
1. A Mesos cluster with Docker installed (duh). We only support CentOS 7 distribution at present and requires at least <b><em>5</em></b> slaves.
2. Slaves in Mesos have network connection to download Docker images
3. install libmicrohttpd in all slaves.
```sh
yum -y install libmicrohttpd
```

Build Ceph-Mesos
--------------------------
Pick a host to setup the build environment. Make sure the host can access your Mesos cluster.
```sh
sudo rpm -Uvh http://repos.mesosphere.io/el/7/noarch/RPMS/mesosphere-el-repo-7-1.noarch.rpm
sudo yum install -y epel-release
sudo yum groupinstall -y "Development Tools"
sudo yum install -y cmake mesos protobuf-devel boost-devel gflags-devel glog-devel yaml-cpp-devel  jsoncpp-devel libmicrohttpd-devel zeromq3-devel gmock-devel gtest-devel
```
Then clone ceph-mesos and build.
```sh
git clone 
cd ceph-mesos
mkdir build
cd build
cmake ..
make
```
After that, you'll see "ceph-mesos", "ceph-mesos-executor", "ceph-mesos-tests" , "cephmesos.yml" , "ceph.conf" and "cephmesos.d" in the build directory.

Run Ceph-Mesos
--------------------------
Modify the cephmesos.yml, you must populate the master and zookeeper fields, can leave other field default:
```sh
master:     zk://mm01:2181,mm02:2181,mm03:2181/mesos
zookeeper:  zk://mm01:2181,mm02:2181,mm03:2181
```
And start ceph-mesos using below command:
```sh
./ceph-mesos -config cephmesos.yml
```
You can check the Mesos web console to see your ceph cluster now. After about 10 mins(depend on your network speed), you'll see 5 active tasks running there.

**NOTE:** Currently, if "ceph-meos" (the scheduler) stops, all containers will be removed. And restart "ceph-mesos" will clear your data( in slave's "~/ceph_config_root" which is bind-mounted by Docker container) and start a new ceph cluster. We will improve this in the near future.

Launch new OSD(s)
--------------------------
ceph-mesos can accept json format request and start new OSD(s) if there are available hosts.
```sh
curl -d '{"instances":2,"profile":"osd"}' http://ceph_scheduler_host:8889
```

Verify your Ceph cluster
--------------------------
You can ssh to a Mesos slave running a ceph-mesos task and execute docker commands.
```sh
#you'll probably see a osd0 container running
docker ps -a 
docker exec osd0 ceph -s
```
Now you can verify your Ceph cluster!


[ceph-docker]: https://github.com/ceph/ceph-docker
