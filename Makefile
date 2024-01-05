EFIHEADERS = /usr/include/efi
EFILIBS = /usr/lib

CFLAGS = -I$(EFIHEADERS) -fpic -ffreestanding -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args
LDFLAGS = -shared -Bsymbolic -L$(EFILIBS) -T$(EFILIBS)/elf_x86_64_efi.lds

BOOTX64.efi: main.so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 $^ $@

main.so: main.o
	ld $(LDFLAGS) /usr/lib/crt0-efi-x86_64.o $^ -o $@ -lgnuefi -lefi

main.o: main.c
	gcc $(CFLAGS) -c $^ -o $@

clean: 
	rm -f *.o *.so

build: BOOTX64.efi
	sudo losetup -f disk.hdd
	sudo mount /dev/loop0 mnt
	sudo mv $< mnt/EFI/Boot/$<
	sudo umount mnt
	sudo losetup -d /dev/loop0
	cp disk.hdd /mnt/c/disks/disk.hdd
	make clean

setup:
	sudo apt update && sudo apt upgrade -y
	sudo apt install gcc gnu-efi dosfstools gdisk