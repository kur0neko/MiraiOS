![alt text](pic/interface1.png)
![alt text](pic/IMG_6592.jpeg)
# Mirai OS build instruction
#### FYI Pi-Gen had been modified to support a custom kernel for Raspberry Pi CM4+Clockwork Pi motherboard or U-console.
#### Original Fork of Pi-Gen from Raspberry Pi: [Official Link](https://github.com/RPi-Distro/pi-gen), [Official Document about Kernel from Raspberry Pi](https://www.raspberrypi.com/documentation/computers/linux_kernel.html) 
#### Original Fork from U-console CM4 OS-image: [Support Link](https://github.com/cuu/pi-gen/tree/uconsole_arm64)
#### Information about custom Kernel for CM4+Clockwork Pi: [Support Link](https://github.com/cuu/ClockworkPi-linux/commit/9a1e3adc9d1431889f62e633ae791bfc9a6cf535), [Build Custom Kernel](https://github.com/clockworkpi/uConsole/tree/master/Code/patch/cm4/20230630)
#### Old Kernel Official from U-console CM4 board provides driver for all hardware pheripherals on Clockwork Pi v3.14: [Download Link](https://github.com/clockworkpi/apt/blob/main/debian/pool/main/u/uconsole-kernel-cm4-rpi/uconsole-kernel-cm4-rpi_0.13_arm64.deb)
#### Official Guideline on building official image from Clockwork Pi: [Support Link](https://github.com/clockworkpi/uConsole/wiki/How-uConsole-CM4-OS-image-made)
- On terminal (example of Ubuntu)
- ```sudo apt install -y coreutils quilt parted qemu-user-static debootstrap zerofree zip dosfstools libarchive-tools libcap2-bin grep rsync xz-utils file git curl bc qemu-utils kpartx gpg pigz ```
- In order to build OS image, first navigate inside pi-gen directory by doing ``` cd pi-gen/ ```
- Then, run .sh script on terminal under superuser privilege  by doing ``` sudo ./build.sh ```
- Output of OS will be on deploy floder
- FYI it is important to clean up every existing image before re-build a new one by doing  ``` sudo ./clean.sh ```
![alt text](pic/A.png)
![alt text](pic/E.png)
![alt text](pic/F.png)
