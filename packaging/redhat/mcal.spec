Name: mcal
Version: 0.7cvs
Summary: A Calendaring library, and drivers
Release: 1
Group: System Environment/Libraries
URL: http://libmcal.sourcefourge.net
Source0: libmcal-0.7-cvs.tar.gz
Source1: mcaldrivers-0.9-cvs.tar.gz
License: GPL and partial BSD-Like with an advertising clause for mstore driver (driver is included)
BuildRoot: %{_tmppath}/%{name}-%{version}-root

%description
The mcal library and the mcal drivers implement a simple calendaring server
and (currently) two calendaring storage adapators, one each for the icap protocol
and local files system.

%package devel
Summary: Files needed for developing applications which use the mcal
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
The mcal library and the mcal drivers implement a simple calendaring server
and (currently) two calendaring storage adapators, one each for the icap protocol
and local files system. The mcal-devel package contains header files necessary
to compile other packages to use the mcal library and its drivers.


%prep

# The libmcal package and drivers are packaged separately but integrated, you have to integrate by hand
rm -rf $RPM_BUILD_DIR/libmcal
tar -zxvf $RPM_SOURCE_DIR/libmcal-0.7-cvs.tar.gz
tar -zxvf $RPM_SOURCE_DIR/mcaldrivers-0.9-cvs.tar.gz
mv $RPM_BUILD_DIR/mcal-drivers/icap $RPM_BUILD_DIR/libmcal
mv $RPM_BUILD_DIR/mcal-drivers/mstore $RPM_BUILD_DIR/libmcal


%build
CFLAGS="$RPM_OPT_FLAGS -fPIC" ; export CFLAGS

# We have to build the mstore and icap drivers by hand since they're not automated
cd $RPM_BUILD_DIR/libmcal/mstore
make
cd ../icap
make
cd ..

# libmcal's configure script isn't executable out of the box
chmod 755 ./configure

# The prefix here is because libmcal's installation is a bit weird
%configure --prefix=$RPM_BUILD_ROOT/usr --with-mstore --with-icap
make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

# This is an artifict of libmcal not being "standard" and exploding into libmcal-%{version}
cd $RPM_BUILD_DIR/libmcal
%makeinstall

# This is because libmcal doesn't version its libraries in its default build
ln -s $RPM_BUILD_ROOT/usr/lib/libmcal.so libmcal.so.0.7
ln -s $RPM_BUILD_ROOT/usr/lib/libmcal.so libmcal.so.0

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig
if (test ! -d /var/calendar); then
  mkdir /var/calendar;
  chmod 1777 /var/calendar;
fi
if (test ! -f /etc/mpasswd); then htpasswd -bc /etc/mpasswd mcaluser mcalpass; fi

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%{_libdir}/*.a
%doc libmcal/LICENSE libmcal/CHANGELOG libmcal/FAQ-MCAL libmcal/FEATURE-IMPLEMENTATION 
%doc libmcal/FUNCTION-REF.html libmcal/HOW-TO-MCAL libmcal/README
%doc libmcal/icap/Changelog libmcal/icap/LICENSE
%doc libmcal/mstore/README libmcal/mstore/Changelog libmcal/mstore/LICENSE
%attr(0755,root,root) %{_libdir}/*.so*

%files devel
%defattr(-,root,root)
%{_includedir}/*

%changelog
* Mon Dec 03 2001 Mike Hardy <mike@h3c.org>
- adding in calendar directory and password creation to post install screen

* Sun Dec 02 2001 Mike Hardy <mike@h3c.org>
- initial package
