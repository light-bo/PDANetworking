#include "stdafx.h"
#include "YiniuHttp.h"


#include <string>
using namespace std;

const int ARRAY_SIZE = 255;
const int MAXBUF = 4096;


//自定义消息
const int MY_MESSAGE = WM_USER + 0x00000100;

//网络重新请求次数
const int RETRY_TIMES = 3;


UINT __cdecl CYiniuHttp::WorkerThreadProc(LPVOID pParam)
{
	CYiniuHttp* pYiniuHttp = static_cast<CYiniuHttp*>(pParam);

	while( !(pYiniuHttp->m_bThreadTerminated) )
	{
		try
		{
			//发送请求
			pYiniuHttp->SendRequest();
			PostThreadMessage(pYiniuHttp->m_dwThreadId, MY_MESSAGE, NULL, NULL);
		}catch(CInternetException* pEx)
		{
			//连接失败，尝试重新连接
			int iRet = AfxMessageBox( TEXT("网络状况不佳，发送失败，是否重新连接？"), 
				                      MB_YESNO | MB_ICONINFORMATION );
			if(IDYES == iRet)
			{
				UINT uTimes = RETRY_TIMES;
				while(uTimes--)
				{
					try
					{
						//发送请求
						pYiniuHttp->SendRequest();
						PostThreadMessage( pYiniuHttp->m_dwThreadId, MY_MESSAGE, 
										   NULL, NULL );
						break;
					}catch(CInternetException* pEx)
					{ 
						Sleep(3000);
						if(0 == uTimes) 
						{
							pEx->Delete();
							AfxMessageBox(TEXT("重新发送失败，请稍后再试！"));
							exit(-1);
						}
					}
				}//while
			} else exit(0);


			pEx->Delete();
		}//catch

		pYiniuHttp->m_pThread->SuspendThread();
	}//while
	
	return 0;
}





CYiniuHttp::CYiniuHttp(void): 
	m_bHadCreateThread(FALSE),
	m_bThreadTerminated(FALSE)
{
	m_strPostHeader = TEXT("Content-Type: application/x-www-form-urlencoded;");
	m_bIsGet = TRUE;//默认为 get 请求
	m_dwThreadId = GetCurrentThreadId();//界面线程ID
}


CYiniuHttp::~CYiniuHttp(void){ } 


void CYiniuHttp::SendRequest() {
	if(m_bIsGet)
		m_pFile->SendRequest();
	else
		m_pFile->SendRequest( m_strPostHeader,
		                      m_strPostHeader.GetLength(),
		                      m_straFormData.GetBuffer(),
		                      m_straFormData.GetLength() );
}


CYiniuHttp* CYiniuHttp::m_instance = NULL;

CYiniuHttp* CYiniuHttp::GetInstance()
{
	if(!m_instance) 
	{
		m_instance = new CYiniuHttp;
		m_instance->ConnectToTheServer(serverIPAddr);
	}

	return m_instance;
}


BOOL CYiniuHttp::ConnectToTheServer(const CString& strIP, const INTERNET_PORT port)
{
	//建立 http 连接
	try
	{
		m_pConnection = m_session.GetHttpConnection(strIP, INTERNET_PORT(port));
	} catch(CInternetException* pEx)
	{
		PrintErrorInfo(pEx);
		pEx->Delete();
		return FALSE;
	}

	return TRUE;
}


void CYiniuHttp::WaitForThreadCompletedRequest(HANDLE* pHthread)
{
	MSG msg;
	BOOL bWaitAll = FALSE;
	int nWaitCount = 1;  
	DWORD dwRet;

	while(TRUE)
	{
		dwRet = MsgWaitForMultipleObjects(nWaitCount, pHthread, bWaitAll, INFINITE, QS_ALLINPUT);
		if(dwRet == WAIT_OBJECT_0 + nWaitCount)
		{
			//接收到消息, 进行消息的循环
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if(MY_MESSAGE == msg.message)
					return;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		} 
		else
			//线程完成请求
			break;
	}
}


DWORD CYiniuHttp::SendRequest(const CString& strURI, CString& strRespondData)
{
	try
	{
		//打开 http 连接
		if(m_bIsGet)
			m_pFile = m_pConnection->OpenRequest(CHttpConnection::HTTP_VERB_GET, strURI);
		else
			m_pFile = m_pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, strURI);

	} catch(CInternetException* pEx)
	{
		PrintErrorInfo(pEx);
		return -1;
	}

	//创建线程完成请求
	if(!m_bHadCreateThread)
	{
		m_pThread = AfxBeginThread(WorkerThreadProc, this);
		m_pThread->m_bAutoDelete = TRUE;
		m_bHadCreateThread = TRUE;
	}
	else
		m_pThread->ResumeThread();

	DWORD dwRet;
	
	//刷新界面，等待线程完成请求
	WaitForThreadCompletedRequest(&(m_pThread->m_hThread));

	//获取响应消息的状态和数据
	GetRespondData(strRespondData);
	m_pFile->QueryInfoStatusCode(dwRet);

	return dwRet;
}


DWORD CYiniuHttp::Get(const CString& strURI, CString& strRespondData)
{
	m_bIsGet = TRUE;
	return SendRequest(strURI, strRespondData);
}


DWORD CYiniuHttp::Post(const CString& strURI, CString& strRespondData, CStringA& straFormData)
{
	m_straFormData = straFormData;
	m_bIsGet = FALSE;

	return SendRequest(strURI, strRespondData);
}


void CYiniuHttp::PrintErrorInfo(CInternetException* ex)
{
	TCHAR szCause[ARRAY_SIZE];
	CString strFormatted(_T(""));

	ex->GetErrorMessage(szCause, ARRAY_SIZE);
	strFormatted += szCause;
	AfxMessageBox(strFormatted);

	ex->Delete();
}


void CYiniuHttp::CloseConnection()
{
	m_pFile->Close();
	if(m_pFile) delete m_pFile;

	m_pConnection->Close();
	if(m_pConnection) delete m_pConnection;

	m_session.Close();


	m_bThreadTerminated = TRUE;
	m_pThread->ResumeThread();
	Sleep(10);//界面线程让出CPU时间，让工作线程自行结束运行
}


void CYiniuHttp::GetRespondData(CString& strRespondData)
{
	char* buffer = new char[MAXBUF];
	UINT nBytesRead = 0;
	
	while ( (nBytesRead=m_pFile->Read(buffer, MAXBUF-1)) > 0 )
	{
		buffer[nBytesRead] = '\0';
		CString strTemp(buffer);
		strRespondData += strTemp;
	}
	
	delete []buffer;
}