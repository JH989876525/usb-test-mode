# Copyright (c) 2023 innodisk Crop.
# 
# This software is released under the MIT License.
# https://opensource.org/licenses/MIT

PKG_CONFIG = ${OECORE_NATIVE_SYSROOT}/usr/bin/pkg-config
TARGRT = USB_TEST_MODE
OBJ = ${TARGRT}.o

CFLAGS += -O2 `${PKG_CONFIG} --cflags libusb-1.0`
LDFLAGS += -Wl,--hash-style=both `${PKG_CONFIG} --libs libusb-1.0`

${TARGRT}: $(OBJ)
	$(CC) $(OBJ) -o ${TARGRT} $(LDFLAGS)

${TARGRT}.o: ${TARGRT}.c
	$(CC) $(CFLAGS) -c ${TARGRT}.c

clean:
	rm -f *.o ${TARGRT}
