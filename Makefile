#COMAKE2 edit-mode: -*- Makefile -*-
####################64Bit Mode####################
ifeq ($(shell uname -m),x86_64)
CC=gcc
CXX=g++
CXXFLAGS=-g \
  -pipe \
  -W \
  -fPIC
CFLAGS=-g \
  -pipe \
  -W \
  -fPIC
CPPFLAGS=-D_GNU_SOURCE \
  -D__STDC_LIMIT_MACROS \
  -DVERSION=\"1.9.8.7\"
INCPATH=-I. \
  -I./include \
  -I./output \
  -I./output/include \
  -I./deps/libevent/include \
  -I./deps/slb/output/include
DEP_INCPATH=-I../../../../../lib2-64/bsl \
  -I../../../../../lib2-64/bsl/include \
  -I../../../../../lib2-64/bsl/output \
  -I../../../../../lib2-64/bsl/output/include \
  -I../../../../../lib2-64/ullib \
  -I../../../../../lib2-64/ullib/include \
  -I../../../../../lib2-64/ullib/output \
  -I../../../../../lib2-64/ullib/output/include \
  -I../../../../../public/configure \
  -I../../../../../public/configure/include \
  -I../../../../../public/configure/output \
  -I../../../../../public/configure/output/include \
  -I../../../../../public/nshead \
  -I../../../../../public/nshead/include \
  -I../../../../../public/nshead/output \
  -I../../../../../public/nshead/output/include \
  -I../../../../../public/spreg \
  -I../../../../../public/spreg/include \
  -I../../../../../public/spreg/output \
  -I../../../../../public/spreg/output/include \
  -I../../../../../third-64/pcre \
  -I../../../../../third-64/pcre/include \
  -I../../../../../third-64/pcre/output \
  -I../../../../../third-64/pcre/output/include

#============ CCP vars ============
CCHECK=@ccheck.py
CCHECK_FLAGS=
PCLINT=@pclint
PCLINT_FLAGS=
CCP=@ccp.py
CCP_FLAGS=


#COMAKE UUID
COMAKE_MD5=67fe25e50767130f40a9faa65a4d7190  COMAKE


.PHONY:all
all:comake2_makefile_check deps/libevent/.libs/libevent.a deps/slb/output/lib/libslb.a proxy 
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mall[0m']"
	@echo "make all done"

.PHONY:comake2_makefile_check
comake2_makefile_check:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mcomake2_makefile_check[0m']"
	#in case of error, update 'Makefile' by 'comake2'
	@echo "$(COMAKE_MD5)">comake2.md5
	@md5sum -c --status comake2.md5
	@rm -f comake2.md5

.PHONY:ccpclean
ccpclean:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mccpclean[0m']"
	@echo "make ccpclean done"

.PHONY:clean
clean:ccpclean
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mclean[0m']"
	rm -rf deps/libevent/.libs/libevent.a
	test -f deps/libevent/Makefile && make -C ./deps/libevent distclean || true
	rm -rf deps/slb/output/lib/libslb.a
	make -C ./deps/slb clean
	rm -rf proxy
	rm -rf ./output/bin/proxy
	rm -rf proxy_dl.o
	rm -rf proxy_event.o
	rm -rf proxy_funcdef.o
	rm -rf proxy_ip_whitelist.o
	rm -rf proxy_log.o
	rm -rf proxy_main.o
	rm -rf proxy_mem.o
	rm -rf proxy_meta.o
	rm -rf proxy_pool.o
	rm -rf proxy_connection.o
	rm -rf proxy_req.o
	rm -rf proxy_router.o
	rm -rf proxy_server.o
	rm -rf proxy_file.o

.PHONY:dist
dist:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdist[0m']"
	tar czvf output.tar.gz output
	@echo "make dist done"

.PHONY:distclean
distclean:clean
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdistclean[0m']"
	rm -f output.tar.gz
	@echo "make distclean done"

.PHONY:love
love:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mlove[0m']"
	@echo "make love done"

deps/libevent/.libs/libevent.a:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdeps/libevent/.libs/libevent.a[0m']"
	cd ./deps/libevent/; ./configure; make

deps/slb/output/lib/libslb.a:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdeps/slb/output/lib/libslb.a[0m']"
	make -C ./deps/slb

proxy:proxy_dl.o \
  proxy_event.o \
  proxy_funcdef.o \
  proxy_ip_whitelist.o \
  proxy_log.o \
  proxy_main.o \
  proxy_mem.o \
  proxy_meta.o \
  proxy_pool.o \
  proxy_connection.o \
  proxy_req.o \
  proxy_router.o \
  proxy_server.o \
  proxy_file.o \
  ./deps/libevent/.libs/libevent.a \
  ./deps/slb/output/lib/libslb.a
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mproxy[0m']"
	$(CXX) proxy_dl.o \
  proxy_event.o \
  proxy_funcdef.o \
  proxy_ip_whitelist.o \
  proxy_log.o \
  proxy_main.o \
  proxy_mem.o \
  proxy_meta.o \
  proxy_pool.o \
  proxy_connection.o \
  proxy_req.o \
  proxy_router.o \
  proxy_server.o \
  proxy_file.o -Xlinker "-(" ./deps/libevent/.libs/libevent.a \
  ./deps/slb/output/lib/libslb.a ../../../../../lib2-64/bsl/lib/libbsl.a \
  ../../../../../lib2-64/bsl/lib/libbsl_ResourcePool.a \
  ../../../../../lib2-64/bsl/lib/libbsl_archive.a \
  ../../../../../lib2-64/bsl/lib/libbsl_buffer.a \
  ../../../../../lib2-64/bsl/lib/libbsl_check_cast.a \
  ../../../../../lib2-64/bsl/lib/libbsl_exception.a \
  ../../../../../lib2-64/bsl/lib/libbsl_pool.a \
  ../../../../../lib2-64/bsl/lib/libbsl_utils.a \
  ../../../../../lib2-64/bsl/lib/libbsl_var.a \
  ../../../../../lib2-64/bsl/lib/libbsl_var_implement.a \
  ../../../../../lib2-64/bsl/lib/libbsl_var_utils.a \
  ../../../../../lib2-64/ullib/lib/libullib.a \
  ../../../../../public/configure/libconfig.a \
  ../../../../../public/nshead/libnshead.a \
  ../../../../../public/spreg/libspreg.a \
  ../../../../../third-64/pcre/lib/libpcre.a \
  ../../../../../third-64/pcre/lib/libpcrecpp.a \
  ../../../../../third-64/pcre/lib/libpcreposix.a -lpthread \
  -lcrypto \
  -lrt \
  -ldl \
  -rdynamic -Xlinker "-)" -o proxy
	mkdir -p ./output/bin
	cp -f --link proxy ./output/bin

proxy_dl.o:dl.cpp \
  dl.h \
  log.h \
  proxy.h \
  event.h \
  mem.h \
  store_error.h \
  funcdef.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mproxy_dl.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o proxy_dl.o dl.cpp

proxy_event.o:event.cpp \
  deps/libevent/include/event2/event.h \
  deps/libevent/include/event2/event-config.h \
  deps/libevent/include/event2/util.h \
  deps/libevent/include/event2/buffer.h \
  deps/libevent/include/event2/bufferevent.h \
  deps/libevent/include/event2/bufferevent_struct.h \
  deps/libevent/include/event2/event_struct.h \
  deps/libevent/include/event2/keyvalq_struct.h \
  deps/libevent/include/event2/listener.h \
  event.h \
  log.h \
  hash.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mproxy_event.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o proxy_event.o event.cpp

proxy_funcdef.o:funcdef.cpp \
  proxy.h \
  event.h \
  log.h \
  mem.h \
  store_error.h \
  funcdef.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mproxy_funcdef.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o proxy_funcdef.o funcdef.cpp

proxy_ip_whitelist.o:ip_whitelist.cpp \
  log.h \
  event.h \
  ip_whitelist.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mproxy_ip_whitelist.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o proxy_ip_whitelist.o ip_whitelist.cpp

proxy_log.o:log.cpp \
  log.h \
  event.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mproxy_log.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o proxy_log.o log.cpp

proxy_main.o:main.cpp \
  proxy.h \
  event.h \
  log.h \
  mem.h \
  store_error.h \
  funcdef.h \
  server.h \
  connection.h \
  pool.h \
  deps/slb/output/include/CamelSuperStrategy.h \
  deps/slb/output/include/TbStrategy.h \
  deps/slb/output/include/request_t.h \
  deps/slb/output/include/slb_define.h \
  deps/slb/output/include/server_rank_t.h \
  deps/slb/output/include/proxy_log.h \
  log.h \
  deps/slb/output/include/ChainFilter.h \
  deps/slb/output/include/Filter.h \
  deps/slb/output/include/request_t.h \
  router.h \
  meta.h \
  hash.h \
  dl.h \
  file.h \
  req.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mproxy_main.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o proxy_main.o main.cpp

proxy_mem.o:mem.cpp
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mproxy_mem.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o proxy_mem.o mem.cpp

proxy_meta.o:meta.cpp \
  meta.h \
  proxy.h \
  event.h \
  log.h \
  mem.h \
  store_error.h \
  funcdef.h \
  router.h \
  pool.h \
  server.h \
  connection.h \
  deps/slb/output/include/CamelSuperStrategy.h \
  deps/slb/output/include/TbStrategy.h \
  deps/slb/output/include/request_t.h \
  deps/slb/output/include/slb_define.h \
  deps/slb/output/include/server_rank_t.h \
  deps/slb/output/include/proxy_log.h \
  log.h \
  deps/slb/output/include/ChainFilter.h \
  deps/slb/output/include/Filter.h \
  deps/slb/output/include/request_t.h \
  hash.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mproxy_meta.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o proxy_meta.o meta.cpp

proxy_pool.o:pool.cpp \
  pool.h \
  proxy.h \
  event.h \
  log.h \
  mem.h \
  store_error.h \
  funcdef.h \
  server.h \
  connection.h \
  deps/slb/output/include/CamelSuperStrategy.h \
  deps/slb/output/include/TbStrategy.h \
  deps/slb/output/include/request_t.h \
  deps/slb/output/include/slb_define.h \
  deps/slb/output/include/server_rank_t.h \
  deps/slb/output/include/proxy_log.h \
  log.h \
  deps/slb/output/include/ChainFilter.h \
  deps/slb/output/include/Filter.h \
  deps/slb/output/include/request_t.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mproxy_pool.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o proxy_pool.o pool.cpp

proxy_connection.o:connection.cpp \
  event.h \
  connection.h \
  log.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mproxy_connection.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o proxy_connection.o connection.cpp

proxy_req.o:req.cpp \
  req.h \
  proxy.h \
  event.h \
  log.h \
  mem.h \
  store_error.h \
  funcdef.h \
  server.h \
  connection.h \
  meta.h \
  hash.h \
  router.h \
  pool.h \
  deps/slb/output/include/CamelSuperStrategy.h \
  deps/slb/output/include/TbStrategy.h \
  deps/slb/output/include/request_t.h \
  deps/slb/output/include/slb_define.h \
  deps/slb/output/include/server_rank_t.h \
  deps/slb/output/include/proxy_log.h \
  log.h \
  deps/slb/output/include/ChainFilter.h \
  deps/slb/output/include/Filter.h \
  deps/slb/output/include/request_t.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mproxy_req.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o proxy_req.o req.cpp

proxy_router.o:router.cpp \
  router.h \
  proxy.h \
  event.h \
  log.h \
  mem.h \
  store_error.h \
  funcdef.h \
  pool.h \
  server.h \
  connection.h \
  deps/slb/output/include/CamelSuperStrategy.h \
  deps/slb/output/include/TbStrategy.h \
  deps/slb/output/include/request_t.h \
  deps/slb/output/include/slb_define.h \
  deps/slb/output/include/server_rank_t.h \
  deps/slb/output/include/proxy_log.h \
  log.h \
  deps/slb/output/include/ChainFilter.h \
  deps/slb/output/include/Filter.h \
  deps/slb/output/include/request_t.h \
  meta.h \
  hash.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mproxy_router.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o proxy_router.o router.cpp

proxy_server.o:server.cpp \
  server.h \
  connection.h \
  event.h \
  log.h \
  meta.h \
  proxy.h \
  mem.h \
  store_error.h \
  funcdef.h \
  pool.h \
  deps/slb/output/include/CamelSuperStrategy.h \
  deps/slb/output/include/TbStrategy.h \
  deps/slb/output/include/request_t.h \
  deps/slb/output/include/slb_define.h \
  deps/slb/output/include/server_rank_t.h \
  deps/slb/output/include/proxy_log.h \
  log.h \
  deps/slb/output/include/ChainFilter.h \
  deps/slb/output/include/Filter.h \
  deps/slb/output/include/request_t.h \
  req.h \
  ip_whitelist.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mproxy_server.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o proxy_server.o server.cpp

proxy_file.o:file.cpp \
  file.h \
  log.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mproxy_file.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o proxy_file.o file.cpp

endif #ifeq ($(shell uname -m),x86_64)


