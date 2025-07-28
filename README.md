# BioAuth

FLAME: Flexible and Lightweight Biometric Authentication Scheme in Malicious
Environments


### Dependencies

The code has been tested on macOS 15.3. Building the code requires the following dependencies:

- A C++20 (GCC 10+,Clang 12+)
- CMake (3.12+)
- The Boost Library (1.70.0 or later)
- The Eigen Library (3.0 or later)
- OpenSSL
- GMP
- GMPXX
- pthread


##### on macOS
```shell
brew install cmake eigen boost openssl gmp
```
```shell
#### on Ubuntu
sudo apt-get update
sudo apt-get install build-essential cmake
sudo apt-get install libeigen3-dev libboost-dev
sudo apt-get install libssl-dev libgmp-dev libgmpxx4ldbl
sudo apt-get install libpthread-stubs0-dev
```

### Building the Project


```shell
git clone https://github.com/NemoYuan2008/MD-ML.git
cd BioAuth
mkdir -p build
cd build
cmake..
make -j
```


### Runing Experiments
1. Secure Inner Product for Database

set vector length=1024 and dbsize =512 in experiments/dot-product-db/dot_product_db_configure.h

```shell
cd build/experiments/dot-product-db

# offline
sudo ./dot_product_db_offline_party_0
sudo ./dot_product_db_offline_party_1

# online
sudo ./dot_product_db_party_0
sudo ./dot_product_db_party_1

```
2. Secure Comparison

set DEFAULT_ROUNDS=256 in experiments/secure-com/com_config.h

```shell
cd build/experiments/secure-com

# offline + online
sudo ./com_party_0
sudo ./com_party_1

```
⚠️ Important Notes
Network Simulation Requirements

sudo privileges are required for network traffic control (tc commands)
Network simulation modifies system network settings temporarily
Experiments automatically clean up network configurations after completion