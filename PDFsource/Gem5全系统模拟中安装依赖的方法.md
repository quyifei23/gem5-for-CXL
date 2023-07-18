## Gem5全系统模拟中安装依赖的方法

> 参考：https://luzhixing12345.github.io/gem5/articles/gem5%E7%9A%84%E7%BC%96%E8%AF%91%E5%92%8C%E4%BD%BF%E7%94%A8/%E6%80%A7%E8%83%BD%E6%B5%8B%E8%AF%95%E4%B8%8E%E7%BB%9F%E8%AE%A1%E7%BB%93%E6%9E%9C/
>
> 本文主题参考上述文章，并另外总结一些安装中的遇到的错误，非常感谢上述文章的指导。

由于在全系统模拟中，终端的交互速度太慢，而且甚至可能存在联网配置的问题，所以一般不直接在启动后的终端中操作，而是将依赖在外面装好（利用挂载磁盘镜像），并提前编译。本文以安装stream带宽测试所需的memkind/vmem依赖为例。（考虑到时效性，本文中的所有依赖的版本可能不是最新版本，可自行搜索更新，但是不能保证更新后在安装过程中不出错误）

使用位于full-system-image/disks/parsec.img磁盘镜像，创建目录并挂载VFS。

此时位于parsec.img同级目录。，挂载镜像文件到local_mnt目录

```
mkdir local_mnt
sudo mount -o loop,offset=$((2048*512)) parsec.img local_mnt
sudo mount -o bind /proc local_mnt/proc
sudo mount -o bind /dev local_mnt/dev
sudo mount -o bind /dev/pts local_mnt/dev/pts
sudo mount -o bind /sys local_mnt/sys

cd local_mnt
sudo chroot ./
mkdir /home/lib_source_code
```

此时已经进入了磁盘镜像中，并且发现当前的`/`目录是该磁盘镜像的目录，而不是本地的。这里相当于直接在磁盘镜像中进行操作。

在这里执行一些下载的命令（如apt，wget）会发现是无法进行的（这里可能是镜像没有配置的问题，如网络配置或者DNS配置）。所以如果需要下载的话，我们还是退出到本机目录中下载。方法：ctrl-D退出chroot模式，进入本地目录`cd ..`，然后在本地安装以下依赖

```
wget https://github.com/pmem/pmdk/releases/download/1.4/pmdk-1.4-dpkgs.tar.gz
wget https://github.com/memkind/memkind/archive/refs/tags/v1.14.0.tar.gz
wget http://ftp.jaist.ac.jp/pub/GNU/libtool/libtool-2.4.6.tar.gz
wget https://github.com/numactl/numactl/releases/download/v2.0.16/numactl-2.0.16.tar.gz
```

然后移入之前创建的local_mnt/home/lib_source_code中。

```
sudo mv libtool-2.4.6.tar.gz local_mnt/home/lib_source_code/
sudo mv numactl-2.0.16.tar.gz local_mnt/home/lib_source_code/
sudo mv v1.14.0.tar.gz local_mnt/home/lib_source_code
sudo mv pmdk-1.4-dpkgs.tar.gz local_mnt/home/lib_source_code
```

重新进入磁盘镜像

```
cd local_mnt
sudo chroot ./
cd /home/lib_source_code
```

解压这些下载的文件

```
tar -xf pmdk-1.4-dpkgs.tar.gz
tar -xf v1.14.0.tar.gz
tar -xf libtool-2.4.6.tar.gz
tar -xf numactl-2.0.16.tar.gz
```

安装libvmem

```
dpkg -i libvmem_1.4-1_amd64.deb
dpkg -l | grep libvmem #查看是否安装成功
dpkg -L libvmem #查看安装的位置
```

安装并编译libtool

```
cd libtool-2.4.6
./configure --prefix=/usr/
make
make install
```

> 同文档里面的一样，这里会出错，但是好像不会报出来，但是在后面依赖的编译中会出现找不到这个的情况，再次执行`./configure --prefix=/usr/`，并`make clean`后，重新编译`make && make install`。

安装并编译memkind

```
cd ..
cd memkind-1.14.0
./autogen.sh
# 之前在这个过程中报错error: Libtool library used but 'LIBTOOL' is undefined
# 原因就是libtool没用正确安装

./configure --prefix=/usr/
make
make install
ls /usr/lib/ | grep kind #查看是否安装成功
```

在所有依赖都成功安装后，卸载镜像

```
# ctrl-D
cd ..
sudo umount local_mnt/proc
sudo umount local_mnt/dev/pts
sudo umount local_mnt/dev
sudo umount local_mnt/sys
sudo umount local_mnt
```

目前，依赖成功安装进入磁盘镜像中，但是进入模拟系统后（利用checkpoint进入）会发现仍然找不到这些依赖（利用`ls /usr/lib/ | grep kind`）。而相较于之前的在磁盘镜像中的home文件夹下操作，进入交互系统可以看到，可能适用于checkpoint保存的快照不包含系统文件（猜测），所以应该重新完整进入系统（证明是有效的）。之后重新建立checkpoint，注意现在包含两个checkpoint，因此在后面的利用checkpoint进入时应改为`-r 2`。