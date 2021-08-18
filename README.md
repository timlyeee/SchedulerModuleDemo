# README

## 简介

这是C++ Scheduler独立模块的测试demo，用来测试Scheduler模块的独立使用的可用性，和被继承被调用时的完整性。

作为测试例存在首先要保证能够流畅运行，这是个控制台程序，所以可以不使用图形界面。其次是明确调度对象，用来保障调用的合法合规。（调度的方式会在后续写出）

Scheduler被调度的前提是组件成立，即Timer和ListEntry的数据结构正确。所以测试会首先独立测试这两个对象的可使用性。

有几个问题目前没有解答：C++ Timer的调度是否可以涉及线程操作？是否能够保证while循环的及时准确性？多线程操作是否能够提高性能？

## 测试列表

- [ ] Timer
- [ ] TargetCallbackTimer
- [ ] ISchedulable
- [ ] System
- [ ] Scheduler
- [ ] ListEntry
- [ ] HashTimerEntry
- [ ] HashUpdateEntry
- [ ] Director

## 测试方法

- Timer
  - 通过创建对象赋予Timer进行时间等待操作。模拟JS层的setInterval和setTimeout等功能进行类似的回调操作。
  - Timer循环机制。借鉴C#的System.threading.timer
- TargetCallbackTimer
  - Callback 函数的正确调度
- ISchedulable
  - 父类指针的子类函数调度
- ListEntry
  - Constructor & Destructor
