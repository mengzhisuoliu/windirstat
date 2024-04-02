// WinDirStat.cpp - Implementation of CDirStatApp and some globals
//
// WinDirStat - Directory Statistics
// Copyright (C) 2003-2005 Bernhard Seifert
// Copyright (C) 2004-2024 WinDirStat Team (windirstat.net)
// Copyright (C) 2010 Chris Wimmer
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//

#include "stdafx.h"
#include "WinDirStat.h"
#include <common/MdExceptions.h>
#include <common/CommonHelpers.h>
#include "MainFrame.h"
#include "selectdrivesdlg.h"
#include "AboutDlg.h"
#include "DirStatDoc.h"
#include "GraphView.h"
#include "OsSpecific.h"
#include "GlobalHelpers.h"
#include "Item.h"
#include "Localization.h"
#include "SmartPointer.h"

#ifdef _DEBUG
#   include <common/tracer.cpp>
#endif

CMainFrame* GetMainFrame()
{
    // Not: return (CMainFrame *)AfxGetMainWnd();
    // because CWinAppEx::m_pMainWnd is set too late.
    return CMainFrame::GetTheFrame();
}

CDirStatApp* GetWDSApp()
{
    return static_cast<CDirStatApp*>(AfxGetApp());
}

CStringW GetAuthorEmail()
{
    return Localization::LookupNeutral(IDS_AUTHOR_EMAIL);
}

CStringW GetWinDirStatHomepage()
{
    return Localization::LookupNeutral(IDS_AUTHOR_WEBSITE);
}

CMyImageList* GetMyImageList()
{
    return GetWDSApp()->GetMyImageList();
}

// CDirStatApp

BEGIN_MESSAGE_MAP(CDirStatApp, CWinAppEx)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    ON_COMMAND(ID_FILE_SELECT, OnFileOpen)
    ON_COMMAND(ID_RUN_ELEVATED, OnRunElevated)
    ON_UPDATE_COMMAND_UI(ID_RUN_ELEVATED, OnUpdateRunElevated)
    ON_COMMAND(ID_HELP_MANUAL, OnHelpManual)
END_MESSAGE_MAP()

const CDirStatApp _theApp;

CDirStatApp::CDirStatApp() :
        m_altColor(GetAlternativeColor(RGB(0x00, 0x00, 0xFF), L"AltColor"))
      , m_altEncryptionColor(GetAlternativeColor(RGB(0x00, 0x80, 0x00), L"AltEncryptionColor"))
#   ifdef VTRACE_TO_CONSOLE
      , m_vtrace_console(new CWDSTracerConsole())
#   endif // VTRACE_TO_CONSOLE
{
}

CMyImageList* CDirStatApp::GetMyImageList()
{
    m_myImageList.initialize();
    return &m_myImageList;
}

void CDirStatApp::RestartApplication()
{
    // First, try to create the suspended process
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (const BOOL success = CreateProcess(GetAppFileName(), nullptr, nullptr, nullptr, false, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi); !success)
    {
        CStringW s;
        s.FormatMessage(Localization::Lookup(IDS_CREATEPROCESSsFAILEDs), GetAppFileName().GetString(), MdGetWinErrorText(::GetLastError()).GetString());
        AfxMessageBox(s);
        return;
    }

    // We _send_ the WM_CLOSE here to ensure that all COptions-Settings
    // like column widths an so on are saved before the new instance is resumed.
    // This will post a WM_QUIT message.
    (void)GetMainFrame()->SendMessage(WM_CLOSE);
    if (const DWORD dw = ::ResumeThread(pi.hThread); dw != 1)
    {
        VTRACE(L"ResumeThread() didn't return 1");
    }

    ::CloseHandle(pi.hProcess);
    ::CloseHandle(pi.hThread);
}

bool CDirStatApp::getDiskFreeSpace(LPCWSTR pszRootPath, ULONGLONG& total, ULONGLONG& unused)
{
    ULARGE_INTEGER u64total = {{0, 0}};
    ULARGE_INTEGER u64free = {{0, 0}};

    const BOOL b = GetDiskFreeSpaceEx(pszRootPath, nullptr, &u64total, &u64free);
    if (!b)
    {
        VTRACE(L"GetDiskFreeSpaceEx(%s) failed.", pszRootPath);
    }

    // FIXME: need to retrieve total via IOCTL_DISK_GET_PARTITION_INFO instead
    total  = u64total.QuadPart;
    unused = u64free.QuadPart;

    // Race condition ...
    ASSERT(unused <= total);
    return FALSE != b;
}

void CDirStatApp::ReReadMountPoints()
{
    m_mountPoints.Initialize();
}

bool CDirStatApp::IsVolumeMountPoint(const CStringW& path) const
{
    return m_mountPoints.IsVolumeMountPoint(path);
}

bool CDirStatApp::IsFolderJunction(DWORD attr) const
{
    return m_mountPoints.IsFolderJunction(attr);
}

// Get the alternative colors for compressed and encrypted files/folders.
// This function uses either the value defined in the Explorer configuration
// or the default color values.
COLORREF CDirStatApp::GetAlternativeColor(COLORREF clrDefault, LPCWSTR which)
{
    COLORREF x;
    DWORD cbValue = sizeof(x);
    CRegKey key;

    // Open the explorer key
    key.Open(HKEY_CURRENT_USER, wds::strExplorerKey, KEY_READ);

    // Try to read the REG_BINARY value
    if (ERROR_SUCCESS == key.QueryBinaryValue(which, &x, &cbValue))
    {
        // Return the read value upon success
        return x;
    }

    // Return the default upon failure
    return clrDefault;
}

COLORREF CDirStatApp::AltColor() const
{
    // Return property value
    return m_altColor;
}

COLORREF CDirStatApp::AltEncryptionColor() const
{
    // Return property value
    return m_altEncryptionColor;
}

CStringW CDirStatApp::GetCurrentProcessMemoryInfo()
{
    // Fetch current working set
    PROCESS_MEMORY_COUNTERS pmc = { sizeof(pmc) };
    if (!::GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    {
        return wds::strEmpty;
    }

    static CStringW memformat = Localization::Lookup(IDS_RAMUSAGEs);
    CStringW formatted;
    formatted.FormatMessage(memformat, FormatBytes(pmc.WorkingSetSize).GetString());
    return L"    " + formatted;
}

bool CDirStatApp::InPortableMode() const
{
    return GetFileAttributes(GetAppFileName(L"ini")) != INVALID_FILE_ATTRIBUTES;
}

bool CDirStatApp::SetPortableMode(bool enable, bool only_open)
{
    // If portable mode is enabled, then just ensure the full path is used
    const CStringW ini = GetAppFileName(L"ini");
    if (ini == m_pszProfileName &&
        enable == InPortableMode())
    {
        return true;
    }

    // Cleanup previous configuration
    if (m_pszRegistryKey != nullptr) free(LPVOID(m_pszRegistryKey));
    if (m_pszProfileName != nullptr) free(LPVOID(m_pszProfileName));
    m_pszProfileName = nullptr;
    m_pszRegistryKey = nullptr;
    
    if (enable)
    {
        // Enable portable mode by creating the file
        const HANDLE ini_handle = CreateFile(ini, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ,
            nullptr, only_open ? OPEN_EXISTING : OPEN_ALWAYS , 0, nullptr);
        if (ini_handle != INVALID_HANDLE_VALUE)
        {
            // Open successful, setup settings to store to file
            CloseHandle(ini_handle);
            m_pszProfileName = _wcsdup(ini);
            return true;
        }

        // Fallback to registry mode for any failures
        SetRegistryKey(Localization::Lookup(IDS_APP_TITLE));
        return false;
    }

    // Attempt to remove file succeeded
    if (DeleteFile(ini) != 0 || GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        SetRegistryKey(Localization::Lookup(IDS_APP_TITLE));
        return true;
    }

    // Deletion failed  - go back to ini mode
    m_pszProfileName = _wcsdup(ini);
    return false;
}

CString AFXGetRegPath(LPCTSTR lpszPostFix, LPCTSTR)
{
    // This overrides an internal MFC function that causes CWinAppEx
    // to malfunction when operated in portable mode
    return CStringW(L"Software\\WinDirStat\\WinDirStat\\") + lpszPostFix + L"\\";
}

BOOL CDirStatApp::InitInstance()
{
    // Prevent state saving
    m_bSaveState = FALSE;

    CWinAppEx::InitInstance();
    CWinAppEx::InitShellManager();

    // Load default language just to get bootstrapped
    Localization::LoadResource(MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL));

    // Initialize visual controls
    constexpr INITCOMMONCONTROLSEX ctrls = { sizeof(INITCOMMONCONTROLSEX) , ICC_STANDARD_CLASSES };
    (void)::InitCommonControlsEx(&ctrls);
    (void)::OleInitialize(nullptr);
    (void)::AfxOleInit();
    ::AfxEnableControlContainer();
    (void)::AfxInitRichEdit2();
    CWinAppEx::EnableHtmlHelp();

    // If a local config file is available, use that for settings
    SetPortableMode(true, true);

    COptions::LoadAppSettings();
    CWinAppEx::LoadStdProfileSettings(4);

    m_pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(CDirStatDoc),
        RUNTIME_CLASS(CMainFrame),
        RUNTIME_CLASS(CGraphView));
    if (!m_pDocTemplate)
    {
        return FALSE;
    }
    AddDocTemplate(m_pDocTemplate);

    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);
    if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileOpen)
    {
        // Use the default a new document since the shell processor will fault
        // interpreting the complex configuration string we pass as a document name
        CCommandLineInfo cmdAlt;
        ProcessShellCommand(cmdAlt);
    }
    else
    {
        if (!ProcessShellCommand(cmdInfo))
            return FALSE;
    }

    FileIconInit(TRUE);

    GetMainFrame()->InitialShowWindow();
    m_pMainWnd->UpdateWindow();

    // When called by setup.exe, WinDirStat remained in the
    // background, so force it to the foreground
    m_pMainWnd->BringWindowToTop();
    m_pMainWnd->SetForegroundWindow();

    // Attempt to enable backup / restore privileges if running as admin
    if (COptions::UseBackupRestore && !EnableReadPrivileges())
    {
        VTRACE(L"Failed to enable additional privileges.");
    }

    if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileOpen)
    {
        // Terminate parent process that called us
        int token = 0;
        const DWORD parent = wcstoul(cmdInfo.m_strFileName.Tokenize(L"|", token), nullptr, 10);
        cmdInfo.m_strFileName = cmdInfo.m_strFileName.Right(cmdInfo.m_strFileName.GetLength() - token);
        if (SmartPointer<HANDLE> handle(CloseHandle, OpenProcess(PROCESS_TERMINATE, FALSE, parent)); handle != nullptr)
        {
            TerminateProcess(handle, 0);
        }

        m_pDocTemplate->OpenDocumentFile(cmdInfo.m_strFileName, true);
    }
    else
    {
        OnFileOpen();
    }

    return TRUE;
}

void CDirStatApp::OnAppAbout()
{
    StartAboutDialog();
}

void CDirStatApp::OnFileOpen()
{
    CSelectDrivesDlg dlg;
    if (IDOK == dlg.DoModal())
    {
        const CStringW path = CDirStatDoc::EncodeSelection(static_cast<RADIO>(dlg.m_radio), dlg.m_folderName, dlg.m_drives);
        m_pDocTemplate->OpenDocumentFile(path, true);
    }
}

void CDirStatApp::OnUpdateRunElevated(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!IsAdmin());
}

void CDirStatApp::OnRunElevated()
{
    // For the configuration to launch, include the parent process so we can
    // terminate it once launched from the child process
    const CStringW sAppName = GetAppFileName();
    CStringW launchConfig;
    launchConfig.Format(L"%lu|%s", GetCurrentProcessId(), GetDocument()->GetPathName().GetString());

    SHELLEXECUTEINFO shellInfo;
    ZeroMemory(&shellInfo, sizeof(shellInfo));
    shellInfo.cbSize = sizeof(shellInfo);
    shellInfo.fMask  = SEE_MASK_DEFAULT;
    shellInfo.lpFile = sAppName;
    shellInfo.lpVerb = L"runas";
    shellInfo.nShow  = SW_NORMAL;
    shellInfo.lpParameters = launchConfig.GetString();

    if (!::ShellExecuteEx(&shellInfo))
    {
        VTRACE(L"ShellExecuteEx failed to elevate %d", GetLastError());
    }
}

void CDirStatApp::OnHelpManual()
{
    // FIXME: open browser, point to Wiki (via windirstat.net short link), based on current language
    DoContextHelp(IDH_StartPage);
}

void CDirStatApp::DoContextHelp(DWORD)
{
    CStringW msg;
    msg.FormatMessage(Localization::Lookup(IDS_HELPFILEsCOULDNOTBEFOUND), L"windirstat.chm");
    AfxMessageBox(msg);
}
