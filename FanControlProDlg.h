#pragma once
#include "Core.h"
#include "afxdialogex.h"
#include <afxcontrolbars.h>
#include <afxcmn.h>

// 控件 ID 定义
#define IDC_CHECK_TAKEOVER      1002
#define IDC_CHECK_FORCE         1003
#define IDC_CHECK_LINEAR        1004
#define IDC_EDIT_INTERVAL       1005
#define IDC_EDIT_TRANSITION     1006
#define IDC_EDIT_FORCE_TEMP     1007
#define IDC_CHECK_AUTORUN       1008
#define IDC_EDIT_GPU_FREQUENCY  1009
#define IDC_CHECK_LOCK_GPU      1010
#define IDC_BUTTON_SAVE         1011
#define IDC_BUTTON_RESET        1012
#define IDC_BUTTON_LOAD         1013
#define IDC_BUTTON_ADVANCED     1014
#define IDC_COMBO_MODE          1015
#define IDC_SLIDER_CPU_FAN      1016
#define IDC_SLIDER_GPU_FAN      1017
#define IDC_EDIT_CPU_FAN        1018
#define IDC_EDIT_GPU_FAN        1019
#define IDC_CHECK_SILENT        1020
#define IDC_CHECK_PERFORMANCE   1021
#define IDC_PROGRESS_CPU_TEMP   1022
#define IDC_PROGRESS_GPU_TEMP   1023
#define IDC_STATIC_CPU_TEMP     1024
#define IDC_STATIC_GPU_TEMP     1025
#define IDC_STATIC_CPU_RPM      1026
#define IDC_STATIC_GPU_RPM      1027
#define IDC_STATIC_CPU_USAGE    1028
#define IDC_STATIC_GPU_USAGE    1029
#define IDC_MAX_DUTY_SLIDER     1030
#define IDC_EDIT_MAX_DUTY       1031
#define IDC_STATIC_ADVANCED_GROUP 1033
#define IDC_CHECK_DESKTOP_NOTIFICATIONS 1034
#define IDC_EDIT_WARNING_TEMP 1035
#define IDC_EDIT_NOTIFICATION_COOLDOWN 1036
#define IDC_STATIC_ADVANCED_UPDATE 1037
#define IDC_STATIC_ADVANCED_TRANSITION 1038
#define IDC_STATIC_ADVANCED_FORCE 1039
#define IDC_STATIC_ADVANCED_FREQUENCY 1040
#define IDC_STATIC_ADVANCED_WARNING 1041
#define IDC_STATIC_ADVANCED_COOLDOWN 1042
#define IDC_STATIC_ADVANCED_PRESETS 1043
#define IDC_STATIC_ADVANCED_MAX_DUTY 1044
#define IDC_STATIC_CONTROL_STATUS 1045
#define IDC_STATIC_WARNING_STATUS 1046

// 托盘菜单 ID
#define IDR_SHOW    11
#define IDR_EXIT    12
#define IDR_FORCED  13

// CFanControlProDlg 对话框
class CFanControlProDlg : public CDialogEx
{
    // 构造
public:
    CFanControlProDlg(CWnd* pParent = NULL);
    virtual ~CFanControlProDlg();

    // 对话框数据
    enum { IDD = IDD_FANCONTROLPRO_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);

    // 实现
protected:
HICON m_hIcon;
     CCore m_core;
     HANDLE m_hCoreThread;
    HANDLE m_hSingleInstanceMutex;
    int m_nLastCoreUpdateTime;
    BOOL m_bWindowVisible;
    BOOL m_bAdvancedMode;
    int m_nWindowSize[2];
    BOOL m_bForceHideWindow;
    int m_nCheckThreadCount;   // 线程检查计数（替代 static 局部变量）
    BOOL m_bLastVisible;       // 上次窗口可见状态（替代 static 局部变量）
    BOOL m_bTrayAdded;         // 托盘图标是否已添加（替代 static 局部变量）
    int m_nTrayLastUpdate;     // 托盘上次更新时间（替代 static 局部变量）
    
    // 控件变量
    CButton m_ctlTakeOver;
    CButton m_ctlForcedCooling;
    CButton m_ctlLinear;
    CEdit m_ctlInterval;
    CEdit m_ctlTransition;
    CEdit m_ctlForceTemp;
    CButton m_ctlAutorun;
    CEdit m_ctlFrequency;
    CButton m_ctlLockGpu;
    CComboBox m_ctlMode;
    CSliderCtrl m_ctlCpuFanSlider;
    CSliderCtrl m_ctlGpuFanSlider;
    CEdit m_ctlCpuFanEdit;
    CEdit m_ctlGpuFanEdit;
    CButton m_ctlSilent;
    CButton m_ctlPerformance;
    CProgressCtrl m_ctlCpuTempProgress;
    CProgressCtrl m_ctlGpuTempProgress;
    CStatic m_ctlCpuTempText;
    CStatic m_ctlGpuTempText;
    CStatic m_ctlCpuRpmText;
    CStatic m_ctlGpuRpmText;
    CStatic m_ctlCpuUsageText;
    CStatic m_ctlGpuUsageText;
    CSliderCtrl m_ctlMaxDutySlider;
    CEdit m_ctlMaxDutyEdit;
    CStatic m_ctlStartupStatus;
    CStatic m_ctlControlStatus;
    CStatic m_ctlWarningStatus;
    CButton m_ctlDesktopNotifications;
    CEdit m_ctlWarningTemp;
    CEdit m_ctlNotificationCooldown;
    
    // 手动模式风扇控件 ID 数组
    int m_nDutyEditCtlID[2][10];

    // 生成的消息映射函数
    DECLARE_MESSAGE_MAP()
    
public:
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg LRESULT OnShowTask(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnDeferredStartup(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnCoreInitResult(WPARAM wParam, LPARAM lParam);
    
    // 按钮事件
    afx_msg void OnBnClickedButtonSave();
    afx_msg void OnBnClickedButtonReset();
    afx_msg void OnBnClickedButtonLoad();
    afx_msg void OnBnClickedButtonAdvanced();
    afx_msg void OnBnClickedCheckTakeover();
    afx_msg void OnBnClickedCheckForce();
    afx_msg void OnBnClickedCheckLinear();
    afx_msg void OnBnClickedCheckAutorun();
    afx_msg void OnBnClickedCheckLockGpu();
    afx_msg void OnBnClickedSilent();
    afx_msg void OnBnClickedPerformance();
    afx_msg void OnCbnSelchangeComboMode();
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    
    // 辅助函数
    void UpdateGui(BOOL bFull);
    void UpdateControlStatus();
    void UpdateWarningStatus();
    void ApplyResponsiveLayout();
    BOOL CheckAndSave();
    void SetTray(PCWSTR string);
    void SetAdvancedMode(BOOL bAdvanced);
    void SetStartupStatus(PCWSTR status);
    void SetTakeoverControlsEnabled(BOOL enabled);
    static DWORD WINAPI CoreThread(LPVOID lParam);
    virtual void OnOK();
    virtual void OnCancel();
    
    // 开机自启
    BOOL SetAutorunReg(BOOL bWrite, BOOL bAutorun = TRUE);
    BOOL SetAutorunTask(BOOL bWrite, BOOL bAutorun = TRUE);
    CStringA ExecuteCmd(CStringA str);
    BOOL CreateTaskXml(PCSTR strXmlPath, PCSTR strTargetPath);
    CString GetExePath();
    BOOL CheckInputFrequency(int nFrequency);
};
