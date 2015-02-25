#if !defined(AFX_UICONTROLS_H__20050423_DB94_1D69_A896_0080AD509054__INCLUDED_)
#define AFX_UICONTROLS_H__20050423_DB94_1D69_A896_0080AD509054__INCLUDED_

#pragma once

/////////////////////////////////////////////////////////////////////////////////////
//
//#include <atlwin.h>
//#include <hash_map>
#include <WTF/HashMap.h>
#include <WTF/Timer.h>

#include "Api/KdGuiApi.h"
#include "UIEventType.h"

// Flags for FindControl()
#define UIFIND_ALL           0x00000000
#define UIFIND_VISIBLE       0x00000001
#define UIFIND_ENABLED       0x00000002
#define UIFIND_HITTEST       0x00000004
#define UIFIND_PASSRESPONSE  0x00000008   // ������Ϣ��͸��
#define UIFIND_ME_FIRST      0x80000000

class UINode;

class ResCache;
class SkBitmap;
struct NVGcontext;

class ScritpMgr;
class EffectsResNodeMgr;
class SysPaintMgr;
class PaintMgr;
class MessageMgr;
class CPageManager;
class IdMgr;
class AnimMgr;
class DbgMgr;

class ThreadTimers;
class DOMTimer;

class PageManagerPublic;

//////////////////////////////////////////////////////////////////////////

struct KWebApiCallbackSet {
	PFN_KdPageCallback m_xmlHaveFinished;
	PFN_KdPageWinMsgCallback m_msgPreCallBack;
	PFN_KdPageWinMsgCallback m_msgPostCallBack;
	PFN_KdPageCallback m_unintCallBack;
	PFN_KdPageScriptInitCallback m_scriptInitCallBack;
	PFN_KdResCallback m_resHandle;
	PFN_KdResCallback m_resOtherNameQuery;
	PFN_KdPagePaintCallback m_paint;
	PFN_KdPageError m_error;
	KWebApiCallbackSet() {
		memset(this, 0, sizeof(KWebApiCallbackSet));
	}
};

class PageManagerDelayTask {
public:
	enum Moment { // ����ʱ������Щ�ص����벻�ڷ���ʼ���е��ã���Щ�ֱ����ڶ�ʱ���е��ã���ű����ػص�
		ePMDTUniniting,
		ePMDTWindowsMsg,
		ePMDTTimer,
		ePMDTEnd,
	};

	virtual void Run(CPageManager* manager, Moment moment) = 0;
	virtual void Destroy() = 0;
};

/////////////////////////////////////////////////////////////////////////////////////
//

class CPageManager {
	WTF_MAKE_FAST_ALLOCATED;
public:
	CPageManager();
	~CPageManager();

public:
	void Init(HWND hWnd);
	void Uninit();

	void SaveScriptDebugInfo(KdPageDebugInfo* info);

	void UpdateLayout();
	void Invalidate(const IntRect& rc);
	void InvalidateAll();

	void SetClientRectAndInvalideta(const IntRect& clientRect);

	void NotifScriptInit();

	// ����ʽ������Դ���ؽű�
	bool LoadSyncScriptFromSrc(const CStdString& src);
	// ������ʽ
	bool LoadAsyncScriptFromSrc(const CStdString& src);

	// ��buf���ؽű�
	bool LoadScriptFromBuf(LPCTSTR src, LPCTSTR scriptBuf, int size);

	void SetPagePtr(void* pagePtr) {m_pagePtr = pagePtr;}
	void* GetPagePtr() {return m_pagePtr;}

	void SetIsAlphaWin() {m_bIsAlphaWin = TRUE;}
	BOOL IsAlphaWin() {return m_bIsAlphaWin;}

	HWND GetPaintWindow() const;
	HWND GetHWND() const;

	bool InitControls(UINode* pControl);
	void ReapObjects(UINode* pControl);

	UINode* GetFocus() const;
	void SetFocus(UINode* pControl);

	PassRefPtr<UINode> FindControlByPointButNoDispatchMsg(FloatPoint pt, const TEventUI* event) const;
	static bool WINAPI __FindControlByPointAndDispatchMsg(UINode* node, LPVOID pData, LPVOID pProcData);

	UINode* GetRoot();
	UINode* GetRootTemporary() {return m_rootNode.get();} // Ϊ���ܿ��ǣ�����һ����д���ü�����

	UINode* GetDocNode();
	UINode* GetDocNodeTemporary() {return m_docNode.get();}

	int GetImageResBySrc(NVGcontext* ctx, const CStdString& pSrc, UINode* owner);

	// ��䡢�ɰ���Ҫ
	UINode* GetEffectsResNodeByUrl(LPCTSTR pSrc, UINode* owner);
	void AddEffectsResNode(UINode* resNode);
	void RemoveEffectsResNode(UINode* resNode);
	WTF::Vector<UINode*>* GetEffectsResOwnerNodes(UINode* resNode);
	void RemoveEffectsResOwnerNode(UINode* ownerNode);
	// �������ӽڵ��ʱ���п����ӽڵ����ڱ��ű�ռ�ã���ʱ�������٣�
	// ����ʱ�����Ѷ�Ӧ����Դ�����ͷţ������´������Դ��ʱ����ظ�
	void ReapResources(UINode* pControl); 
	void ReapNodeTreeResources(UINode* pControl); // ��������������Դ��Ӧ��ͨ����UINode����������

	// ���ü������һ
	UINode* GetNodeByID(const CStdString& id);
	// ���ü�������
	void AddToIdMap(const SQChar* id, UINode* n);
	void RemoveIdMapByNode(UINode* n);

	ScritpMgr* GetScriptMgr() {return m_scriptMgr;}
	HSQUIRRELVM GetVM();

	KWebApiCallbackSet m_callbacks;

	DOMTimer* GetDOMTimer();
	ThreadTimers* GetThreadTimers() {return m_threadTimers;}

	void AppendAnimNode(UINode* node);
	void RemoveAnimNode(UINode* node);
	void ForceStopAllAnim();

	void ScheduleTaskForHeartBeat(UINT uMsg);
	void ScheduleTasks(Vector<PageManagerDelayTask*>& delayTasks, PageManagerDelayTask::Moment moment);
	void ScheduleAllTasks(PageManagerDelayTask::Moment moment);
	void PostDelayTask(PageManagerDelayTask* task);
	void PostAsysTask(PageManagerDelayTask* task);

 	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNCDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	BOOL PreProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);
	BOOL ProcessDbgMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);

	int  OnInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	bool InputEventToRichEdit(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void SetBackgroundColor(COLORREF c);

	void NotifSvgInited();
	bool HasSvgInit() {return m_bHasSvgInit;}

	bool HasJsonReady() {return m_bHasJonsReady;}
	void SetJonsReady() {m_bHasJonsReady = true;}

	PageManagerPublic* GetWrap() {return m_selfWrap;}

	void CopyMemoryDC(HDC hDC, const RECT* rc);

	CStdString DumpNode();

	void SetLayerWindow(bool b);

// 	bool IsRootNode(UINode* n) {return n == m_rootNode.get();}
// 	bool IsFocusNode(UINode* n) {return n == m_pFocus.get();}
// 	bool IsEventHoverNode(UINode* n) {return n == m_pEventHover.get();}
// 	bool IsEventClickNode(UINode* n) {return n == m_pEventClick.get();}
// 	bool IsEventKeyNode(UINode* n) {return n == m_pEventKey.get();}

	void PushFireEventNode(UINode* n); // �˽ڵ������ɷ��¼���������ɾ��������ʱ��Ҳ���������
	void EraseFireEventNode(UINode* n);
	int IsFireEventNode(UINode* n);

	void SetIsDraggableRegionNcHitTest();

	void ResetRequestRender();
	bool IsRequestRender();

	NVGcontext* GetCanvas();

	void HighLightRing(UINode* n);

	enum State {
		eNoInit,
		eRunning,
		eUniniting,
		eScriptDestroying,
		eUninit
	};
	State GetState() {return m_eState;}

	// �ڹرսű��������ʱ�򣬿��ܻ���Ϊɾ���հ�����ɾ���ڵ㡣
	// ��Щ�ڵ��Ѿ����Ƴ���dom tree������Ϊ���հ����ã����Բ�δ���ӳٻص����������٣���������
	// �ű��رն����١���ʱ�ű����Ѿ����رգ��������ٽڵ�������������¼��������У������ٲ����ű�Ԫ��
	bool IsScriptDestroying() {return eScriptDestroying == m_eState;}

	PaintMgr* GetPaintMgr() {return m_paintMgr;}

	CStdString m_docURI; // �����á�����֪����ǰ���ĸ�����

	bool IsLayerWindow();

	void MainLoop();

	void handleTouchesBegin(int num, int ids[], float xs[], float ys[]);
	void handleTouchesMove(int num, int ids[], float xs[], float ys[]);
	void handleTouchesEnd(int num, int ids[], float xs[], float ys[]);
	void handleTouchesCancel(int num, int ids[], float xs[], float ys[]);

private:
	void InitMgrs();

#ifdef _MSC_VER
	BEGIN_MSG_MAP(CPageManager)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_NCDESTROY, OnNCDestroy)
	END_MSG_MAP()
#else
	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult) {return TRUE;}
#endif // _MSC_VER

	LRESULT OnDumpNode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnShowHightLightRing(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	RefPtr<UINode> m_rootNode;
	RefPtr<UINode> m_docNode;

	HWND m_hWndPaint;

	void* m_pagePtr;

#ifdef _MSC_VER
	friend PageManagerPublic;
#endif // _MSC_VER
	PageManagerPublic* m_selfWrap;

	PaintMgr* m_paintMgr;
	ScritpMgr* m_scriptMgr;
	MessageMgr* m_messageMgr;
	EffectsResNodeMgr* m_effectsResNodeMgr;
	IdMgr* m_idMgr;
	AnimMgr* m_animMgr;
	DOMTimer* m_DOMTimerMgr;
	DbgMgr* m_dbgMgr;

	bool m_bHasSvgInit; // �Ƿ�ִ�й��ű���SvgInit
	bool m_bHasJonsReady;  // ���SvgInit�Ѿ�ִ����ϣ�����Ҫ����Ⱦ���һ����ͼ����ã������������������ص�

	State m_eState;

	bool m_bHadNotifScriptInit; // �Ƿ�֪ͨ���ⲿ�ű�������ʼ�����

	BOOL m_bIsAlphaWin;

	ThreadTimers* m_threadTimers;
	
	Vector<PageManagerDelayTask*> m_delayTasks;
	// �ű�����ֻ���ڶ�ʱ����������Ϊ�����Ϣ�ᴥ�����أ����»�û����ջ�ͱ�����
	Vector<PageManagerDelayTask*> m_asysTasks; 

	int m_nProcessWindowMessageCount;
};

#endif // !defined(AFX_UICONTROLS_H__20050423_DB94_1D69_A896_0080AD509054__INCLUDED_)

