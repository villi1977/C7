yum install -y libxml2-devel  e2fsprogs-devel libtool automake.noarch autoconf lsof wget texinfo libselinux-devel libsepol-devel libcgroup.x86_64 libcgroup-devel.x86_64 bridge-utils attr wget
rm -f /root/rpmbuild/RPMS/x86_64/*
rpm -i e2fsprogs-1.42.11-1.ovz.src.rpm
rpmbuild -ba /root/rpmbuild/SPECS/e2fsprogs.spec
rpm -i /root/rpmbuild/RPMS/x86_64/e2fsprogs-*
cd ploop
make rpms
rpm --force --nodeps -Uvh /root/rpmbuild/RPMS/x86_64/ploop-*
cd ../vzquota/
make rpms
rpm --force --nodeps -Uvh /root/rpmbuild/RPMS/x86_64/vzquota*
cd ../vzctl
./autogen.sh
make rpms
cd ../vzstats
make rpms
rpm --force -Uvh /root/rpmbuild/RPMS/noarch/vzstats*
rpm --force -Uvh --nodeps /root/rpmbuild/RPMS/x86_64/vzctl*
