本驱动只实现驱动加载和卸载例程，不对其他IRP做过滤

在scanner里面没有看到驱动如何实时响应用户发送来的信息。
实际上在FltCreateCommunicationPort()创建端口响应函数的时候会有一个消息响应函数的设置，其原型如下：
typedef NTSTATUS (*PFLT_MESSAGE_NOTIFY) (
      IN PVOID PortCookie,
      IN PVOID InputBuffer OPTIONAL,
      IN ULONG InputBufferLength,
      OUT PVOID OutputBuffer OPTIONAL,
      IN ULONG OutputBufferLength,
      OUT PULONG ReturnOutputBufferLength
      );
