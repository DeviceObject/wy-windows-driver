������ֻʵ���������غ�ж�����̣���������IRP������

��scanner����û�п����������ʵʱ��Ӧ�û�����������Ϣ��
ʵ������FltCreateCommunicationPort()�����˿���Ӧ������ʱ�����һ����Ϣ��Ӧ���������ã���ԭ�����£�
typedef NTSTATUS (*PFLT_MESSAGE_NOTIFY) (
      IN PVOID PortCookie,
      IN PVOID InputBuffer OPTIONAL,
      IN ULONG InputBufferLength,
      OUT PVOID OutputBuffer OPTIONAL,
      IN ULONG OutputBufferLength,
      OUT PULONG ReturnOutputBufferLength
      );
