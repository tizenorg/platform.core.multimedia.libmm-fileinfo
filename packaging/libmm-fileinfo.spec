Name:	    libmm-fileinfo
Summary:    Media Fileinfo
Version:    0.3.0
Release:    15
Group:      System/Libraries
License:    LGPL
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

BuildRequires: pkgconfig(mm-common)
BuildRequires: pkgconfig(mm-log)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(libavcodec)
BuildRequires: pkgconfig(libavutil)
BuildRequires: pkgconfig(libavformat)

%define use_drm 1

%if %{use_drm}
BuildRequires: libss-client-devel
BuildRequires: pkgconfig(drm-service)
%endif

%description
Multimedia Framework FileInfo Library


%package devel
Summary:    Media Fileinfo
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Multimedia Framework FileInfo Library (developement files)

%prep
%setup -q

%build
./autogen.sh

%if %{use_drm}
CFLAGS="${CFLAGS} -D_MM_PROJECT_FLOATER -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" " LDFLAGS="${LDFLAGS}" ./configure  --disable-testmode --disable-dump --enable-dyn --disable-iommap --prefix=/usr --enable-drm --disable-gtk
%else
CFLAGS="${CFLAGS} -D_MM_PROJECT_FLOATER -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" " LDFLAGS="${LDFLAGS}" ./configure --disable-testmode --disable-dump --enable-dyn --disable-iommap --prefix=/usr --disable-gtk
%endif

make

%install
rm -rf %{buildroot}
%make_install


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
/usr/bin/memtrace_reader
/usr/bin/mm_file_test
/usr/lib/libmmffile.so.0
/usr/lib/libmmffile.so.0.0.0
/usr/lib/libmmfile_codecs.so.0
/usr/lib/libmmfile_codecs.so.0.0.0
/usr/lib/libmmfile_formats.so.0
/usr/lib/libmmfile_formats.so.0.0.0
/usr/lib/libmmfile_utils.so.0
/usr/lib/libmmfile_utils.so.0.0.0


%files devel
%defattr(-,root,root,-)
/usr/include/mmf/mm_file.h
/usr/lib/pkgconfig/mm-fileinfo.pc

/usr/lib/libmmffile.so
/usr/lib/libmmfile_codecs.so
/usr/lib/libmmfile_formats.so
/usr/lib/libmmfile_utils.so
