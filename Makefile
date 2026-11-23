# ─────────────────────────────────────────────────────────────
#  NEBULA HUB — Makefile
#  Designed to run INSIDE the official OpenOrbis Docker container:
#    docker run --rm -w /workspace -v "$(pwd)":/workspace \
#      openorbisofficial/toolchain:latest make
#
#  Inside that container, OO_PS4_TOOLCHAIN is already set and
#  clang/clang++/lld are already on PATH with the correct PS4
#  musl libc + libc++ + SDL2 + PS4 stub libraries pre-built.
# ─────────────────────────────────────────────────────────────

TARGET      := x86_64-pc-freebsd12-elf
OO          := $(OO_PS4_TOOLCHAIN)

CXX         := clang++
LD          := clang++

CXXFLAGS := \
    --target=$(TARGET) \
    -fPIC \
    -funwind-tables \
    -std=c++17 \
    -O2 \
    -D__ORBIS__ \
    -isysroot $(OO) \
    -isystem $(OO)/include \
    -Isrc

LDFLAGS := \
    --target=$(TARGET) \
    -fuse-ld=lld \
    -isysroot $(OO) \
    -Wl,--eh-frame-hdr \
    -Wl,-pie \
    -Wl,-z,max-page-size=0x4000 \
    -Wl,-z,relro \
    -Wl,--script=$(OO)/link.x \
    -L$(OO)/lib

LIBS := \
    -lSDL2_stub \
    -lSDL2_image_stub \
    -lSDL2_ttf_stub \
    -lSceNet_stub \
    -lSceHttp_stub \
    -lSceSsl_stub \
    -lScePad_stub \
    -lSceUserService_stub \
    -lSceSystemService_stub \
    -lSceVideoOut_stub \
    -lScePthread_stub \
    -lkernel_stub \
    -lc \
    -lc++ \
    -lc++abi

SRCS := \
    src/main.cpp \
    src/db/database.cpp \
    src/net/fetcher.cpp \
    src/input/controller.cpp \
    src/install/installer.cpp \
    src/ui/renderer.cpp \
    src/ui/screens.cpp

OBJS := $(SRCS:.cpp=.o)

.PHONY: all clean eboot fself gp4 pkg

all: eboot.bin project.gp4

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

eboot.elf: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

eboot.bin: eboot.elf
	$(OO)/bin/linux/create-fself \
	    -in eboot.elf \
	    -out eboot.bin \
	    --paid 0x3800000000000011 \
	    --npdrmtype fake \
	    --app \
	    --authinfo 0100000000000000F0000000FFD40000000000000000000000000000000000000000000000000000000800000000000000380000000000008000003800000000000000000000008000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000

project.gp4: eboot.bin
	$(OO)/bin/linux/create-gp4 \
	    -out project.gp4 \
	    --content-id "NEBU00001-NEBU00001_00-NEBULAHUB0000001" \
	    --app-path . \
	    --title-id "NEBU00001" \
	    --version "01.00"

clean:
	rm -f $(OBJS) eboot.elf eboot.bin project.gp4
