#### Memory acquisition 

```
#inspect the target memory, e.g., the hafnimum's exception vector table. Up to 20MB at a time.
sc_user_client -m -e 0x2 -a 0x6000800 -s 2048 //VA by deafult, or add `-p 1` to use PA access.

#then decrypt the memory dump and save it to a file named `mem_dump`
sc_user_client -o mem -d

#show the content

hexdump -C <file>

```

#### Register 

```
sc_user_client -r

#save the register info to file named registers_info.txt
sc_user_client -o reg

#with decryption
sc_user_client -o reg -d

```

#### Memory traps 



```
#first set the target tarpped memory address, e.g., for insepcting the hafnimum's exception vector table. When a trap occurs, the Monitor currently dumps the target memory and registers by default.

//e.g., watchpoint
sc_user_client -m -e 0x2 -a 0x6000800 -s 2048 -w 0x630df28 

or

//breakpoint
sc_user_client -m -e 0x2 -a 0x6000800 -s 2048 -b 0x62d462c

#run a TA to exectue the TrustZone system, then a trap will be hitted.
tee-supplicant -d
/root/optee_example_hello_world

#decrypt and save dump to file
sc_user_client -o mem -d
sc_user_client -o reg -d

```

#### Instruction tracing

```
#enable ete
sc_user_client -i 1 

#run a TA to exectue the TrustZone system
tee-supplicant -d
/root/optee_example_hello_world

#disable ete and the instruction trace will be saved to a file named ins_dump
sc_user_client -i 2 -d

```

ETE decode. Download `ins_dump` to `<SCRUTINIZER-REPO>/scripts/trace` and rename it to `trace_data.bin`. Run `/scripts/test-ete-decode.sh` to get the decoded ETE trace within `trace_decode.ppl`.