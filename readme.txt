mediasmartserverd
=================

Recently I installed Ubuntu on my HP MediaSmart Server 485 and got annoyed
with the blinking health LED. So here's a small Linux daemon that stops
that, takes control of the LEDs, decreases the brightness, and monitors for
disk changes using udev.


I have only tested this on my single HP MediaSmart 485 server. I have _not_
tested it on any other hardware let alone different variations.

Basically USE AT YOUR OWN RISK.


- Chris (mediasmartserverd@adapt.gen.nz)

-----------------------------------------------------------------------------

mediasmartserverd [-D] [-v] [-V] [--brightness <level>]



-D, --daemon
              Causes process to operate in the background as a daemon.

-V
              Prints the program version number.

--brightness <level>
              Controls the LED brightness level.
              Where level is 0 (off) to 10 (full).


-----------------------------------------------------------------------------

Compiling under ubuntu requires g++, libstdc++-dev, libudev-dev, and make

# compile
$ make


# query help
$ ./mediasmartserverd --help


# run as a daemon in the background
$ sudo ./mediasmartserverd -D


On my server I have this executable living in /opt/mediasmartserverd
and then being invoked on startup by rc.local

# start our monitor daemon
/opt/mediasmartserverd -D


-----------------------------------------------------------------------------

Installing your favourite Linux distribution on the HP MediaSmart ex485:

i) Either create or buy a VGA cable and attach it. While it is possible to
install Linux without this cable, having one makes things soooo much easier.

 http://www.mediasmartserver.net/forums/viewtopic.php?f=6&t=3980

ii) Clear out a USB stick and go download UNetbootin

 http://unetbootin.sourceforge.net/

Which allows you to create a bootable USB drive of your favourite Linux
distribution. It will even go and download a distribution for you, if you
don't have an ISO handy.

I used the 64 bit version of Ubuntu 9.10 aka ubuntu-9.10-server-amd64.iso

iii) Run unetbootin and follow the instructions to create a bootable USB drive

iv) Insert the USB drive into the _bottom_ USB port on the _rear_ of your
server. Only the bottom port is initialized by the BIOS and is the only port
that will it boot from. This can cause difficulty as you won't be able to use
a keyboard until an operating system has initialised the other USB ports. In
short you will not be able to play with grub options and will have to wait
for the time out.

v) Install Linux!


-----------------------------------------------------------------------------

On the ex485 all of the LED brightness, fan control, voltage sensors, etc
are hooked up to an SMSC SCH5127-NW chip.

Super I/O with Temperature Sensing, Auto Fan Control and Glue Logic

http://www.smsc.com/index.php?tid=249&pid=160


Actual LEDs are controlled via the General Purpose I/O of the ICH9 chip.
Intel I/O Controller Hub 9 (ICH9)


As I haven't had much experience communicating to hardware devices directly,
and I was somewhat inspired by reverse engineering posts by ymoc & cakalapti.
I ended up partially reverse engineering the Windows driver WNAS.sys. This
turned out to be quite beneficial as documentation isn't easily available for
the SCH5127 chip.


LEDs are numbered from BOTTOM (1) to TOP (4) to match the SCSI bay assignments.
