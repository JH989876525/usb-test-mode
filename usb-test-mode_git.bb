SUMMARY = "Simple tool to setup USB test mode"
DESCRIPTION = "A simple tool to setup USB test mode for USB2.0 and USB3.0 compliance test using libusb."
HOMEPAGE = "https://github.com/JH989876525/USB_TEST_MODE"
BUGTRACKER = "https://github.com/JH989876525/USB_TEST_MODE/issues"
SECTION = "utils"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=39c0045a7d20c7472371214084c1938c"

DEPENDS = "libusb make"

PV = "1.0+git${SRCPV}"
SRC_URI = "git://git@github.com/JH989876525/USB_TEST_MODE.git;branch=main;protocol=ssh"
SRCREV = "23746ef5d8684874f3c7afd7c7d2cf0e7e1d8e7b"

S = "${WORKDIR}/git"

CVE_PRODUCT = "usb_test_mode"

do_configure() {
    echo "No configure step needed for USB_TEST_MODE"
}

do_install() {
    install -d "${D}${bindir}"
    install -m 0755 "${S}/USB_TEST_MODE" "${D}${bindir}"
}

RDEPENDS:${PN} += "bash perl"

BBCLASSEXTEND = "native nativesdk"
