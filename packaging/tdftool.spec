# This is a sample spec file for wget
 
%define name        tdftool 
%define release     0 
%define version     0.1 
 
Summary:        TDFTool 
License:        MIT
Name:           %{name}
Version:        %{version}
Release:        %{release}
Source:         %{name}-%{version}.tar.gz
Prefix:         /usr
Group:          Development/Tools
 
%description
A utility for rendering TDF ("TheDraw") fonts on a terminal.
 
%prep
%setup -q
%build
make
%install
mkdir -p $RPM_BUILD_ROOT/usr/bin
cp tdftool $RPM_BUILD_ROOT/usr/bin/tdftool
cp tdftool-utf8.sh $RPM_BUILD_ROOT/usr/bin/tdftool-utf8.sh
 
%files
%defattr(0755,root,root)
/usr/bin/tdftool
/usr/bin/tdftool-utf8.sh
 
