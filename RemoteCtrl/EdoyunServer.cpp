#include "EdoyunServer.h"
#include "pch.h"

//template<EdoyunOperator op>
//AcceptOverlapped<op>::AcceptOverlapped()
//{
//	m_worker = ThreadWorker(this,(FUNCTYPE)&AcceptOverlapped::AcceptWorker);
//	m_operator = EAccept;
//	memset(m_overlapped, 0, sizeof(m_overlapped));
//	m_buffer.resize(1024);
//	m_server(NULL);
//}
//
//template<EdoyunOperator op>
//int AcceptOverlapped<op>::AcceptWorker(){
//	INT lLength = 0, rLength = 0;
//	if (*(LPDWORD)*m_client.get()> 0)
//	{
//		//���ڴ�������ͨ���е��������󣬲��������ӵı��غ�Զ�̵�ַ��Ϣ��
//		GetAcceptExSockaddrs(*m_client, 0,
//			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
//			(sockaddr**)m_client->GetLocalAddr(), &lLength,//���ص�ַ
//			(sockaddr**)m_client->GetRemoteAddr(), &lLength//Զ�̵�ַ
//		)
//
//			//Ͷ��һ����������
//			if (!m_server->NewAccept())
//			{
//				return -2;
//			}
//	}
//	return -1;
//}
//
//void EdoyunClient::SetOverlappedClient(PCLIENT& ptr) {
//	m_overlapped->m_client = ptr;
//}
//
//EdoyunClient::EdoyunClient() :m_isbusy(false), m_overlapped(new ACCEPTOVERLAPPED()){
//	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
//	m_buffer.resize(1024);
//	memset(&m_laddr, 0, sizeof(m_laddr));
//	memset(&m_raddr, 0, sizeof(m_raddr));
//}
//
//EdoyunClient:: operator LPOVERLAPPED(){
//	return &m_overlapped->m_overlapped;
//}