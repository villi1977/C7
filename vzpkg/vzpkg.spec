Name:		vzpkg
Version:	3.0.0
Release:	1
Summary:	OpenVZ template management tools
Source:		%{name}-%{version}.tar.bz2
License:	GPL
URL:		http://openvz.org/
Group:		Applications/System
BuildRoot:	%{_tmppath}/%{name}-%{version}-root
BuildArch:	noarch

Requires:	sed, gawk, coreutils
Requires:	procmail
Requires:	vzctl >= 2.7.0-23
Requires:	vzyum >= 2.4.0-5
# NOTE this package actually requires vzrpm of some version installed,
# but the version of vzrpm depends on OS template used.
# Thus this requirement for vzrpmXX is put to OS template metadata instead.

# New vzpkg does not work with old template metadata.
Conflicts:	vztmpl-fedora-core-3 > 2.0
Conflicts:	vztmpl-fedora-core-4 > 2.0
Conflicts:	vztmpl-centos-4 > 2.0
# Since vzpkg-3.0 it requires newer vzrpms
# (the ones with dynamically linked python modules)
Conflicts:	vzrpm43 < 4.3.3-13_nonptl.1.swsoft
#Conflicts:	vzrpm44 < 

%define libdir %_datadir/%name

#avoid stripping
%define __strip /bin/true

%description
OpenVZ template management tools are used for software installation
inside Virtual Environments.


%prep
%setup -n %{name}-%{version}

%install
make DESTDIR=%buildroot install

%files
%defattr(-, root, root)
%attr(755,root,root) %_bindir/vzpkgcache
%attr(755,root,root) %_bindir/vzyum
%attr(755,root,root) %_bindir/vzrpm
%attr(755,root,root) %_bindir/vzpkgadd
%attr(755,root,root) %_bindir/vzpkgrm
%attr(755,root,root) %_bindir/vzpkgls
%attr(755,root,root) %_bindir/vzosname
%dir %libdir
%attr(644,root,root) %libdir/functions
%attr(755,root,root) %libdir/cache-os
%attr(755,root,root) %libdir/myinit.*
%_mandir/man8/vzpkgcache.8.*
%_mandir/man8/vzyum.8.*
%_mandir/man8/vzrpm.8.*
%doc README NEWS TODO COPYING

%clean
test "x$RPM_BUILD_ROOT" != "x" && rm -rf $RPM_BUILD_ROOT


%changelog
* Thu May 11 2006 Kir Kolyshkin <kir-at-openvz.org> 3.0.0-1
- use newer vzrpms with dynamically linked python modules
- relicensed under GNU GPL
- changed VPS to VE

* Tue Oct 25 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-18
- support for different (per OSTEMPLATE) rpm versions (bug #27)
- support/requirement for customized yum
- rpm/yum now executes scripts in VPS context
- mounting of /proc is done in VPS context (by our init)
- preliminary support for template 'flavours' (bug #15)
- support for per-ostemplate .rpmmacros (bug #16)
- fixed bug #28 (vzyum should exit if there's no relevant yum.conf)
- included statically built myinit for different platforms
- added vzosname utility
- added requirement for procmail (needed for lockfile util)
- fixed bug #63 (template cache should not be created if install-post fails)

* Fri Sep 16 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-17
- fixed vzpkgcache broken by vzctl-2.7.0-21
- project name changed

* Mon Aug 15 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-16
- fixed cleanup of temporary VPS in vzpkgcache (bug #19)
- no need to repack cache tarball if no updates are available (bug #13)
- fixed program name logging to /var/log/vzpkg.log
- added README, NEWS, TODO (bug #14)

* Wed Aug 10 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-15
- copyright statement and LICENSE.QPL added

* Sun Aug  7 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-14
- minor manpages fixes
- removed 0. prefix from the release
- support for vzpkgls -q VPS added for compatibility

* Thu Jul 27 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-0.13
- myinit is installed with mode 755 in Makefile

* Tue Jul 26 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-0.12
- source tarball now contains {release}

* Tue Jul 19 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-0.11
- vzpkgcache properly fails now if vzctl status returns !0 (vz.org bug #11)

* Thu Jul 14 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-0.10
- fixed order of 'tar' arguments in cache-os to work with newer GNU tar
- added checking of tar exit code

* Thu Jul 14 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-0.9
- fixed order of 'find' arguments to avoid warnings
- added 'dist' and 'rpm' targets to Makefile

* Wed Jun 22 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-0.8
- added -q, -o options to vzpkgls (for compatibility only)

* Wed Jun 22 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-0.7
- added vzpkgls utility

* Tue Jun 21 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-0.6
- removed workaround for old vzctl which expect cache tarball in /vz/template
- require newer vzctl which gets cache from /vz/template/cache

* Tue Jun 21 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-0.5
- fixed vzrpm (use --root not --installroot for rpm)

* Tue Jun 21 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-0.4
- added -f|--force option to vzpkgadd/vzpkgrm
- added ability to install gpgkeys from config/gpgkeys if available, as yum currently can not handle
  more that one gpgkey per repo

* Wed Jun  8 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-0.3
- added vzrpm wrapper
- added simple vzpkgadd/vzpkgrm wrappers (w/out man pages)
- right name is 'Open Virtuozzo'

* Tue Mar 24 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-0.2
- removed -d and -e options from yum calls
- added dependency for yum>=2.2.1
- cache-os now creates/removes symlink to tarball in $TEMPLATE for older vzctl
- use ve-vps.basic.conf-sample as VPS config in cache-os

* Mon Mar 23 2005 Kir Kolyshkin <kir-at-sw.ru> 2.7.0-0.1
- initially packaged
