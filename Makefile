include config.mk

VERSION  ?= 1.2.4
export VERSION
BOARD_REVISION ?= 3
export BOARD_REVISION

# Set to true at build time to enable nileswan branding
NILESWAN_BRANDING ?= 0
export NILESWAN_BRANDING

DISTDIR  ?= out/dist
EMUDIR   ?= out/emulator
MFGDIR   ?= out/manufacturing
MANIFEST ?= manifest/user_update.txt
MANIFEST_FULL ?= manifest/full_update.txt
FULLUPWS := $(DISTDIR)/nileswan-fw-reflash.ws
UPDATEWS := $(DISTDIR)/nileswan-fw-update.ws
FLASHBIN := $(MFGDIR)/spi.rev$(BOARD_REVISION).bin
EMUIPL0  := $(EMUDIR)/nileswan.ipl0
EMUSPI   := $(EMUDIR)/nileswan.spi
EMUIMG   := $(EMUDIR)/nileswan.img
MCUBIN   := $(DISTDIR)/NILESWAN/MCU.BIN
EMUIMG_SIZE_MB ?= 512

FIRMWARE_REV9_RAW_BIN := firmware/build_rev9/firmware.bin
FIRMWARE_REV9A_RAW_BIN := firmware/build_rev9a/firmware.bin
FIRMWARE_RAW_BIN_RECOVERY := software/userland/cbin/recovery/firmware.bin

.PHONY: all dist dist-mfg dist-emu clean distclean help firmware program-fpga program libnile-clean libnile libnile-ipl1 ipl0-clean ipl0 ipl1-clean ipl1 ipl1-factory ipl1-safe recovery-clean recovery updater-clean updater fpga-clean fpga fpga-rev6 fpga-rev6-factory fpga-rev7 fpga-rev7-factory fpga-rev8 fpga-rev8-factory fpga-rev9 fpga-rev9-factory

all: dist dist-mfg

dist: $(FULLUPWS) $(UPDATEWS)

dist-mfg: $(FLASHBIN)

dist-emu: $(EMUIPL0) $(EMUSPI) $(EMUIMG)

help:
	@echo "nileswan build system"
	@echo ""
	@echo "all              Build all user/manufacturing components (default)"
	@echo "  dist           Build user distributables, stored in $(DISTDIR)"
	@echo "  dist-mfg       Build manufacturing files, stored in $(MFGDIR)"
	@echo "dist-emu         Build emulation package, stored in $(EMUDIR)"
	@echo "                 (requires dd, dosfstools, mtools)"
	@echo "clean            Delete build intermediates"
	@echo "distclean        Delete build intermediates and outputs"
	@echo "program-fpga     Build and program initial FPGA bitstream"
	@echo "program          Build and program complete SPI flash contents"

$(EMUIPL0): ipl0
	@mkdir -p $(@D)
	cp software/ipl0/ipl0.bin $@

$(EMUSPI): $(FLASHBIN)
	@mkdir -p $(@D)
	cp $(FLASHBIN) $@

$(EMUIMG):
	@mkdir -p $(@D)
	dd if=/dev/zero of="$@" bs=1M count=$(EMUIMG_SIZE_MB)
	mkfs.vfat "$@"

firmware: $(FIRMWARE_REV9_RAW_BIN) $(FIRMWARE_REV9A_RAW_BIN) $(MCUBIN)

$(MCUBIN): $(FIRMWARE_REV9A_RAW_BIN)
	@mkdir -p $(@D)
	python3 firmware/headerize.py $< $@

$(FIRMWARE_REV9_RAW_BIN): firmware/build_rev9/build.ninja
	cd firmware/build_rev9 && ninja

$(FIRMWARE_REV9A_RAW_BIN): firmware/build_rev9a/build.ninja
	cd firmware/build_rev9a && ninja

firmware/build_rev9/build.ninja:
	-mkdir firmware/build_rev9
	cd firmware/build_rev9 && cmake -G Ninja -DBOARD_REVISION=rev9 ..

firmware/build_rev9a/build.ninja:
	-mkdir firmware/build_rev9a
	cd firmware/build_rev9a && cmake -G Ninja -DBOARD_REVISION=rev9a ..

libnile:
	cd software/libnile && make TARGET=wswan/medium DEFINES="-DLIBNILE_ENABLE_FAST_ALIGNED_READS" && make -j1 TARGET=wswan/medium install

libnile-ipl1:
	cd software/libnile && make TARGET=ipl1 && make -j1 TARGET=ipl1 install

ipl0:
	cd software/ipl0 && make

ipl1: libnile-ipl1
	cd software/ipl1 && make PROGRAM=boot

ipl1-factory: libnile-ipl1
	cd software/ipl1 && make PROGRAM=factory

ipl1-safe: libnile-ipl1
	cd software/ipl1 && make PROGRAM=safe

$(FIRMWARE_RAW_BIN_RECOVERY): $(FIRMWARE_REV9A_RAW_BIN)
	@mkdir -p $(@D)
	cp $< $@

recovery: libnile $(FIRMWARE_RAW_BIN_RECOVERY)
	cd software/userland && make PROGRAM=recovery
	cd software/userland && make PROGRAM=recovery SUFFIX=_factory

updater: libnile
	cd software/userland && make PROGRAM=updater

$(FLASHBIN): fpga ipl1 ipl1-factory ipl1-safe recovery $(MANIFEST_FULL) software/userland/manifest_to_bin.py
	@mkdir -p $(@D)
	python3 software/userland/manifest_to_bin.py $(MANIFEST_FULL) $@ --version $(VERSION) --board-revision $(BOARD_REVISION)

$(FULLUPWS): fpga firmware ipl1 ipl1-factory ipl1-safe recovery updater $(MANIFEST_FULL) software/userland/manifest_to_rom.py
	@mkdir -p $(@D)
	python3 software/userland/manifest_to_rom.py software/userland/updater.wsc $(MANIFEST_FULL) $@ --version $(VERSION)

$(UPDATEWS): fpga firmware ipl1 recovery updater $(MANIFEST) software/userland/manifest_to_rom.py
	@mkdir -p $(@D)
	python3 software/userland/manifest_to_rom.py software/userland/updater.wsc $(MANIFEST) $@ --version $(VERSION)

fpga: fpga-rev6 fpga-rev6-factory fpga-rev7 fpga-rev7-factory fpga-rev8 fpga-rev8-factory fpga-rev9 fpga-rev9-factory

fpga-rev6: ipl0
	cd fpga && make BOARD_REV=rev6

fpga-rev6-factory: ipl0
	cd fpga && make BOARD_REV=rev6 FACTORY=1

fpga-rev7: ipl0
	cd fpga && make BOARD_REV=rev7

fpga-rev7-factory: ipl0
	cd fpga && make BOARD_REV=rev7 FACTORY=1

fpga-rev8: ipl0
	cd fpga && make BOARD_REV=rev8

fpga-rev8-factory: ipl0
	cd fpga && make BOARD_REV=rev8 FACTORY=1

fpga-rev9: ipl0
	cd fpga && make BOARD_REV=rev9

fpga-rev9-factory: ipl0
	cd fpga && make BOARD_REV=rev9 FACTORY=1

program-fpga: fpga
	cd fpga && make program

program: $(FLASHBIN)
	iceprog -p $<

ipl0-clean:
	cd software/ipl0 && make clean

ipl1-clean:
	cd software/ipl1 && make PROGRAM=boot clean
	cd software/ipl1 && make PROGRAM=factory clean
	cd software/ipl1 && make PROGRAM=safe clean

recovery-clean:
	cd software/userland && make PROGRAM=recovery clean
	cd software/userland && make PROGRAM=recovery SUFFIX=_factory clean
	-rm $(FIRMWARE_RAW_BIN_RECOVERY)

updater-clean:
	cd software/userland && make PROGRAM=updater clean

libnile-clean: ipl1-clean recovery-clean updater-clean
	-cd software/libnile && rm -rf build
	-cd software/libnile && rm -rf dist

fpga-clean:
	cd fpga && make BOARD_REV=rev6 clean
	cd fpga && make BOARD_REV=rev7 clean
	cd fpga && make BOARD_REV=rev8 clean
	cd fpga && make BOARD_REV=rev9 clean

clean: fpga-clean ipl0-clean libnile-clean
	-cd firmware/build_rev9 && ninja clean
	-rm -rf firmware/build_rev9/build.ninja
	-cd firmware/build_rev9a && ninja clean
	-rm -rf firmware/build_rev9a/build.ninja

distclean: clean
	rm -rf out
