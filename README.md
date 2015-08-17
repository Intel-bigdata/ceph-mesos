Ceph on Apache Mesos
======================
A Mesos framework utilizing Docker for scaling a Ceph cluster. It aims to be a fast and reliable solution.  
For now, it can only:
  - Start 1 mon, 3 osd, 1 radosgw. Based on [ceph-docker]
  - Accept RESTful flexup request for launching a new osd

Goal & Roadmap
--------------------------
Scaling and monitoring large Ceph cluster in production Mesos environment in an easy way is our goal. And it's in progress. Check below for updates ( Your ideas are welcome ).
- [x] Multiple OSDs on one disk
- [x] Multiple OSDs on dedicated disks
- [ ] Journals on dedicated disks
- [ ] Flexdown OSD
- [ ] Launch RBD, MDS
- [ ] TBD

Prerequisites
--------------------------
1. A Mesos cluster with Docker installed (duh). We only support CentOS 7 distribution at present and requires at least <b><em>1</em></b> slave
2. DHCP service configured since ceph-Mesos use Macvlan to assign IP to containers
3. Slaves in Mesos have network connection to download Docker images
4. Install libmicrohttpd in all slaves
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
sudo yum install -y cmake mesos protobuf-devel boost-devel gflags-devel glog-devel yaml-cpp-devel  jsoncpp-devel libmicrohttpd-devel gmock-devel gtest-devel
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
Configure the cephmesos.yml and cephmesos.d/{hostname}.yml before you go.

Ceph-Mesos chooses Macvlan to assign different IPs( DHCP needed) for OSDs in same host to work around the issue 19 mentioned in ceph-docker.And it supports disk management now. It can partition, mkfs.xfs and mount the disks for you.

So Ceph-Mesos needs to know which network card link to use and which disks are avlaible for OSD launching. Here comes cephmesos.yml and cephmesos.d/{hostname}.yml. Cephmesos.yml is for common settings of hosts and cephmesos.d/{hostname}.yml is particular settings of a dedicated host.

For instance, assume we have 5 slaves. 4 of them have same network card link name "eno1" when execute "ip a", but slave5 have "eno2". So we need to create a cephmesos.d/slave5.yml which have corret "mgmtdev: eno2" in it. In this situation, Ceph-Mesos will launch containers in slave5 based on "eno2", but others based on "eno1". Disk settings are similar.

And you must populate the master, zookeeper and mgmtdev fields, can leave other field default. Sample configurations are as follows:

cephmesos.yml:
```sh
master:     zk://mm01:2181,mm02:2181,mm03:2181/mesos
zookeeper:  zk://mm01:2181,mm02:2181,mm03:2181
restport:   8889
fileport:   8888
fileroot:   ./
mgmtdev:    eno1 #the common network link name used for creating Macvlan device
datadev:    ""
osddevs:
  - sdb
  - sdc
jnldevs:    []
```
cephmesos.d/slave5.yml:
```sh
mgmtdev:    eno2
osddevs:
  - sdd
  - sde
jnldevs:    []
```

After finishing all these configurations, we can start ceph-mesos using below command:
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
