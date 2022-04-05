#!/bin/bash

cd build || exit 1 

rm disk.img

drive_sectors=16k	#Number of sectors of emulated drive.
bytes_per_sector=512	#Number of bytes per sector.
kernel_size=$(ls -l | grep kernel.bin | awk '{print $5}')	#Kernel file size in bytes.

#Calculate kernel size in sectors.
modulus=$(expr $kernel_size % $bytes_per_sector)

if [ $modulus == 0 ]
then
	kernel_sectors=$(expr $kernel_size / $bytes_per_sector)
else
	kernel_sectors=$(expr $kernel_size / $bytes_per_sector + 1)
fi

echo "Number of kernel sectors = $kernel_sectors"

###	Disk image creation	###
#Create an empty emulated disk with 'drive_sectors' sectors.
dd if=/dev/zero of=disk.img bs=$drive_sectors count=$bytes_per_sector
#Fill the MBR sector or the emulated disk.
dd if=loader.bin of=disk.img bs=1 count=$bytes_per_sector seek=0 conv=notrunc
#Fill the sectors after MBR sector with kernel image.
dd if=kernel.bin of=disk.img bs=1 count=$kernel_size seek=512 conv=notrunc

#Offset 0. Status: bit 7 set is for active or bootable.
printf '\x80' | dd of=disk.img bs=1 seek=446 count=1 conv=notrunc

#Offset 4. Partition type: Hard coded to 0x20 in our system.
printf '\x20' | dd of=disk.img bs=1 seek=450 count=1 conv=notrunc

#Offset 8. First absolute sector in the patition. Right after the MBR sector (sector 0)in our case, i.e., 1.
printf '\x01' | dd of=disk.img bs=1 seek=454 count=1 conv=notrunc

#Offset 12. Number of sectors in partition.
printf "0: %.8x" $kernel_sectors | sed -r 's/0: (..)(..)(..)(..)/0: \4\3\2\1/' | xxd -r -g0 | dd of=disk.img bs=1 seek=458 count=4 conv=notrunc

cd ..

	##### Legacy IDE hard drive
	# -drive file=build/disk.img,format=raw,index=0,media=disk \

	##### SATA hard drive
	# -drive id=sata,file=build/disk.img,if=none,format=raw \
	# -device ahci,id=ahci \
	# -device ide-hd,drive=sata,bus=ahci.0 \

	##### NVMe hard drive
	# -drive id=nvme,file=build/disk.img,if=none,format=raw \
	# -device nvme,serial=deadbeef,drive=nvme \

	##### USB stick
	# -drive id=usbstick,file=build/disk.img,if=none \
	# -device usb-ehci,id=ehci \
	# -device usb-storage,drive=usbstick,bus=ehci.0 \

qemu-system-i386 \
	-device isa-debug-exit \
	-drive file=build/disk.img,format=raw,index=0,media=disk \
	-drive id=sata,file=build/kernel.bin,if=none,format=raw \
	-device ahci,id=ahci \
	-device ide-hd,drive=sata,bus=ahci.0 \
	-m 128M \
	-serial stdio \
	-net none
