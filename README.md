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

Retrieve DevkitPro for PPC Docker image

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

Compile the GRRLIB and GAmeCube examples (already in devkitPro `/opt/devkitpro/examples/gamecube` in fact)

````bash
sudo docker run --rm -it --name devkit -v /projects/gc:/home/gc devkitpro/devkitppc bash
cd /home/gc

cd /home/gc/GRRLIB/GRRLIB
make PLATFORM=cube all

cd /home/gc/gamecube-examples
make

cd /home/gc/GameCube4Fun/examples_gc/template
make
````


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
