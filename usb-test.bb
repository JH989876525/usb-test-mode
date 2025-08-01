SUMMARY = " A simple tool to setup USB test mode for USB2.0 and USB3.0 compliance test."
HOMEPAGE = "https://github.com/JH989876525/USB_TEST_MODE"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "git://git@github.com/JH989876525/USB_TEST_MODE.git;branch=main;protocol=ssh"
SRCREV = "8a4067a5a5e0270ab43dec0f6842c443615fdaf9"

S = "${WORKDIR}/git"

DEPENDS = "make libusb"

do_configure() {
	echo "No configure step needed for USB_TEST_MODE"
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 ${S}/USB_TEST_MODE ${D}${bindir}
}

RDEPENDS:${PN} += "bash perl"
