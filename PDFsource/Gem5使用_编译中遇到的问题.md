## Gem5使用/编译中遇到的问题

### 1.  error: no declaration matches "gem5::xxx* xxxxx::create() const"

#### create()函数的自动生成 （）

​	根据官网的教程[Creating a very simple SimObject](https://www.gem5.org/documentation/learning_gem5/part2/helloobject/)我们知道在C++文件中的.cc文件中要完成两个函数，一个是相应的构造函数，另一个是create()函数。create()函数的功能是调用SImObject()的构造函数，并且返回一个实例化的SimObject，由于我们创建的SimObject是以SimObject为基类的，create()函数在Gem5中是必不可少的。

​	但是如果你的构造函数满足一下的形式：

```
Foo(const FooParams &)
```

​	那么相应的FooParams::create()就会自动的实现，不需要你再去编写。

​	但是如果你的构造函数是按照这个形式来写的，然而发现在编译中仍然发现如图所示的错误：

![image-20230723102551530](./images/image-20230723102551530.png)

​	会发现报错的是有关create()函数，但是这个函数式自动生成的，不应该会出现错误。在对比翻看了build文件夹下自动生成的有关我们自己定义的SimObject文件后，发现缺少了gem5的命名空间，同文件夹下的其他文件中的类都有gem5的命名空间。

​	最终的问题出现在.py文件上，因为在编写cxx_class的过程中就缺少了`gem5::`，导致这个错误。加上后，成功编译。



### 2. warn: no dot file generated. Please install pydot to generate the dot file

其实在之前我使用gem5的时候是生成了.dot文件的，但是当我重置了gem5源码的时候，就无法生成了。

安装pydot显示已经安装。查阅后发现pydot的使用一共有三个依赖：

+ pydot
+ pydotplus
+ Graphviz

发现这三个库都已安装，尝试卸载后，**按照顺序**安装pydot，pydotplus，Graphviz这三个库。重新运行发现仍然没有。

我直接利用github在仓库中搜索pydot，看到了配置文件中的一句话`apt-get install python3-dot`，然后执行了`sudo apt-get install python3-dot`，新安装了python3-dot。然后再次运行，发现生成了.dot文件以及配置图。原因可能是python多版本的问题。



### 3. terminate called after throwing an instance of 'pybind11::error_already_set'
  what():  KeyError: 'cxx_class'

scons: *** [build/X86/params/RootComplex.hh] Error 134

遇到`pybind11::error_already_set`的问题，尤其是在编译刚开始的时候出现`core dumped`，大概率是在SConscript中的编译配置出现了问题，或者就是在python文件,C++文件的链接中出现问题。在本问题中，发现是python文件中定义的类RootComplex出现了问题。在python文件中，定义的RootComplex是继承同文件的PciBridge2，同时并没用将RootComplex申请为新的SimObject，只是一个继承的类而已，所以并没有cxx_header以及cxx_class的设置。但是在SConscript中的sim_objects里却加入了RootComplex，因此

报错。将其去掉再次编译即可通过。



### 4. src/sim/simobject.cc:125: xxx does not have any port named xxx

这里我直接继承了ResponsePort和RequestPort设计了两个最简单的端口（ResponsePort和RequestPort是抽象类，不能直接使用），编译成功通过，但是在配置文件中调用端口的时候出现了这个错误。

原因：没有重写基类Port中的`getPort()`函数。在利用名字（name）调用相应的端口成员时，会调用getPort()函数，而Port本身的getPort()直接就是抛出fatal(“xxx does not have any port named xxx”)，所以要自己重写getPort()函数，在其中加入判断的过程，当访问到该类含有的端口的名字的时候，返回该端口名字对应的端口成员。