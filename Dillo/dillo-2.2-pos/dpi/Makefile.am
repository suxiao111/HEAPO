bookmarksdir = $(libdir)/dillo/dpi/bookmarks
downloadsdir = $(libdir)/dillo/dpi/downloads
ftpdir = $(libdir)/dillo/dpi/ftp
httpsdir = $(libdir)/dillo/dpi/https
hellodir = $(libdir)/dillo/dpi/hello
filedir = $(libdir)/dillo/dpi/file
cookiesdir = $(libdir)/dillo/dpi/cookies
datauridir = $(libdir)/dillo/dpi/datauri
bookmarks_PROGRAMS = bookmarks.dpi
downloads_PROGRAMS = downloads.dpi
ftp_PROGRAMS = ftp.filter.dpi
https_PROGRAMS = https.filter.dpi
hello_PROGRAMS = hello.filter.dpi
file_PROGRAMS = file.dpi
cookies_PROGRAMS = cookies.dpi
datauri_PROGRAMS = datauri.filter.dpi

bookmarks_dpi_LDADD = ../dpip/libDpip.a ../dlib/libDlib.a
downloads_dpi_LDADD = @LIBFLTK_LIBS@ ../dpip/libDpip.a ../dlib/libDlib.a
ftp_filter_dpi_LDADD = ../dpip/libDpip.a ../dlib/libDlib.a
https_filter_dpi_LDADD = @LIBSSL_LIBS@ ../dpip/libDpip.a ../dlib/libDlib.a
hello_filter_dpi_LDADD = ../dpip/libDpip.a ../dlib/libDlib.a
file_dpi_LDADD = ../dpip/libDpip.a ../dlib/libDlib.a
cookies_dpi_LDADD = ../dpip/libDpip.a ../dlib/libDlib.a
datauri_filter_dpi_LDADD = ../dpip/libDpip.a ../dlib/libDlib.a

downloads_dpi_CXXFLAGS = @LIBFLTK_CXXFLAGS@

bookmarks_dpi_SOURCES = bookmarks.c dpiutil.c dpiutil.h
downloads_dpi_SOURCES = downloads.cc dpiutil.c dpiutil.h
ftp_filter_dpi_SOURCES = ftp.c dpiutil.c dpiutil.h
https_filter_dpi_SOURCES = https.c dpiutil.c dpiutil.h
hello_filter_dpi_SOURCES = hello.c dpiutil.c dpiutil.h
file_dpi_SOURCES = file.c dpiutil.c dpiutil.h
cookies_dpi_SOURCES = cookies.c dpiutil.c dpiutil.h
datauri_filter_dpi_SOURCES = datauri.c dpiutil.c dpiutil.h

