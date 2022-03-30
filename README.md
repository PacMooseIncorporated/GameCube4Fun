# GameCube4Fun

## Prerequisites

 - Ubuntu (works with WSL2 too)
 - Dolphin GameCube emulator (https://dolphin-emu.org/)
 - Docker
 - [Optional] Visual Code with extensions (https://code.visualstudio.com/download)
   - WSL remote if using WSL2
   - ...  

## Installation

From your Ubuntu non-root account:

Retrieve DevkitPro for PPC Docker image. This image contains:
 - cross-compliation toolchain for PPC and ARM
 - standard libraries pre-compiled: libz, libturbojpeg, libpng/u, libvorbis, libogg, linmpg123, libfreetype, libode, libgd
 - libogc already compiled which also compiles:
   - libiso9660, libmodplay, libmad, libasnd, libaesnd, libdb, lwip

````bash
sudo docker pull devkitpro/devkitppc
sudo docker images
````

Clone all Git repos

````bash
mkdir -p /projects/gc
cd /projects/gc
git clone https://github.com/devkitPro/gamecube-examples.git
git clone https://github.com/GRRLIB/GRRLIB.git
git clone https://github.com/PacMooseIncorporated/GameCube4Fun.git
````

Compile the GRRLIB, GameCube examples (already in devkitPro `/opt/devkitpro/examples/gamecube` in fact)

````bash
sudo docker run --rm -it --name devkit -v /projects/gc:/home/gc devkitpro/devkitppc bash

cd /home/gc/gamecube-examples
make

cd /home/gc/GRRLIB/GRRLIB
make PLATFORM=cube clean all install

cd /home/gc/GameCube4Fun/examples_gc/template
make
````

## Documentation

 - libogc: https://libogc.devkitpro.org/
 - GRRLIB: https://grrlib.github.io/GRRLIB/
 - Interesting article on the GAmeCube GX and libogc: https://devkitpro.org/wiki/libogc/GX#Preface
 - Forums: 
   - https://devkitpro.org/viewforum.php?f=40
   - http://grrlib.santo.fr/forum/ 

## Installing Docker without Docker Desktop on WSL2

````bash
sudo apt install --no-install-recommends apt-transport-https ca-certificates curl gnupg2
curl -fsSL https://download.docker.com/linux/${ID}/gpg | sudo apt-key add -
echo "deb [arch=amd64] https://download.docker.com/linux/${ID} ${VERSION_CODENAME} stable" | sudo tee /etc/apt/sources.list.d/docker.list
sudo apt update
sudo apt install docker-ce docker-ce-cli containerd.io
sudo usermod -aG docker $USER
sudo docker ps 
````

add to `~/.profile`:

````bash
if service docker status 2>&1 | grep -q "is not running"; then
    wsl.exe -d "${WSL_DISTRO_NAME}" -u root -e /usr/sbin/service docker start >/dev/null 2>&1
fi
````

## GameCube Specs

### CPU

 - 485 MHz IBM "Gekko" PowerPC CPU based on the 750CXe and 750FX achieving 1125 DMIPS (Dhrystone 2.1)
 - 7-stage floating point unit: 64-bit double precision FPU, usable as 2×32-bit SIMD for 1.9 single-precision GFLOPS performance, often found under the denomination "paired singles"
 - Branch Prediction Unit (BPU)
 - Load-Store Unit (LSU)
 - System Register Unit (SRU)
 - Memory Management Unit (MMU)
 - Branch Target Instruction Cache (BTIC)
 - SIMD instructions: PowerPC 750 + roughly 50 new SIMD instructions, geared toward 3D graphics
 - On-chip caches:
   - 32 KB 8-way set-associative L1 instruction cache
   - 32 KB 8-way set-associative L1 data cache
   - 256 KB 2-way set-associative L2 cache
 - Front-side bus: 64-bit enhanced 60x bus to Flipper northbridge at 162 MHz clock with 1.3 GB/s peak bandwidth (32-bit address, 64-bit data bus)

### GPU
 
 - 162 MHz ArtX-designed ATI "Flipper" ASIC (9.4 GFLOPS)
 - Contains GPU, audio DSP, I/O controller and northbridge
 - 3 MB of on-chip 1T-SRAM (2 MB Z-buffer/framebuffer + 1 MB texture cache) with ~18 GB/s total bandwidth
 - 24 MB (2x 12 MB) 1T-SRAM main memory @ 324 MHz, 64-bit bus, 2.6 GB/s bandwidth
 - 1 vertex pipeline, 4 pixel pipelines with 1 texture mapping unit (TMU) each and 4 render output units (ROPs)
 - Color depth: 24-bit RGB, 32-bit RGBA
 - Fillrate: 648 megapixels/sec, with Z-buffering, alpha blending, fogging, texture mapping, trilinear filtering, mipmapping and S3 Texture Compression[7]
 - Raw polygon performance: 90 million polygons/sec
 - Real-time decompression of display list, hardware motion compensation capability, HW 3-line deflickering filter

### Memory

 - 43 MB total non-unified RAM
   - 24 MB (2x 12 MB) MoSys 1T-SRAM @ 324 MHz (codenamed "Splash") as main system RAM
   - 3 MB embedded 1T-SRAM cache within "Flipper" GPU (2 MB framebuffer/Z-buffer, 1 MB texture cache)
   - 16 MB DRAM used as I/O buffer for audio and DVD drive
 - Memory bus width: 64-bit main system RAM, 896-bit internal GPU memory, 8-bit ARAM
 - Memory bandwidth: 1.3 GB/s Gekko to Northbridge, 2.6 GB/s Flipper to main system RAM, 10.4 GB/s texture cache, 7.8 GB/s framebuffer/Z-buffer, 81 MB/s auxiliary RAM[5]
 - Latency: Under 10 ns main memory, 5 ns texture cache, 5 ns framebuffer memory

### Audio

 - Audio processor integrated into Flipper: custom 81 MHz Macronix 16-bit DSP
 - External auxiliary RAM: 16 MB DRAM @ 81 MHz
 - Stereo output (may contain 5.1-channel surround via Dolby Pro Logic II)

### Video

 - 640×480 interlaced (480i) @ 60 Hz
 - 640×480 progressive scan (480p) @ 60 Hz (NTSC only)
 - 768×576 interlaced (576i) @ 50 Hz (PAL only)

