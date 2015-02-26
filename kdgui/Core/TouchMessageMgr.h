
static bool WINAPI __FindControlByPointAndDispatchMsg(UINode* node, LPVOID pData, LPVOID pProcData) {
	FloatPoint* pPoint = static_cast<FloatPoint*>(pData);
	if (!node->BoundingRectAbsolute().contains((int)pPoint->x(), (int)pPoint->y()))
		return false;

	// ������������ͣ�����Ҫ�ɷ���Ϣ�������ܽ��ܽ��㣬Ҳ�����ò�ѯ�����������ж�
	node->ref(); // ��ֹ
	if (node->IsContainer()) { 
		TEventUI* event = (TEventUI*)pProcData;
		node->Event(*event, TRUE);
		node->deref();
		return false;
	}

	int ref = node->GetRef();
	node->deref();
	return (0 != ref); // Ϊ0��ʾ���ɷ���Ϣ�Ĺ����д˽ڵ㱻�ɵ���
}

// �Ӹ��ڵ�һ·�ɷ������սڵ�,Ҫ�����顣��Щ��Ϣ�����ܱ߲�ѯ���ɷ�������mouseout����
// �ҵ�Ŀ��ڵ����Ҫ�Ӹ��ڵ�һ·����Ϣ��ȥ������ÿ�����ڵ㶼Ҫ���ü�һ����ֹ���¼��ɷ��еĽű���������
/*static*/ BOOL DispathMsgCaptureAndBubbling(UINode* node, const TEventUI& event) {
	TEventUI eventDummy = event;
	eventDummy.dwTimestamp = ::GetTickCount();
	WTF::Vector<UINode*> dispathedMsgNodes;
	BOOL bNotCallDefWindowProc = FALSE; // ֻҪ��һ��ؼ���������defProc�������
LOGI("DispathMsgCaptureAndBubbling 1~");
	node->ref();
LOGI("DispathMsgCaptureAndBubbling 2~");
	UINode* parent = node;
	while (parent) { // ���ռ�
LOGI("DispathMsgCaptureAndBubbling 3~");
		dispathedMsgNodes.append(parent);
		parent = parent->GetParent();
	}
LOGI("DispathMsgCaptureAndBubbling 4~");
	eventDummy.pTarget = node;
	for (int i = (int)dispathedMsgNodes.size() - 1; i >= 0; --i) {
		if (i == 0)
			break;

		eventDummy.pCurrentTarget = dispathedMsgNodes[i];
		eventDummy.bNotHandle = FALSE;
		dispathedMsgNodes[i]->Event(eventDummy, TRUE);

		if (eventDummy.bNotCallDefWindowProc)
			bNotCallDefWindowProc = TRUE;

		if (eventDummy.bNotHandle)
			break;
	}
LOGI("DispathMsgCaptureAndBubbling 3~");
	for (size_t i = 0; i < dispathedMsgNodes.size(); ++i) {
		eventDummy.pCurrentTarget = dispathedMsgNodes[i];
		UINode* n = dispathedMsgNodes[i];
		eventDummy.bNotHandle = FALSE;

		n->Event(eventDummy, FALSE);

		if (eventDummy.bNotCallDefWindowProc)
			bNotCallDefWindowProc = TRUE;

		if (eventDummy.bNotHandle)
			break;
	}

	for (size_t i = 0; i < dispathedMsgNodes.size(); ++i)
		dispathedMsgNodes[i]->deref();

	return bNotCallDefWindowProc;
}

class MessageMgr {
private:
	CPageManager* m_pManager;
	WTF::Vector<UINode*, 4> m_fireEventingNodes;
	//
	RefPtr<UINode> m_pFocus;
	RefPtr<UINode> m_pEventHover;
	RefPtr<UINode> m_pEventClick;
	RefPtr<UINode> m_pEventKey;
	//
	POINT m_ptLastMousePos;
	SIZE m_szMinWindow;
	//UINT m_uMsgMouseWheel;
	bool m_bFirstLayout;
	bool m_bFocusNeeded;
	bool m_bMouseTracking;
	bool m_bMouseIn;

	bool m_bIsDraggableRegionNcHitTest;
	IntPoint m_lastPosForDrag;

public:
	MessageMgr (CPageManager* pManager)
		: m_pManager(pManager)
		, m_bFirstLayout(true)
		, m_bFocusNeeded(false)
		, m_bMouseTracking(false) 
		, m_bIsDraggableRegionNcHitTest(false)
		, m_bMouseIn(false) {
		m_szMinWindow.cx = 140;
		m_szMinWindow.cy = 200;
		m_ptLastMousePos.x = m_ptLastMousePos.y = -1;
	}

	void Init() {
		m_bFirstLayout = true;
		m_bFocusNeeded = true;

		ResetNode();
	}

	void Uninit() {
		ResetNode();
	}

	void ResetNode() {
		m_pFocus = NULL;
		m_pEventKey = NULL;
		m_pEventHover = NULL;
		m_pEventClick = NULL;
	}

	void CheckForReapObjects(UINode* pControl) {
		KDASSERT(pControl != m_pEventKey.get());
		KDASSERT(pControl != m_pEventHover.get());
		KDASSERT(pControl != m_pEventClick.get());
		KDASSERT(pControl != m_pFocus.get());
	}

	void DispatchToDocNode(const TEventUI& evt) {
		TEventUI eventDummy = evt;

		UIEventType oldType = evt.Type;
		// ����������յ�mousedwon��������mousover
		if (!m_bMouseIn && UIEVENT_MOUSEENTER != evt.Type) {
			eventDummy.Type = UIEVENT_MOUSEENTER;
			DoDispatchToDocNode(eventDummy);
		}

		m_bMouseIn = UIEVENT_MOUSELEAVE != oldType;
		
		eventDummy.Type = oldType;
		DoDispatchToDocNode(eventDummy);
	}

	void DoDispatchToDocNode(TEventUI& evt) {
		UINode* docNode = m_pManager->GetDocNodeTemporary();
		if (!docNode->HaveListenersByEventType(evt.Type))
			return;
		
		docNode->Event(evt, true);
		docNode->Event(evt, false);
	}

	PassRefPtr<UINode> FindControlByPointButNoDispatchMsg(FloatPoint pt, const TEventUI* event) const {
		LOGI("FindControlByPointButNoDispatchMsg 1:%x ", m_pManager);
		RefPtr<UINode> node = m_pManager->GetRootTemporary()->FindControl(NULL, &pt, UIFIND_VISIBLE|UIFIND_HITTEST, (LPVOID)event);
		LOGI("FindControlByPointButNoDispatchMsg 3:%x %x", m_pManager, m_pManager->GetRootTemporary());
		if (!node.get())
			return NULL;

		LOGI("FindControlByPointButNoDispatchMsg 4:%x %x", m_pManager, m_pManager->GetRootTemporary());

		return node;
	}

	void handleTouchesBegin(int num, int ids[], float xs[], float ys[]) {
		for (int i = 0; i < num; ++i) {
			POINT pt = {xs[i], ys[i]};
			m_ptLastMousePos = pt;

			TEventUI event;
			event.ptMouse = pt;
			event.Type = UIEVENT_TOUCHBEGIN;
			event.dwTimestamp = ::GetTickCount();

			DispatchToDocNode(event);
			RefPtr<UINode> pNewHover = FindControlByPointButNoDispatchMsg(FloatPoint(pt), &event);
			if (pNewHover.get()) {
				LOGI("handleTouchesBegin 1~~:%x ", pNewHover.get());
				DispathMsgCaptureAndBubbling(pNewHover.get(), event);
				LOGI("handleTouchesBegin 2~:%x ", m_pManager);
			}
		}
	}

	void handleTouchesMove(int num, int ids[], float xs[], float ys[]) {
// 		for (int i = 0; i < num; ++i) {
// 			POINT pt = {xs[i], ys[i]};
// 			m_ptLastMousePos = pt;
// 
// 			TEventUI event;
// 			event.ptMouse = pt;
// 			event.Type = UIEVENT_TOUCHMOVE;
// 			event.dwTimestamp = ::GetTickCount();
// 
// 			DispatchToDocNode(event);
// 			RefPtr<UINode> pNewHover = FindControlByPointButNoDispatchMsg(FloatPoint(pt), &event);
// 			DispathMsgCaptureAndBubbling(pNewHover.get(), event);
// 		}
	}

	void handleTouchesEnd(int num, int ids[], float xs[], float ys[]) {
// 		for (int i = 0; i < num; ++i) {
// 			POINT pt = {xs[i], ys[i]};
// 			m_ptLastMousePos = pt;
// 
// 			TEventUI event;
// 			event.ptMouse = pt;
// 			event.Type = UIEVENT_TOUCHEND;
// 			event.dwTimestamp = ::GetTickCount();
// 
// 			DispatchToDocNode(event);
// 			RefPtr<UINode> pNewHover = FindControlByPointButNoDispatchMsg(FloatPoint(pt), &event);
// 			DispathMsgCaptureAndBubbling(pNewHover.get(), event);
// 		}
	}

	void handleTouchesCancel(int num, int ids[], float xs[], float ys[]) {
// 		for (int i = 0; i < num; ++i) {
// 			POINT pt = {xs[i], ys[i]};
// 			m_ptLastMousePos = pt;
// 
// 			TEventUI event;
// 			event.ptMouse = pt;
// 			event.Type = UIEVENT_TOUCANCEL;
// 			event.dwTimestamp = ::GetTickCount();
// 
// 			DispatchToDocNode(event);
// 			RefPtr<UINode> pNewHover = FindControlByPointButNoDispatchMsg(FloatPoint(pt), &event);
// 			DispathMsgCaptureAndBubbling(pNewHover.get(), event);
// 		}
	}
};