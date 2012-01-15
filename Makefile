# Change these to match your source code.
TARGET	= negative.so
OBJECTS	= negative.o

# Change these to match your environment.
GLFS_SRC  = /root/glusterfs_patches
GLFS_ROOT = /opt/glusterfs
GLFS_VERS = 3git
GLFS_LIB  = $(GLFS_ROOT)/$(GLFS_VERS)/lib64
HOST_OS  = GF_LINUX_HOST_OS

# You shouldn't need to change anything below here.

CFLAGS	= -fPIC -Wall -O0 -g \
	  -DHAVE_CONFIG_H -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D$(HOST_OS) \
	  -I$(GLFS_SRC) -I$(GLFS_SRC)/libglusterfs/src \
	  -I$(GLFS_SRC)/contrib/uuid -I.
LDFLAGS	= -shared -nostartfiles -L$(GLFS_LIB) -lglusterfs -lpthread

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

install: $(TARGET)
	cp $(TARGET) $(GLFS_LIB)/glusterfs/$(GLFS_VERS)/xlator/features

clean:
	rm -f $(TARGET) $(OBJECTS)
