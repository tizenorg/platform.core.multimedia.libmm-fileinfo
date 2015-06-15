Name:	    libmm-fileinfo
Summary:    Media Fileinfo
Version:    0.6.31
Release:    0
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001:    libmm-fileinfo.manifest
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

BuildRequires: pkgconfig(mm-common)
BuildRequires: pkgconfig(mm-log)
BuildRequires: pkgconfig(libswscale)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(libavcodec)
BuildRequires: pkgconfig(libavutil)
BuildRequires: pkgconfig(libavformat)
BuildRequires: pkgconfig(icu-i18n)
BuildRequires: pkgconfig(vconf)

%define use_drm 0

%if %{use_drm}
BuildRequires: libss-client-devel
#BuildRequires: pkgconfig(drm-client)
#BuildRequires: pkgconfig(drm-trusted)
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
cp %{SOURCE1001} .

%build
CFLAGS="${CFLAGS} -D_MM_PROJECT_FLOATER -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" "
export CFLAGS

%reconfigure \
    --disable-testmode \
    --disable-dump \
    --enable-dyn \
    --disable-iommap \
    --disable-gtk \
%if %{use_drm}
CFLAGS="${CFLAGS} -D_MM_PROJECT_FLOATER -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" " LDFLAGS="${LDFLAGS}" ./configure  --disable-testmode --disable-dump --enable-dyn --disable-iommap --prefix=/usr --enable-drm --disable-gtk
%else
CFLAGS="${CFLAGS} -D_MM_PROJECT_FLOATER -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" " LDFLAGS="${LDFLAGS}" ./configure --disable-testmode --disable-dump --enable-dyn --disable-iommap --prefix=/usr --disable-drm --disable-gtk
%endif

%__make

%install
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%{_bindir}/memtrace_reader
%{_bindir}/mm_file_test
%{_libdir}/libmmffile.so.*
%{_libdir}/libmmfile_codecs.so.*
%{_libdir}/libmmfile_formats.so.*
%{_libdir}/libmmfile_utils.so.*
%{_libdir}/libmmffile.so
%{_libdir}/libmmfile_codecs.so
%{_libdir}/libmmfile_formats.so
%{_libdir}/libmmfile_utils.so
%license LICENSE.APLv2.0

%files devel
%manifest %{name}.manifest
%{_includedir}/mmf/mm_file.h
%{_libdir}/pkgconfig/mm-fileinfo.pc

