

scanner主要介绍了反病毒软件内核的实现方式

其中的基础知识点是：内核与用户态通信

驱动程序：
1. 调用FltBuildDefaultSecurityDescriptor()创建默认的安全描述符。
2. 调用InitializeObjectAttributes()初始化驱动端口名。
3. 调用FltCreateCommunicationPort()创建驱动端口。
3.1 连接端口的处理例程定义：
	NTSTATUS ScannerPortConnect(__in PFLT_PORT ClientPort, __in_opt PVOID ServerPortCookie,
			__in_bcount_opt(SizeOfContext) PVOID ConnectionContext, __in ULONG SizeOfContext, 
			__deref_out_opt PVOID *ConnectionCookie);
	创建完毕我们可以调用FltSendMessage()向指定的客户端口发送消息。
			
	断开端口的处理例程定义：
	VOID ScannerPortDisconnect(__in_opt PVOID ConnectionCookie);
	创建完毕我们可以调用FltCloseClientPort()关闭客户端口。
4. 调用FltFreeSecurityDescriptor()释放安全描述符。
5. 调用FltCloseCommunicationPort()关闭驱动端口

用户态程序
1. 调用FilterConnectCommunicationPort()创建一个与内核连接的通信通道
2. 调用CreateIoCompletionPort()创建一个完成端口，将完成端口与通信端口绑定在线程上下文中
3. 调用FilterSendMessage()向驱动发送请求
4. 调用FilterReplyMessage()或FilterGetMessage()从驱动程序中获取消息

//====================================================================================================
关于FLT_REGISTRATION结构的InstanceSetup与InstanceQueryTeardown例程的一些自己的看法：

a. 关于InstanceSetup例程
   当在一个卷上创建一个新的实例的时候，这个例程被调用。
   我们是否可以不做处理，直接返回STATUS_SUCCESS？
   注：参考ISICOFS加密驱动，这里的载入部分可以参考swapBuffer里面的逻辑，但是具体的需求还是得自己写。
   问：为什么一定要处理这个例程？出于一个什么样的机制？
   
b. 关于InstanceQueryTeardown例程
   当调用FltDetachVolume()或FilterDetach()手动删除一个实例的时候，这个例程被调用。
   这个例程可以总是返回OK，即不做任何处理，允许删除。
   
c. FLT_CONTEXT_REGISTRATION的作用
   问：什么是“实例”？什么是“上下文”？为什么一定要设置“上下文”？