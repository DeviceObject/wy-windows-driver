
swapBuffer主要介绍了缓冲区交换的方法（难道这就是传说中的双缓冲？应该不是）

1. FLT_IO_PARAMETER_BLOCK结构是干嘛用的，例子中多次出现
typedef struct _FLT_IO_PARAMETER_BLOCK {
  ULONG  IrpFlags;			// 位标记，用于标记各种不同类型的IO操作;这些标记只用于基于IRP的IO操作。
  UCHAR  MajorFunction;		// IRP主类型
  UCHAR  MinorFunction;		// IRP辅类型
  UCHAR  OperationFlags;	// 位标记
  UCHAR  Reserved;			// 预留给系统使用，我们用不着它
  PFILE_OBJECT  TargetFileObject;	// IO操作的目标。一个文件对象指针，代表一个文件或目录
  PFLT_INSTANCE  TargetInstance;	// IO操作的目标。一个不透明的微过滤器指针
  FLT_PARAMETERS  Parameters;		// 参数结构，具体的成员跟所处的IRP有关（具体解释参见WDK帮助文档）
} FLT_IO_PARAMETER_BLOCK, *PFLT_IO_PARAMETER_BLOCK;

2. 前后操作中，上下文是如何传递的？
将要传递的上下文指针赋值给前操作的输出参数CompletionContext，这样后操作的时候会将此参数传入。

3. FlagOn是干嘛用的？
它实际上是位运算，用于检测某值中是否存在此 标志位

4. 卷上下文结构的设置在InstanceSetup()例程里面，根据我们定义的卷上下文结构设置的。那么在以后获取卷上下文的时候就可以以我们定义的结构来获取了。

5. 【终极疑问】此例展示的缓冲区交换倒是是为了干什么？仅仅是为了传递前操作的完成状态么？