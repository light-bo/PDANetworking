#ifndef YINIU_HTTP
#define YINIU_HTTP


class CYiniuHttp
{
public:
	~CYiniuHttp(void);

public:
	//网络请求单例
	static CYiniuHttp* GetInstance();

	/**
	 *
	 * 连接服务器，创建 http 连接
	 * @param strIP 服务器IP地址
	 * @param port 服务器端口（端口号默认为80）
	 * @return 成功返回TRUE，否则返回FALSE
	 */
	BOOL ConnectToTheServer(const CString& strIP, const INTERNET_PORT port = 80);

	/**
	 *
	 * 发送Get请求
	 *
	 * @param strURI 请求URI
	 * @param strRespondData 响应消息数据
	 * @return 响应消息状态码
	 *
	 */
	DWORD Get(const CString& strURI, CString& strRespondData);

	/**
	 *
	 * 发送POST请求
	 * @param strURI 请求URI
	 * @param strRespondData 响应消息数据
	 * @param post请求表格数据
	 *
	 * @return 响应消息状态码
	 */
	DWORD Post(const CString& strURI, CString& strRespondData, CStringA& straFormData);

	//关闭连接，释放资源， 关闭工作线程
	void CloseConnection();

private:
	static CYiniuHttp *m_instance;

private:
	CInternetSession m_session;
	CHttpConnection* m_pConnection;
	CHttpFile* m_pFile;

	//是否结束工作线程标志
	BOOL m_bThreadTerminated;
	
	//工作线程 ID
	DWORD m_dwThreadId;

	//工作线程对象
	CWinThread* m_pThread;

	//是否已经创建线程标志
	BOOL m_bHadCreateThread;

	//POST 请求数据
	CStringA m_straFormData;

	//POST 请求头
	CString m_strPostHeader;

	//请求方式
	BOOL m_bIsGet;

private:
	CYiniuHttp(void);

	//打印错误的信息
	void PrintErrorInfo(CInternetException* ex);

	//获取响应消息的数据
	void GetRespondData(CString& strRespondData);

	//刷新界面，等待线程完成请求
	void WaitForThreadCompletedRequest(HANDLE* dpHthread);

	//工作线程入口函数
	static UINT  WorkerThreadProc(LPVOID pParam);

	//发送请求,返回响应消息状态码
	DWORD SendRequest(const CString& strURI, CString& strRespondData);

	//根据请求类型发送请求(Get or Post)
	void SendRequest();
};


#endif
