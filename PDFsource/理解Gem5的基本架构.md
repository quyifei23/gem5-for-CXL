## 理解Gem5的基本架构

#### 分析gem5的事件驱动原理

gem5是一个事件驱动的模拟器，在事件驱动模型中，每个事件都有一个回调函数用于处理事件。

##### 对于创建事件的解释

SimObject类是gem5中最重要的类之一，几乎所有的系统组件，如CPU，总线，Cache等都需要继承SimObject类。根据源代码，SimObject类继承了EventManager类，EventManager类中包装了一个EventQueue对象。因此，在startup()函数中使用的schedule()函数本质就是将event放入事件队列中，等待被调度。另外由于建立了回调函数与事件对象之间的联系，当事件被触发后，回调函数会被执行。

*所以可以理解为当创建了一个继承SimObject类的对象后，就是将其包含的event放入了事件队列中，当事件被触发后，会执行相应的回调函数*



#### 分析gem5中对象序列化操作

gem5在对模拟器中的对象序列化前，需要先将其排水(Drain)，将不确定的状态先清除，再将对象序列化到磁盘中

##### Drain

当gem5正常运行时，模拟器中的对象在一开始都处于DrainState::Running状态，并用事件驱动模拟器的运行这会导致很多对象再运行时处于似是而非的状态——部分信号正在传递，部分程序正在进行，缓冲区还待处理等。然而，模拟器总要在某些时刻有所停顿。这时候就需要引入drain的概念，将这些中间态清除。drain指系统清空SimObject对象中内部状态的过程。



#### 分析gem5中的SimObject

SimObject 类是一个非常复杂但又十分重要的类，它在 Gem5 中占有极为重要的地位。gem5 的模块化设计是围绕 SimObject 类型构建的。 模拟系统中的大多数组件都是 SimObjects 的子类，如CPU、缓存、内存控制器、总线等。gem5 将所有这些对象从其 C++ 实现导出到 python。使用提供的 python 配置脚本便可以创建任何 SimObject 类对象，设置其参数，并指定 SimObject 之间的交互。理解该类的实现有助于我们理解整个 gem5 模拟器的运作逻辑。我们先从它的父类开始讲起，它一共有 5 个父类：EventManger、Serializable、Drainable、statistics::Group、Named。

- EventManager 类：负责调度、管理、执行事件。EventManager 类是对 EventQueue 类的包装，SimObject 对象中所有的事件实际都由 EventQueue 队列管理。该队列以二维的单链表的形式管理着所有事件，事件以触发时间点从近到远排列。
- Serializable 类：负责对象的序列化。SimObjects 可通过 `SimObject::serializeAll()` 函数自动完成序列化，写入到自己的 sections 中。Serializable 类根据 SimObject 类对象的名字以及对象间的包含关系，帮助用户构建起了层次化的序列化模型，并使用该模型完成 SimObject 的序列化，以 ini 文件格式输出。
- Drainable 类：负责 drain 对象。DrainManager 类以单例的方式管理整个模拟器的 drain 过程。只有系统中所有的对象都被 drained，才能开始序列化、更改模型等操作。完成后需要使用 `DrainManager::resume()` 函数将系统回归到正常运行状态。
- statistics::Group 类：负责运行过程中统计、管理数据。Group 对象之间可组成树状层次，从而反应出 SimObject 对象间的树状层次。
- Name 类：负责给 SimObject 起名。