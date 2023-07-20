## Gem5中python文件与C++文件之间的关系

> 参考：
>
> [Gem5学习1 - Gem5及其文件结构](https://blog.csdn.net/drinks_/article/details/117065147)
>
> [Simple parameters](https://www.gem5.org/documentation/learning_gem5/part2/parameters/)

+ Gem5所有主要的仿真单元都成为SimObject（或者说以此为基类），它们使用相同的方式进行配置，初始化，统计与序列化
+ SimObject包括具体的硬件单元模型比如处理器核，cache，互联单元与设备，也包括更为抽象的比如负载和与之相关联的用于系统调用仿真的处理内容。
+ 每个SimObject由两个类表示，一个Python类，一个C++类。Python类定义SimObject参数并进行给予脚本的配置。C++类包含了SimObject状态与剩余的行为，包括关键性能的仿真模型。
+ 对于不同的组件SimObject，Gem5中使用Python进行集成，Python负责初始化，配置和模拟控制。仿真器一开始就立即执行python代码。



#### 在Python中添加参数

在Python文件下，在SimObject类下添加参数：

```
class HelloObject(SimObject):
    type = 'HelloObject'
    cxx_header = "learning_gem5/part2/hello_object.hh"

    time_to_wait = Param.Latency("Time before firing the event")
    number_of_fires = Param.Int(1, "Number of times to fire the event before "
                                   "goodbye")
```

这里添加了一个类型为`Latency`名称为`time_to_wait`的参数和一个类型为Int名称为`number_of_fires`的参数。格式为`<name> = Param.<TypeName>`。一般在定义这个参数的时候会有两个参数，如果前一个为这个参数的默认值，后一个为对这个参数简单的描述（如果只有一个参数，则是简单的描述）。

常用的参数还有`Percent,Cycles,MemorySize`等等。

当在python文件中声明了这些参数以后，需要讲这些参数加入到C++文件中的构造函数中，通过构造函数的`params`来获取：

```
HelloObject::HelloObject(const HelloObjectParams &params) :
    SimObject(params),
    event(*this),
    myName(params.name),
    latency(params.time_to_wait),
    timesLeft(params.number_of_fires)
{
    DPRINTF(Hello, "Created the hello object with the name %s\n", myName);
}
```

python文件中可调用的Param的类型位于[params.py](https://github.com/uart/gem5-mirror/blob/master/src/python/m5/params.py)中。（Python中的`m5.params`）