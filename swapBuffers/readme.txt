
swapBuffer��Ҫ�����˻����������ķ������ѵ�����Ǵ�˵�е�˫���壿Ӧ�ò��ǣ�

1. FLT_IO_PARAMETER_BLOCK�ṹ�Ǹ����õģ������ж�γ���
typedef struct _FLT_IO_PARAMETER_BLOCK {
  ULONG  IrpFlags;			// λ��ǣ����ڱ�Ǹ��ֲ�ͬ���͵�IO����;��Щ���ֻ���ڻ���IRP��IO������
  UCHAR  MajorFunction;		// IRP������
  UCHAR  MinorFunction;		// IRP������
  UCHAR  OperationFlags;	// λ���
  UCHAR  Reserved;			// Ԥ����ϵͳʹ�ã������ò�����
  PFILE_OBJECT  TargetFileObject;	// IO������Ŀ�ꡣһ���ļ�����ָ�룬����һ���ļ���Ŀ¼
  PFLT_INSTANCE  TargetInstance;	// IO������Ŀ�ꡣһ����͸����΢������ָ��
  FLT_PARAMETERS  Parameters;		// �����ṹ������ĳ�Ա��������IRP�йأ�������Ͳμ�WDK�����ĵ���
} FLT_IO_PARAMETER_BLOCK, *PFLT_IO_PARAMETER_BLOCK;

2. ǰ������У�����������δ��ݵģ�
��Ҫ���ݵ�������ָ�븳ֵ��ǰ�������������CompletionContext�������������ʱ��Ὣ�˲������롣

3. FlagOn�Ǹ����õģ�
��ʵ������λ���㣬���ڼ��ĳֵ���Ƿ���ڴ� ��־λ

4. �������Ľṹ��������InstanceSetup()�������棬�������Ƕ���ľ������Ľṹ���õġ���ô���Ժ��ȡ�������ĵ�ʱ��Ϳ��������Ƕ���Ľṹ����ȡ�ˡ�

5. ���ռ����ʡ�����չʾ�Ļ���������������Ϊ�˸�ʲô��������Ϊ�˴���ǰ���������״̬ô��