

scanner��Ҫ�����˷���������ں˵�ʵ�ַ�ʽ

���еĻ���֪ʶ���ǣ��ں����û�̬ͨ��

��������
1. ����FltBuildDefaultSecurityDescriptor()����Ĭ�ϵİ�ȫ��������
2. ����InitializeObjectAttributes()��ʼ�������˿�����
3. ����FltCreateCommunicationPort()���������˿ڡ�
3.1 ���Ӷ˿ڵĴ������̶��壺
	NTSTATUS ScannerPortConnect(__in PFLT_PORT ClientPort, __in_opt PVOID ServerPortCookie,
			__in_bcount_opt(SizeOfContext) PVOID ConnectionContext, __in ULONG SizeOfContext, 
			__deref_out_opt PVOID *ConnectionCookie);
	����������ǿ��Ե���FltSendMessage()��ָ���Ŀͻ��˿ڷ�����Ϣ��
			
	�Ͽ��˿ڵĴ������̶��壺
	VOID ScannerPortDisconnect(__in_opt PVOID ConnectionCookie);
	����������ǿ��Ե���FltCloseClientPort()�رտͻ��˿ڡ�
4. ����FltFreeSecurityDescriptor()�ͷŰ�ȫ��������
5. ����FltCloseCommunicationPort()�ر������˿�

�û�̬����
1. ����FilterConnectCommunicationPort()����һ�����ں����ӵ�ͨ��ͨ��
2. ����CreateIoCompletionPort()����һ����ɶ˿ڣ�����ɶ˿���ͨ�Ŷ˿ڰ����߳���������
3. ����FilterSendMessage()��������������
4. ����FilterReplyMessage()��FilterGetMessage()�����������л�ȡ��Ϣ

//====================================================================================================
����FLT_REGISTRATION�ṹ��InstanceSetup��InstanceQueryTeardown���̵�һЩ�Լ��Ŀ�����

a. ����InstanceSetup����
   ����һ�����ϴ���һ���µ�ʵ����ʱ��������̱����á�
   �����Ƿ���Բ�������ֱ�ӷ���STATUS_SUCCESS��
   ע���ο�ISICOFS������������������벿�ֿ��Բο�swapBuffer������߼������Ǿ���������ǵ��Լ�д��
   �ʣ�Ϊʲôһ��Ҫ����������̣�����һ��ʲô���Ļ��ƣ�
   
b. ����InstanceQueryTeardown����
   ������FltDetachVolume()��FilterDetach()�ֶ�ɾ��һ��ʵ����ʱ��������̱����á�
   ������̿������Ƿ���OK���������κδ�������ɾ����
   
c. FLT_CONTEXT_REGISTRATION������
   �ʣ�ʲô�ǡ�ʵ������ʲô�ǡ������ġ���Ϊʲôһ��Ҫ���á������ġ���