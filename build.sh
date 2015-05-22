cd /usr/local/src/
yum -y install rpm-build redhat-rpm-config gcc ncurses-devel git bc
git clone https://src.openvz.org/scm/ovz/vzkernel.git
cd vzkernel
cp config.OpenVZ .config
echo "%post" >> scripts/package/mkspec
echo "/sbin/new-kernel-pkg --package kernel --mkinitrd --depmod --install "$KERNELRELEASE" || exit \$?" >> scripts/package/mkspec
echo "grub2-mkconfig -o /boot/grub2/grub.cfg " >> scripts/package/mkspec
echo "grub2-set-default 1" >> scripts/package/mkspec
echo "" >> scripts/package/mkspec
echo "%preun" >> scripts/package/mkspec
echo "/sbin/new-kernel-pkg --rminitrd --rmmoddep --remove "$KERNELRELEASE" || exit \$?" >> scripts/package/mkspec
echo "" >> scripts/package/mkspec
make -j4 binrpm-pkg
rpm --force -Uvh /root/rpmbuild/RPMS/x86_64/kernel-3.10.0+-3.x86_64.rpm /root/rpmbuild/RPMS/x86_64/*.rpm
cd /usr/local/src
git clone https://src.openvz.org/scm/ovzl/ploop.git
git clone https://src.openvz.org/scm/ovzl/vzquota.git
git clone https://src.openvz.org/scm/ovzl/vzctl.git
git clone https://src.openvz.org/scm/ovzl/vzstats.git
git clone https://src.openvz.org/scm/ovzl/vzpkg.git
cd ploop
yum install -y libxml2-devel  e2fsprogs-devel
make rpms
cd ../vzquota/
make rpms
cd ../
yum -y install libtool automake.noarch autoconf lsof wget texinfo libselinux-devel libsepol-devel libcgroup.x86_64 libcgroup-devel.x86_64 bridge-utils attr
http://openvz.sciserv.eu/utils/e2fsprogs/1.42.11-1.ovz/src/e2fsprogs-1.42.11-1.ovz.src.rpm
rpm -i e2fsprogs-1.42.11-1.ovz.src.rpm
rpmbuild -ba /root/rpmbuild/SPECS/e2fsprogs.spec
rpm -i /root/rpmbuild/RPMS/x86_64/e2fsprogs-*
rpm --force -Uvh /root/rpmbuild/RPMS/x86_64/ploop-*
cd vzctl
./autogen.sh
make rpm
rpm --force --nodeps -Uvh /root/rpmbuild/RPMS/x86_64/vzquota*
cd ../vzstats
make rpms
rpm --force -Uvh /root/rpmbuild/RPMS/noarch/vzstats*
rpm --force -Uvh /root/rpmbuild/RPMS/x86_64/vzctl*
