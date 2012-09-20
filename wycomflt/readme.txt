
本驱动用于过滤COM端口数据，例子源于《寒江独钓 Windows内核安全编程》。

此例展示了如何使用WDM模式驱动制作过滤驱动，如何在FDO与PDO之间附加驱动。

这里我介绍一些关于使用C++编译驱动程序的方法：
1. 大部分的Windows驱动开发的书籍都是使用C语言进行开发，很多使用C++习惯的程序员并不太习惯C语言严谨的代码风格。
2. C++在编译上与C的区别主要就是函数的符号表，在代码中需要将C++导出符号表强制设置为C风格的，因此我们需要在以下几个地方做出相应的调整：
	a. 在引用系统头文件的时候需要添加 extern "C" 。
	b. 在需要导出的例程头部添加 extern "C" 。
	c. 如果要兼容C语言则需要使用宏来规范 #ifdef __cplusplus/#endif
3. 其他的函数编写与C语言没有太大区别。

接下来介绍此例子的工作流程：
DriverEntry：
1. 将所有IRP的派遣函数设置为CFTDefaultDispatch()，由它统一派发各IRP请求。
2. 设置动态卸载例程CFPUnloadDriver()
3. 绑定所有串口
	3.1 打开串口
		3.1.1 串口的名称：\Device\Serial0 ~ \Device\Serial31
		3.1.2 串口设备的总数量为32
		3.1.3 调用IoGetDeviceObjectPointer()打开指定的串口设备
	3.2 绑定设备
		3.2.1 生成过滤驱动设备IoCreateDevice()