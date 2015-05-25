yum install -y libxml2-devel  e2fsprogs-devel libtool automake.noarch autoconf lsof wget texinfo libselinux-devel libsepol-devel libcgroup.x86_64 libcgroup-devel.x86_64 bridge-utils attr wget
#cd /usr/local/src
wget http://openvz.sciserv.eu/utils/e2fsprogs/1.42.11-1.ovz/src/e2fsprogs-1.42.11-1.ovz.src.rpm
rpm -i e2fsprogs-1.42.11-1.ovz.src.rpm
rpmbuild -ba /root/rpmbuild/SPECS/e2fsprogs.spec
rpm -i /root/rpmbuild/RPMS/x86_64/e2fsprogs-*
#git clone https://src.openvz.org/scm/ovzl/ploop.git
#git clone https://src.openvz.org/scm/ovzl/vzquota.git
#git clone https://src.openvz.org/scm/ovzl/vzctl.git
#git clone https://src.openvz.org/scm/ovzl/vzstats.git
#git clone https://src.openvz.org/scm/ovzl/vzpkg.git
cd ploop
make rpms
cd ../vzquota/
make rpms
rpm --force -Uvh /root/rpmbuild/RPMS/x86_64/ploop-*
rpm --force --nodeps -Uvh /root/rpmbuild/RPMS/x86_64/vzquota*
cd ../vzctl
./autogen.sh
make rpms
cd ../vzstats
make rpms
rpm --force -Uvh /root/rpmbuild/RPMS/noarch/vzstats*
rpm --force -Uvh /root/rpmbuild/RPMS/x86_64/vzctl*
