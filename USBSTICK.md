You can create a bootable USB stick with the following steps.

1. Convert the `image.iso` file to an .img 

	hdiutil convert -format UDRO -o converted.img image.iso

2. Find out which drive the USB stick is with

	diskutil list
	
3. Unmount the disk
	
	diskutil unmountDisk /dev/diskN
	
4. Write the image to the stick

	sudo dd if=converted.img of=/dev/rdiskN bs=1m

5. Unmount it again
	
	diskutil unmountDisk /dev/diskN