#ifndef YINIU_HTTP
#define YINIU_HTTP


class CYiniuHttp
{
public:
	~CYiniuHttp(void);

public:
	//����������
	static CYiniuHttp* GetInstance();

	/**
	 *
	 * ���ӷ����������� http ����
	 * @param strIP ������IP��ַ
	 * @param port �������˿ڣ��˿ں�Ĭ��Ϊ80��
	 * @return �ɹ�����TRUE�����򷵻�FALSE
	 */
	BOOL ConnectToTheServer(const CString& strIP, const INTERNET_PORT port = 80);

	/**
	 *
	 * ����Get����
	 *
	 * @param strURI ����URI
	 * @param strRespondData ��Ӧ��Ϣ����
	 * @return ��Ӧ��Ϣ״̬��
	 *
	 */
	DWORD Get(const CString& strURI, CString& strRespondData);

	/**
	 *
	 * ����POST����
	 * @param strURI ����URI
	 * @param strRespondData ��Ӧ��Ϣ����
	 * @param post����������
	 *
	 * @return ��Ӧ��Ϣ״̬��
	 */
	DWORD Post(const CString& strURI, CString& strRespondData, CStringA& straFormData);

	//�ر����ӣ��ͷ���Դ�� �رչ����߳�
	void CloseConnection();

private:
	static CYiniuHttp *m_instance;

private:
	CInternetSession m_session;
	CHttpConnection* m_pConnection;
	CHttpFile* m_pFile;

	//�Ƿ���������̱߳�־
	BOOL m_bThreadTerminated;
	
	//�����߳� ID
	DWORD m_dwThreadId;

	//�����̶߳���
	CWinThread* m_pThread;

	//�Ƿ��Ѿ������̱߳�־
	BOOL m_bHadCreateThread;

	//POST ��������
	CStringA m_straFormData;

	//POST ����ͷ
	CString m_strPostHeader;

	//����ʽ
	BOOL m_bIsGet;

private:
	CYiniuHttp(void);

	//��ӡ�������Ϣ
	void PrintErrorInfo(CInternetException* ex);

	//��ȡ��Ӧ��Ϣ������
	void GetRespondData(CString& strRespondData);

	//ˢ�½��棬�ȴ��߳��������
	void WaitForThreadCompletedRequest(HANDLE* dpHthread);

	//�����߳���ں���
	static UINT  WorkerThreadProc(LPVOID pParam);

	//��������,������Ӧ��Ϣ״̬��
	DWORD SendRequest(const CString& strURI, CString& strRespondData);

	//�����������ͷ�������(Get or Post)
	void SendRequest();
};


#endif
