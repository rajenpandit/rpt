LOCAL_LIB=tcp_server/libtcpserver.la thread_pool/libthreadpool.la
##noinst_LIBRARIES = librpt.a
lib_LTLIBRARIES = librpt.la
librpt_la_SOURCES=
librpt_la_LIBADD=$(LOCAL_LIB)
nodist_EXTRA_librpt_la_SOURCES = dummy.cpp
librpt_la_LDFLAGS=-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

SUBDIRS=tcp_server thread_pool
