##简介
该类是在 windows 环境下利用 MFC CInternetSession 等与网络相关的类封装的一个用于网络请求的类 Get
和 Post，

该类中利用多线程实现了网络的异步请求，请求网络数据时在新建的工作线程中进行，从而避免由于主线程（界面线程）的阻塞而导致界面阻塞的问题。

##使用方法

```
/*Get 请求方式*/
假设有接口地址为:http://www.XXXX.com/Public/Login/

const CString strIpAddr(_T("www.XXXX.com"));
const CString strPort(_T("80"));

const CString strURI("/Public/Login/");

CString strRespondData;

CYiniuHttp *httpClient = CYiniuHttp::GetInstance();

httpClient->Get(strURI, strRespondData);

```
