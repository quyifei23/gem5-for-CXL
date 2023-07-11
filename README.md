# gem5-for-CXL
this is a repository based on gem5 and aims to be modified for CXL

## Environment

+ Ubuntu 22.04



## IDE

+ VScode



## Build

You need to install the required dependencies with following command.

```
sudo apt install build-essential git m4 scons zlib1g zlib1g-dev libprotobuf-dev protobuf-compiler libprotoc-dev libgoogle-perftools-dev python-dev python
```

Some may fail to be installed because your tools versions(such as g++ etc) is higher than it need to be. Once you have the least high version,these won't influence your build.



After installing the dependencies, you can ues the following command to build.

```
scons build/X86/gem.opt -j <your cores + 1>
```



If you face the problem like this:

```
Error: Can't find version of M4 macro processor.  Please install M4 and try again.
```

use

```
sudo apt-get install automake
```

to solve.



It may may have some warnings, but it is ok to run.



The executable file should be build/X86/gem5.opt
