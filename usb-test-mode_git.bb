SUMMARY = "Simple tool to setup USB test mode"
DESCRIPTION = "A simple tool to setup USB test mode for USB2.0 and USB3.0 compliance test using libusb."
HOMEPAGE = "https://github.com/JH989876525/usb-test-mode"
BUGTRACKER = "https://github.com/JH989876525/usb-test-mode/issues"
SECTION = "utils"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=39c0045a7d20c7472371214084c1938c"

DEPENDS = "libusb make"

PV = "1.0+git${SRCPV}"
SRC_URI = "git://git@github.com/JH989876525/usb-test-mode.git;branch=main;protocol=ssh"
SRCREV = "70fd1c0094c606ba1965994a84c0b0a98217ef68"

UNPACKDIR ??= "${WORKDIR}"
S = "${UNPACKDIR}/git"

CVE_PRODUCT = "usb_test_mode"

do_configure() {
    echo "No configure step needed for usb-test-mode"
}

do_install() {
    install -d "${D}${bindir}"
    install -m 0755 "${S}/usb-test-mode" "${D}${bindir}"
}

RDEPENDS:${PN} += "bash perl"

BBCLASSEXTEND = "native nativesdk"
