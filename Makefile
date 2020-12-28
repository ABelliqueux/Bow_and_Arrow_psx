TARGET = bow
TYPE = ps-exe

SRCS = bow.c \
../common/crt0/crt0.s \
TIM/bowsht.tim \
TIM/bowbg.tim \
TIM/menu.tim \
TIM/inst.tim \
VAG/vil.vag \
VAG/crowd.vag \
VAG/shoot.vag \
VAG/folks.vag \
VAG/ready.vag \
VAG/pop.vag \
VAG/jmp.vag \
VAG/toc.vag \

CPPFLAGS += -I../psyq/include
LDFLAGS += -L../psyq/lib
LDFLAGS += -Wl,--start-group
LDFLAGS += -lapi
LDFLAGS += -lc
LDFLAGS += -lc2
LDFLAGS += -lcard
LDFLAGS += -lcomb
LDFLAGS += -lds
LDFLAGS += -letc
LDFLAGS += -lgpu
LDFLAGS += -lgs
LDFLAGS += -lgte
LDFLAGS += -lgun
LDFLAGS += -lhmd
LDFLAGS += -lmath
LDFLAGS += -lmcrd
LDFLAGS += -lmcx
LDFLAGS += -lpad
LDFLAGS += -lpress
LDFLAGS += -lsio
LDFLAGS += -lsnd
LDFLAGS += -lspu
LDFLAGS += -ltap
LDFLAGS += -Wl,--end-group

include ../common.mk \


#bowsht.tim:
#~ 	img2tim -org 512 0 -plt 0 960 -bpp 8 -o bowsht.tim bow.png
#~ 	img2tim -org 512 0 -bpp 16 -o bowsht.tim bow.png
#~ bowbg.tim:
#~ 	img2tim -org 512 256  -plt 0 961 -bpp 8 -o bowbg.tim bow-bg.png
#~ 	img2tim -org 512 256 -bpp 16 -o bowbg.tim bow-bg.png


#~ nugget : add binary to psx-exe 

#~ bowsht.o: bowsht.tim
#~ 	$(PREFIX)-objcopy -I binary --set-section-alignment .data=4 --rename-section .data=.rodata,alloc,load,readonly,data,contents -O elf32-tradlittlemips -B mips bowsht.tim bowsht.o

#~ bowbg.o: bowbg.tim
#~ 	$(PREFIX)-objcopy -I binary --set-section-alignment .data=4 --rename-section .data=.rodata,alloc,load,readonly,data,contents -O elf32-tradlittlemips -B mips bowbg.tim bowbg.o
