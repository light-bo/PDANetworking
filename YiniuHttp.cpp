#include "stdafx.h"
#include "YiniuHttp.h"


#include <string>
using namespace std;

const int ARRAY_SIZE = 255;
const int MAXBUF = 4096;


//�Զ�����Ϣ
const int MY_MESSAGE = WM_USER + 0x00000100;

//���������������
const int RETRY_TIMES = 3;


UINT __cdecl CYiniuHttp::WorkerThreadProc(LPVOID pParam)
{
	CYiniuHttp* pYiniuHttp = static_cast<CYiniuHttp*>(pParam);

	while( !(pYiniuHttp->m_bThreadTerminated) )
	{
		try
		{
			//��������
			pYiniuHttp->SendRequest();
			PostThreadMessage(pYiniuHttp->m_dwThreadId, MY_MESSAGE, NULL, NULL);
		}catch(CInternetException* pEx)
		{
			//����ʧ�ܣ�������������
			int iRet = AfxMessageBox( TEXT("����״�����ѣ�����ʧ�ܣ��Ƿ��������ӣ�"), 
				                      MB_YESNO | MB_ICONINFORMATION );
			if(IDYES == iRet)
			{
				UINT uTimes = RETRY_TIMES;
				while(uTimes--)
				{
					try
					{
						//��������
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
							AfxMessageBox(TEXT("���·���ʧ�ܣ����Ժ����ԣ�"));
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
	m_bIsGet = TRUE;//Ĭ��Ϊ get ����
	m_dwThreadId = GetCurrentThreadId();//�����߳�ID
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
	//���� http ����
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
			//���յ���Ϣ, ������Ϣ��ѭ��
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if(MY_MESSAGE == msg.message)
					return;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		} 
		else
			//�߳��������
			break;
	}
}


DWORD CYiniuHttp::SendRequest(const CString& strURI, CString& strRespondData)
{
	try
	{
		//�� http ����
		if(m_bIsGet)
			m_pFile = m_pConnection->OpenRequest(CHttpConnection::HTTP_VERB_GET, strURI);
		else
			m_pFile = m_pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, strURI);

	} catch(CInternetException* pEx)
	{
		PrintErrorInfo(pEx);
		return -1;
	}

	//�����߳��������
	if(!m_bHadCreateThread)
	{
		m_pThread = AfxBeginThread(WorkerThreadProc, this);
		m_pThread->m_bAutoDelete = TRUE;
		m_bHadCreateThread = TRUE;
	}
	else
		m_pThread->ResumeThread();

	DWORD dwRet;
	
	//ˢ�½��棬�ȴ��߳��������
	WaitForThreadCompletedRequest(&(m_pThread->m_hThread));

	//��ȡ��Ӧ��Ϣ��״̬������
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
	Sleep(10);//�����߳��ó�CPUʱ�䣬�ù����߳����н�������
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