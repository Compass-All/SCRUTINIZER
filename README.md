
# SCRUTINIZER

The code is tested on Ubuntu 22.04 x86 and aarch64 with minimum 16GB memory.

The system is based on FVP, and the software running in four worlds is as follows:

Linux (Normal-EL1), tf-rmm (Realm-EL2), Hafnium (S-EL2), OPTEE(S-EL1), and tf-a (Root-EL3).

## 1. Setup

Run `scripts/env.sh all` to sync the software stacks.


## 2. Build

Run `scripts/build.sh all` to build the components.

or Run `scripts/build.sh <target>` to build a specific component.

## 3. Test SCRUTINIZER 

### Lanuch FVP

Run `./scripts/bootfvp.sh` to lanuch the FVP.

### Client

For ease of testing, there is a local client that can invoke the functionalities of SCRUTINIZER Monitor.

#### Compile and upload to FVP
```shell
cd src/sc_client/
make
scp ./sc_user_client root@192.168.122.33:~/

cd src/sc_client/driver
make
scp ./sc_manager.ko root@192.168.122.33:~/
```

#### Usage
In FVP linux terminal, load the kernel module: 

```
 insmod sc_manager.ko
```

Run the client to test. More details in [client.md](https://www.example.com).

```
sc_user_client -h
```

## Publication

```
@inproceedings{zhang2025scrutinizer,
  title={SCRUTINIZER: Towards Secure Forensics on Compromised TrustZone},
  author={Zhang, Yiming and Zhang, Fengwei and Luo, Xiapu and Hou, Rui and Ding, Xuhua and Liang, Zhenkai and Yan, Shoumeng and Wei, Tao and He, Zhengyu},
  booktitle={32nd Network and Distributed System Security Symposium (NDSS’25)},
  year={2025}
}
```