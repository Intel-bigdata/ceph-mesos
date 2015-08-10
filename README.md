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
- [x] Journals on dedicated disks
- [ ] Public and cluster network
- [ ] Flexdown OSD
- [ ] Launch RBD, MDS
- [ ] TBD

Prerequisites
--------------------------
1. A Mesos cluster with Docker installed (duh). We only support CentOS 7 distribution at present and requires at least <b><em>1</em></b> slave
2. Slaves in Mesos have network connection to download Docker images
3. Install libmicrohttpd in all slaves
```sh
yum -y install libmicrohttpd
```
**Mesos Resource Configuration(Optional)**

Ceph-Mesos supports "role" setting so you can constrain ceph cluster's resource through Mesos's role or resource configuration. For instance, you want 10 slaves in the Mesos cluster to be role "ceph" in order to deploy ceph-mesos on them.

* In cephmesos.yml, fill "ceph" in role field
* On mesos master hosts
```sh
#if no other existing roles, just echo ceph > /etc/mesos-master/roles
echo ",ceph" >> /etc/mesos-master/roles
sudo service mesos-master restart
```
* On each slave where you want it to be role "ceph", set role for it and remove the old task state
```sh
echo ceph > /etc/mesos-slave/default_role
sudo service mesos-slave stop
rm -f /tmp/mesos/meta/slaves/latest
sudo service mesos-slave start
```
With such Mesos configuration, ceph-mesos can only accept offers from the 10 hosts that have the role "ceph".

**NOTE:** You can also set "resource" configuration instead of "role" to reserve resources
```sh
echo "cpus(*):8;cpus(ceph):4;mem(*):16384;mem(ceph):8192" > /etc/mesos-slave/resources
```
Detailed configurations please refer to:  
https://open.mesosphere.com/reference/mesos-master/  
https://open.mesosphere.com/reference/mesos-slave/

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
Configure the cephmesos.yml and cephmesos.d/{hostname}.yml before you go if you want Ceph-Mesos to prepare the disks.

Ceph-Mesos needs to know which disks are avlaible for OSD launching. Here comes cephmesos.yml and cephmesos.d/{hostname}.yml. Cephmesos.yml is for common subset settings of hosts and cephmesos.d/{hostname}.yml is particular settings of a dedicated host. Once the osddevs and jnldevs are populated, Ceph-Mesos will make partitions and filesystem on the disks and bind-mount them for OSD containers to use.

For instance, assume we have 5 slaves. 4 of them have same one disk "sdb" when execute "fdisk -l", but slave5 have another "sdc". So we need to create a cephmesos.d/slave5.yml which have addition "sdc" in field "osddevs". In this situation, Ceph-Mesos can use "sdc" to launch containers in slave5, but others only have "sdb".

And you must populate the id, role and master field, can leave other field default. Sample configurations are as follows:

cephmesos.yml:
```sh
id:         myceph
role:       ceph
master:     zk://mm01:2181,mm02:2181,mm03:2181/mesos
zookeeper:  ""
restport:   8889
fileport:   8888
fileroot:   ./
mgmtdev:    ""
datadev:    ""
osddevs:
  - sdb
jnldevs:    []
```
cephmesos.d/slave5.yml:
```sh
osddevs:
  - sdc
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
curl -d '{"instances":2,"profile":"osd"}' http://ceph_scheduler_host:8889/api/cluster/flexup
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
