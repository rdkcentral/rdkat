##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
CURRENTPATH = `pwd`
all: librdkat.so

VPATH=linux

SEARCH=\
  -I. \
  -I$(SYSROOT_INCLUDES_DIR) \
  -I$(SYSROOT_INCLUDES_DIR)/atk-1.0 \
  -I$(SYSROOT_INCLUDES_DIR)/glib-2.0 \
  -I$(SYSROOT_LIBS_DIR)/glib-2.0/include

EXTRA_CXXFLAGS += -Wno-attributes -Wall -g -fpermissive $(SEARCH) -std=c++1y -fPIC
EXTRA_LDFLAGS = -lglib-2.0 -latk-1.0 -lTTSClient -Wl,-rpath=../../,-rpath=./

ifdef ENABLE_RDK_LOGGER
EXTRA_CXXFLAGS += -DUSE_RDK_LOGGER
EXTRA_LDFLAGS += -lrdkloggers -llog4c
endif

OBJDIR=obj
includes = $(wildcard ./*.h)

$(OBJDIR)/%.o : ./%.cpp ${includes}
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(CXX) -c $(CXXFLAGS) $(EXTRA_CXXFLAGS) $< -o $@

rdkat_SRCS=rdkat.cpp logger.cpp
rdkat_OBJS=$(patsubst %.cpp, $(OBJDIR)/%.o, $(notdir $(rdkat_SRCS)))
rdkat_OBJS:=$(patsubst %.c, $(OBJDIR)/%.o, $(rdkat_OBJS))
rdkat_OBJS: $(rdkat_SRCS)
rdkat_OBJS_ALL=$(rdkat_OBJS)
librdkat.so: $(rdkat_OBJS_ALL)
	$(CXX) $(rdkat_OBJS_ALL) $(EXTRA_LDFLAGS) -shared -fPIC -o librdkat.so

install:
	@mkdir -p ${INSTALL_PATH}/usr/lib/
	@cp -f librdkat.so ${INSTALL_PATH}/usr/lib/
	-ln -s librdkat.so ${INSTALL_PATH}/usr/lib/librdkat.so.0
	-ln -s librdkat.so ${INSTALL_PATH}/usr/lib/librdkat.so.0.0
	
	@mkdir -p ${INSTALL_PATH}/usr/include/
	@cp -f rdkat.h ${INSTALL_PATH}/usr/include

clean:
	@rm -rf obj/* librdkat.so*
