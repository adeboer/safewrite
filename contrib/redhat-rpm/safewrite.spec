Name: safewrite
Summary: Safely write STDOUT
Version: 1.3
Release: 0
Copyright: GPL
Group: Applications/Networking
Source: safewrite-1.3.tar.gz
BuildRoot: /tmp/safewrite.build

%description
Safely write STDOUT

%prep
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/man/man1
echo $RPM_BUILD_ROOT/usr > installprefix

%setup

%build
make

%install
make install

%files
%defattr(-, root, root)
%doc GPL
/usr/bin/safewrite
/usr/man/man1/safewrite.1.gz
