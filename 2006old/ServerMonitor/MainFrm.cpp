// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "Utils.h"

#include "aboutdlg.h"
#include "ServerMonitorView.h"
#include "ServerStatus.h"
#include "ServerSummary.h"
#include "MainFrm.h"

BOOL CMainFrame::PreTranslateMessage(PMSG pMsg)
{
	if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;
	if(pMsg->hwnd == m_hWndStatusBar && pMsg->message == WM_LBUTTONDOWN)
	{
		OnStatusBarLButtonDown(pMsg->message, pMsg->wParam, pMsg->lParam);
	}
	return m_view.PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle()
{
	UIUpdateToolBar();
	ViewType *pView = (ViewType *)m_view.GetPageData(m_view.GetActivePage());
	UIEnable(ID_SERVERS_RESTARTALL, m_view.GetPageCount()>1);
	UIEnable(ID_SERVERS_RESTART, pView->type() == ViewType::STATUS_TYPE);
	UIEnable(ID_WINDOW_CLOSE_ALL, m_view.GetPageCount()>1);
	UIEnable(ID_WINDOW_CLOSE, pView->type() == ViewType::STATUS_TYPE);
	return FALSE;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	

	for(DWORD a=0; a<15; a++)
	{
		m_PreVec.push_back(new std::vector<MIB_TCPROW_OWNER_PID>(10000));
	}
	
	new Utils();

	m_TimerCount = 0;
	m_RefreshCount = 0;


	ServiceChecker::Instance()->Init();

	
	// create command bar window
	HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
	// attach menu
	m_CmdBar.AttachMenu(GetMenu());
	// load command bar images
	m_CmdBar.LoadImages(IDR_MAINFRAME);
	m_CmdBar.m_bFlatMenus = TRUE;
	// remove old menu
	SetMenu(NULL);

	HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
	AddSimpleReBarBand(hWndToolBar, NULL, TRUE);

	CreateSimpleStatusBar();

	m_sb.Attach(m_hWndStatusBar);
	int panels[] = {1,IDP_CPU, IDP_MEM, IDP_NETUSAGE};
	m_sb.SetPanes(panels, sizeof(panels)/sizeof(panels[0]), FALSE);
	int widths[] = {110,150,250,300};
	for(int a=0; a<sizeof(panels)/sizeof(panels[0]); a++)
	{
		m_sb.SetPaneWidth(panels[a], widths[a]);
	}


	m_pCPU = new CPUsage(::GetCurrentProcessId());
	m_pNet = new NetUsage();

	m_NetPopup.CreatePopupMenu();
	m_CurrentSelNetIfIndex = 0;
	for(DWORD a=0; a<m_pNet->m_IfRow.size(); a++)
	{
		const NetUsage::NetIf  & Nif = m_pNet->m_IfRow[a];
		if(m_pNet->m_IfRow[m_CurrentSelNetIfIndex].LastIn<=Nif.LastIn)
		{
			m_CurrentSelNetIfIndex = a;
		}
		m_NetPopup.AppendMenu(MF_STRING, a+1, Nif.Name.c_str());
	}

	m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);

	UIAddToolBar(hWndToolBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	CMenuHandle menuMain = m_CmdBar.GetMenu();
	m_view.SetWindowMenu(menuMain.GetSubMenu(WINDOW_MENU_POSITION));
	m_view.m_nMenuItemsCount = 20;

	SendMessage(WM_COMMAND, ID_VIEW_TOOLBAR);

	CRect rc;
	m_view.GetClientRect(&rc);
	m_hSummary = new CServerSummary;
	m_hSummary->Create(m_view, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0);
	m_view.AddPage(m_hSummary->m_hWnd,L"Summary", -1, (LPVOID)(ViewType *)m_hSummary);



	Utils::GetDebugPriv();
	SendMessage(WM_COMMAND,ID_REFRESHTIME_REFRESHNOW);
	SetTimer(1, 1000,NULL);

	





	SendMessage(WM_COMMAND, ID_REFRESHTIME_2SECONDS);




	m_view.SetActivePage(0);
	


	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	// unregister message filtering and idle updates
	m_sb.Detach();
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = FALSE;

	for(DWORD a=0; a<m_PreVec.size(); a++)
	{
		delete m_PreVec[a];
	}

	return 1;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{

	// TODO: add code to initialize document

	return 0;
}

LRESULT CMainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnWindowClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int nActivePage = m_view.GetActivePage();
	CServerStatus *pServer = dynamic_cast<CServerStatus *>( (ViewType *)m_view.GetPageData(nActivePage) );
	
	if(FALSE == Utils::KillProcess(pServer->m_ProcInfo.PID))
	{
		AtlMessageBox(m_hWnd,(LPCTSTR) L"Error");
	}
	//SendMessage(WM_TIMER,1);
	return 0;
}

LRESULT CMainFrame::OnWindowCloseAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int c = m_view.GetPageCount();
	while(c>0)
	{
		CServerStatus *pServer = dynamic_cast<CServerStatus *>( (ViewType *)m_view.GetPageData(c-1) );
		if(pServer != NULL)
		{
			if(FALSE == Utils::KillProcess(pServer->m_ProcInfo.PID))
			{
				AtlMessageBox(m_hWnd,(LPCTSTR) L"Error");
			}
		}
		
		c--;
	}
	SendMessage(WM_TIMER,1);
	
	return 0;
}

LRESULT CMainFrame::OnWindowActivate(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int nPage = wID - ID_WINDOW_TABFIRST;
	m_view.SetActivePage(nPage);

	return 0;
}

BOOL CMainFrame::NewPage( Utils::ProcessInfo & Info )
{
	CServerStatus* pView = new CServerStatus;
	pView->m_ProcInfo = Info;
	pView->Create(m_view, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0);
	m_view.AddPage(pView->m_hWnd, Info.Name,-1, (LPVOID)(ViewType *)pView );
	m_hSummary->AddServer(pView);
	return TRUE;
}

void CMainFrame::RefreshConn()
{
	
	
	static std::vector<MIB_TCPROW_OWNER_PID> TCPtables(10000);
	static std::vector<MIB_UDPROW_OWNER_PID> UDPtables(10000);

	
	Utils::GetOpenedPort(TCPtables, UDPtables);

	stdext::hash_map<int, std::vector<MIB_TCPROW_OWNER_PID> *, Utils::hash_xk<int> > tcpmap;
	
	for (int a=0; a<m_view.GetPageCount(); a++)
	{
		m_PreVec[a]->clear();
		CServerStatus* pView = dynamic_cast<CServerStatus *>(  (ViewType *) m_view.GetPageData(a) );
		if(pView == NULL)
		{
			continue;
		}
		tcpmap[pView->m_ProcInfo.PID] = m_PreVec[a];
	}
	
	for (DWORD a=0;a<TCPtables.size();a++)
	{
		MIB_TCPROW_OWNER_PID & row = TCPtables[a];
		stdext::hash_map<int, std::vector<MIB_TCPROW_OWNER_PID> *>::iterator Iter = tcpmap.find(row.dwOwningPid);
		if(Iter!=tcpmap.end())
		{
			Iter->second->push_back(row);
		}
	}

	
	int active = m_view.GetActivePage();
	
	for (int a=0; a<m_view.GetPageCount(); a++)
	{
		CServerStatus* pView = dynamic_cast<CServerStatus *>(  (ViewType *) m_view.GetPageData(a) );
		if(pView == NULL)
		{
			continue;
		}
		
		if(active != a)
		{
			time_t t;
			if( pView->m_LastUpdate - (DOUBLE)time(&t) < 8 )
			{
				continue;
			}
		}
		
		pView->RefreshTcp( *(tcpmap[pView->m_ProcInfo.PID]) );
		pView->RefreshUdp(UDPtables);
		Utils::Rest();
	}

	
}

LRESULT CMainFrame::OnTimer( UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
{
	
	if( wParam == 1 )
	{


		RefreshStatusBar();
		++m_TimerCount;
		if(m_TimerCount >= m_RefreshCount)
		{	
			m_TimerCount = 0;
		}
		else
		{
			return 0;
		}
		
		for(int b=0; b<m_view.GetPageCount(); b++)
		{
			ViewType *pBase = (ViewType *)m_view.GetPageData(b);
			if(pBase->type() != ViewType::STATUS_TYPE)
			{
				continue;
			}
			CServerStatus* pView = dynamic_cast<CServerStatus *>(pBase);
			if(pView == NULL)
			{
				continue;
			}
			
			if(Utils::ProcessExit(pView->m_ProcInfo.PID) == FALSE)
			{
				RemovePage(b);
				--b;
			}
		}
		
		CAtlArray<Utils::ProcessInfo> procs;
		Utils::GetProcessInfo(procs);
		for(DWORD a=0; a<procs.GetCount(); a++)
		{
			if(procs[a].CompanyName == L"Possibility Space")
			{
				
				BOOL bFound = FALSE;
				for(int b=0; b<m_view.GetPageCount(); b++)
				{
					ViewType *pType = (ViewType *)m_view.GetPageData(b);
					if(pType->type() == ViewType::STATUS_TYPE )
					{
						CServerStatus* pView = dynamic_cast<CServerStatus *>( pType );
						if(pView == NULL)
							continue;
						if(pView->m_ProcInfo.PID == procs[a].PID)
						{
							bFound = TRUE;
							break;
						}
					}
					
				}
				if(!bFound)
				{
					NewPage(procs[a]);
				}
				
			}

		}
		
		if(m_view.GetPageCount() > 1)
		{
			RefreshConn();
		}
	
	}

	return 0;
}
LRESULT CMainFrame::OnRefreshtime2seconds(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your command handler code here
	m_RefreshCount = 2;
	ClearMenu();
	UISetCheck(ID_REFRESHTIME_2SECONDS,TRUE);
	return 0;
}

LRESULT CMainFrame::OnRefreshtime1second(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your command handler code here
	m_RefreshCount = 1;
	ClearMenu();
	UISetCheck(ID_REFRESHTIME_1SECOND,TRUE);
	return 0;
}

LRESULT CMainFrame::OnRefreshtime5seconds(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your command handler code here
	m_RefreshCount = 5;
	ClearMenu();
	UISetCheck(ID_REFRESHTIME_5SECONDS,TRUE);
	return 0;
}

LRESULT CMainFrame::OnRefreshtime10seconds(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your command handler code here
	m_RefreshCount = 10;
	ClearMenu();
	UISetCheck(ID_REFRESHTIME_10SECONDS,TRUE);
	return 0;
}

LRESULT CMainFrame::OnRefreshtimeStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your command handler code here
	m_RefreshCount = 0xfffff;
	ClearMenu();
	UISetCheck(ID_REFRESHTIME_STOP,TRUE);
	return 0;
}

LRESULT CMainFrame::OnRefreshtimeRefreshnow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your command handler code here
	m_TimerCount = m_RefreshCount;
	SendMessage(WM_TIMER,1);
	return 0;
}

void CMainFrame::ClearMenu()
{
	UISetCheck(ID_REFRESHTIME_1SECOND,FALSE);
	UISetCheck(ID_REFRESHTIME_2SECONDS,FALSE);
	UISetCheck(ID_REFRESHTIME_5SECONDS,FALSE);
	UISetCheck(ID_REFRESHTIME_10SECONDS,FALSE);
	UISetCheck(ID_REFRESHTIME_STOP,FALSE);
}
LRESULT CMainFrame::OnServersRestart(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your command handler code here
	int n = m_view.GetActivePage();
	if(n>-1)
	{
		CServerStatus *pView = dynamic_cast<CServerStatus *>((ViewType *)m_view.GetPageData(n));

		Utils::KillProcess(pView->m_ProcInfo.PID);
		if(pView->m_ServiceName.IsEmpty())
		{
			Utils::StartProcess(pView->m_CmdLine,pView->m_Title);
		}
		else
		{
			Utils::StartService(pView->m_ServiceName);
		}
	}
	return 0;
}

LRESULT CMainFrame::OnServersRestartall(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add your command handler code here
	for(int n =0; n<m_view.GetPageCount(); n++)
	{
		CServerStatus *pView = dynamic_cast<CServerStatus *>((ViewType *)m_view.GetPageData(n));
		if(pView == NULL)
			continue;
		Utils::KillProcess(pView->m_ProcInfo.PID);
		if(pView->m_ServiceName.IsEmpty())
		{
			Utils::StartProcess(pView->m_CmdLine,pView->m_Title);
		}
		else
		{
			Utils::StartService(pView->m_ServiceName);
		}
		
	}
	return 0;
}

BOOL CMainFrame::RemovePage( int n )
{
	CServerStatus *pView = dynamic_cast<CServerStatus *>( (ViewType *)m_view.GetPageData(n));
	m_hSummary->DelServer(pView);
	m_view.RemovePage(n);
	delete pView;

	return TRUE;
}



LRESULT CMainFrame::OnPageActiveted( int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/ )
{
	if(pnmh->hwndFrom == m_view)
	{
		int n = m_view.GetActivePage();
		if(n>-1)
		{
			ViewType *pView = (ViewType *)m_view.GetPageData(n);
			CServerStatus *pServer = dynamic_cast<CServerStatus *>(pView);
			if(pServer)
			{
				time_t t;
				if(time(&t) - pServer->m_LastUpdate>m_RefreshCount)
				{
					//SendMessage(WM_COMMAND,ID_REFRESHTIME_REFRESHNOW);
				}
			}
		}
	}
	return 0;	
}

LRESULT CMainFrame::OnFrameMessage( UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/ )
{
	if(wParam == WF_SETACTIVEPAGE)
	{
		for(int n=0; n<m_view.GetPageCount(); n++)
		{
			if((LPVOID)lParam == m_view.GetPageData(n))
			{
				m_view.SetActivePage(n);
				SendMessage(WM_COMMAND, ID_REFRESHTIME_REFRESHNOW);
				break;
			}
		}
	}
	return 0;
}

void CMainFrame::RefreshStatusBar()
{	
	m_pCPU->Update();

	FLOAT CpuUsage = m_pCPU->GetSysUsage(FALSE);
	CString s;
	s.Format(L"CPU: %.03f%% - %.03f%%", CpuUsage, m_pCPU->GetUsage( FALSE ));
	m_pCPU->Save();
	m_sb.SetPaneText(IDP_CPU,(LPCTSTR)s);

	MEMORYSTATUS mem;
	GlobalMemoryStatus(&mem);
	s.Format(L"Memory: %dK / %d/K,  %02d%%", (mem.dwTotalPageFile>>10)-(mem.dwAvailPageFile>>10), mem.dwTotalPageFile>>10, mem.dwMemoryLoad);
	m_sb.SetPaneText(IDP_MEM, (LPCTSTR)s);


	m_pNet->Update();
	s.Format(L"In: %.002fk/s, Out: %.002fk/s", (float)m_pNet->m_IfRow[m_CurrentSelNetIfIndex].InSpeed/1024.0f,  (float)m_pNet->m_IfRow[m_CurrentSelNetIfIndex].OutSpeed/1024.0f);
	m_sb.SetPaneText(IDP_NETUSAGE, (LPCTSTR)s);
}

LRESULT CMainFrame::OnStatusBarLButtonDown( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam )
{

	int xPos = GET_X_LPARAM(lParam); 
	int yPos = GET_Y_LPARAM(lParam); 
	CRect rc;
	m_sb.GetPaneRect( IDP_NETUSAGE, &rc );
	if(rc.PtInRect(CPoint(xPos, yPos)))
	{
		// show menu
		m_sb.ClientToScreen(&rc);


		MENUITEMINFO mii={0};
		mii.cbSize = sizeof(mii);
		

		for(int a=0; a<m_NetPopup.GetMenuItemCount(); a++)
		{
			if(a == m_CurrentSelNetIfIndex) continue;
			mii.fMask = MIIM_STATE;
			mii.fState = MFS_UNCHECKED;
			m_NetPopup.SetMenuItemInfo(a,TRUE, &mii);
		}
		mii.fMask = MIIM_STATE;
		mii.fState = MFS_CHECKED;
		m_NetPopup.SetMenuItemInfo(m_CurrentSelNetIfIndex,TRUE, &mii);
		int sel = m_CmdBar.TrackPopupMenu(m_NetPopup,TPM_NONOTIFY | TPM_RETURNCMD, rc.left, rc.top );
		if(sel!=0)
			m_CurrentSelNetIfIndex = sel - 1;
	}
	return 0;
}

