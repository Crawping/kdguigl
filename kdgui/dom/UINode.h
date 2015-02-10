#ifndef UINode_h
#define UINode_h

#include "UINodeEnum.h"
#include <wtf/Timer.h>
#include "Core/UIEventType.h"
#include "style/NodeAttrDef.h"

class UINode;
class GraphicsContext;
class NodeStyle;
class EventListener;
class ListenersMgr;
class NodeAnim;

typedef bool (WINAPI* FINDCONTROLPROC)(UINode*, LPVOID, LPVOID);

class KqNAdditionalData {
public:
	virtual void Uninit(bool bIsScriptDestroying) = 0;
};

class UINode {
public:
	UINode();

protected:
	virtual ~UINode();

public:
	virtual int ref();
	virtual void deref();
	int GetRef() {return m_ref;}

	void DelayDeref();

	virtual BOOL DestroyTree();

	static void RecursionClearAllBindAndAdditionalData(UINode* root);
	void ClearAllBindAndAdditionalData();

	virtual CStdString GetName() const;
	virtual void SetName(LPCTSTR pstrName);
	virtual LPVOID GetInterface(LPCTSTR pstrName);

	bool IsEqualToTheClassName(LPCTSTR className) const;

	// ��̬�ӿڣ��������ʵ�����⹦�ܡ�
	virtual int Dispatch(LPCTSTR func, int nArg, LPCTSTR strArg, void* arg, void* ret) {return -1;}

	virtual CStdString GetToolTip() const;
	virtual void SetToolTip(LPCTSTR pstrText);

	virtual void SetFocus();

	virtual BOOL IsContainer() const {return FALSE;}

	virtual PassRefPtr<UINode> FindControl(FINDCONTROLPROC Proc, LPVOID pData, UINT uFlags, LPVOID pProcData);

	virtual CPageManager* GetManager() const;
	virtual void SetManager(CPageManager* pManager);

	virtual bool IsResNode() const {return false;}

	// ע�⣬���漰��DOM�ڵ�Ļ�ȡ�����ü��������һ������ʹ����Ҫ�ǵ��ͷ�
	virtual void SetParent(UINode* pParent);
	virtual UINode* GetParent() const;
	virtual UINode* GetParentTemp() const; // ���������ü�����Ϊ���Ż�

	// Ψһ���������������������һ
	WTF::Vector<UINode*>* GetChilds() {return &m_children;}

	virtual bool AppendChild(UINode* pControl);
	virtual bool RemoveChild(UINode* pControl);

	virtual UINode* Prev();
	virtual UINode* Sibling();
	virtual UINode* FirstChild();
	virtual UINode* LastChild();

	void SetWidget(KqNAdditionalData* widget);
	KqNAdditionalData* Widget();

	void SetUserdata(KqNAdditionalData* userdata);
	KqNAdditionalData* Userdata();

	UINode* FindByClass(const SQChar* value) const;

	virtual void Paint(GraphicsContext* g, const IntRect &rcInvalid);

	// ��������任����߿򣬵��ӽڵ������任
	virtual IntRect BoundingRectInLocalCoordinates();
	// ��Ը��ڵ�ľ�������
	virtual IntRect BoundingRectAbsolute();
	
	virtual void Layout(IntRect rc);
	virtual void SetNeedLayout();

	virtual void Invalidate();
	// bForce����˼�ǣ���ʹ���ڵ㱻�����ˣ���ȻҪˢ��
	virtual void InvalidateRelativeRect(const IntRect& rc, bool bForce);
	// ˢ�¾��Σ��ύ���Ǿ�������
	virtual void InvalidateAbsoluteRect(const IntRect& rc, bool bForce);

	virtual void OnChildInvalidateRelativeRect(const IntRect& rc, bool bForce);
	virtual void OnChildInvalidateAbsoluteRect(const IntRect& rc, bool bForce);

	void UpdateLayout();

	virtual void Init();
	virtual int Event(TEventUI& event, BOOL bCapturePhase);
	virtual int DispatchEventToListeners(TEventUI& event, BOOL bCapturePhase);

	bool AddEventListener(UIEventType eventType, PassRefPtr<EventListener> eventListener);
	void RemoveEventListener(UIEventType eventType, EventListener* eventListener);
	void ClearEventListener();
	bool HaveListenersByEventType(UIEventType Type);

	virtual NodeAttrDef* GetCanRefreshAttrs(NodeAttrDef style) const;
	bool IsCanRefreshAttr(LPCTSTR pstrName);
		
	virtual const NodeStyle* GetAttrs() const {return m_nodeStyle;}
	virtual bool SetAttr(LPCTSTR pstrName, LPCTSTR pstrValue);
	virtual CStdString GetAttr(LPCTSTR pstrName) const;

	virtual void SetAttrBegin();
	virtual void SetAttrEnd();

	virtual void OnChildSetAttrBegin();
	virtual void OnChildSetAttrEnd();
	virtual void OnChildSetAttr(LPCTSTR pstrName, LPCTSTR pstrValue);

	// �����Ļ�ͼ��Դ�仯���罥�����ĸ������Ա仯
	enum ChildState {
		eAttrBegin,
		eAttrEnd,
		eAppendChild,
		eRemoveChild,
	};
	virtual void OnDrawResChange(ChildState childState);
	
	bool HasChildren() {return m_children.size() > 0;}

	virtual LPCTSTR GetClass() const = 0;
	virtual UINodeEnum GetClassEnum() const = 0;

	virtual void SetBoundingDirty();
	virtual void SetRepaintDirty();

	void ClearAnimWhenNoticed (int idx);
	void ForceStopAllAnimAndDestroy();
	NodeAnim* Anim (bool bStopAll);

	// ���ú��ڷ���ʼ�������в������ٽ�����Ϣ
	void SetStateBeginUninit() {m_eState = eBeginDelayUninit;}

	CPageManager* GetManager() {return m_pManager;}

	CStdString Dump();

protected:
	void SetBoundingDirtyImp();

	int m_ref;
	NodeStyle* m_nodeStyle;
	CPageManager* m_pManager;
	UINode* m_pParent;

	CStdString m_sName;
	CStdString m_sToolTip;

	IntRect m_rcItem;
	bool m_bBoundingDirty;
	bool m_bRepaintDirty;
	bool m_bVisibleWhenBeginSetAttr; // ����ʼ�������Ե�ʱ���Ƿ�Ϊ��ʾ״̬�����������ڽ������ã�ˢ�µ�ʱ�򣬲��ᱻ�Ż������ˢ��
	bool m_bStartDelayLayout;
	IntRect m_saveRepaintRectAbsolute; // setAttrʱ��ı�

	bool m_bVisible;
	bool m_bEnabled;
	bool m_bFocused;

	KqNAdditionalData* m_kqWidget;
	KqNAdditionalData* m_kqUserdata;

	WTF::Vector<UINode*> m_children;
	enum State {
		eRunning,
		eBeginDelayUninit, // �ӳٷ���ʼ���Ĺ����У�����Ҫ�ټӼ�������
		eBeginToUninit,
		eUniniting,
		eUninited
	};
	State m_eState; // releaseʱ��������ֹ����

	friend class ListenersMgr;
	ListenersMgr* m_listenerMgr;

	Timer<UINode>* m_delayLayoutTimer;

	Vector<NodeAnim*> m_animObjs;

	CStdString DoDump(int nDeep);
};

#endif // UINode_h