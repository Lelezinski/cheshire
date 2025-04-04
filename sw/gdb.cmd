target extended-remote localhost:3333
set remotetimeout unlimited
file /home/zcu102/git/cheshire/sw/tests/chessy_packet.spm.elf
load
set architecture riscv:rv64
monitor arm semihosting enable
break semihost_break