CXX=g++
CXXFLAGS=-g \
  -pipe \
  -W \
  -fPIC
CFLAGS=-g \
  -pipe \
  -W \
  -fPIC

INCPATH=-I.. 
DEP_INCPATH=-I../../../../../lib2-64/bsl \
  -I../../../../../../public/configure \
  -I../../../../../../public/configure/include \
  -I../../../../../../public/configure/output \
  -I../../../../../../public/configure/output/include \
  -I../../../../../../lib2-64/bsl \
  -I../../../../../../lib2-64/bsl/include \
  -I../../../../../../lib2-64/bsl/output \
  -I../../../../../../lib2-64/bsl/output/include \
  -I../../../../../../lib2-64/ullib \
  -I../../../../../../lib2-64/ullib/include \
  -I../../../../../../lib2-64/ullib/output \
  -I../../../../../../lib2-64/ullib/output/include \
  -I../../../../../../public/spreg \
  -I../../../../../../public/spreg/include \
  -I../../../../../../public/spreg/output \
  -I../../../../../../public/spreg/output/include 

shardtool:shardtool.o
	$(CXX) shardtool.o -Xlinker "-(" ../../../../../../public/configure/libconfig.a \
	../../../../../../lib2-64/bsl/lib/libbsl.a \
	../../../../../../lib2-64/bsl/lib/libbsl_ResourcePool.a \
	../../../../../../lib2-64/bsl/lib/libbsl_archive.a \
	../../../../../../lib2-64/bsl/lib/libbsl_buffer.a \
	../../../../../../lib2-64/bsl/lib/libbsl_check_cast.a \
      	../../../../../../lib2-64/bsl/lib/libbsl_exception.a \
	../../../../../../lib2-64/bsl/lib/libbsl_pool.a \
	../../../../../../lib2-64/bsl/lib/libbsl_utils.a \
	../../../../../../lib2-64/bsl/lib/libbsl_var.a \
	../../../../../../lib2-64/bsl/lib/libbsl_var_implement.a \
	../../../../../../lib2-64/bsl/lib/libbsl_var_utils.a \
	../../../../../../lib2-64/ullib/lib/libullib.a \
	../../../../../../public/spreg/libspreg.a \
	-lpthread \
	-Xlinker "-)" -o shardtool

shardtool.o:shardtool.cpp 
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CXXFLAGS) -o shardtool.o shardtool.cpp

.PHONY:clean
clean:
	rm *.o shardtool
