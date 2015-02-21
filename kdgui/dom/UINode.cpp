
#include <UIlib.h>
#include <wtf/UtilHelp.h>
#include <wtf/RefCountedLeakCounter.h>

#include "core/UIManager.h"
#include "UINode.h"
#include "style/NodeStyle.h"
#include "EventListener.h"
#include "ListenersMgr.h"

#include "graphics/GraphicsContext.h"
#include "rendering/RenderSupport.h"

#include "NodeAnim.h"

#include "Core/ScritpMgr.h"
#include "script/include/sqstdaux.h" // for sqstd_printcallstack

#ifndef NDEBUG
#include "WTF/HashSet.h"

static RefCountedLeakCounter UINodeCounter(_SC("UINode"));
WTF::HashSet<UINode*>* g_leaksNodeSet;
WTF::Mutex g_leaksNodeSetMutex;

#endif

// ���ü���һ��ʼΪ0����Ӻ�ɾ���ӽڵ㲻Ӱ�������һ��deref����Ϊ0��û���ڵ㣬��ʼ����
UINode::UINode()
	: m_ref(0)
	, m_pManager(NULL)
	, m_pParent(NULL)
	, m_bVisible(true)
	, m_bFocused(false)
	, m_bEnabled(true)
	, m_kqWidget(NULL)
	, m_kqUserdata(NULL)
	, m_eState(eRunning)
	, m_nodeStyle(new NodeStyle(this))
	, m_listenerMgr(NULL)
	, m_delayLayoutTimer(NULL)
	, m_bStartDelayLayout(false)
	, m_bVisibleWhenBeginSetAttr(true)
	, m_bRepaintDirty(true)
	, m_bBoundingDirty(true) {
#ifndef NDEBUG
	UINodeCounter.increment();

	WTF::MutexLocker lock(g_leaksNodeSetMutex);
	if (!g_leaksNodeSet)
		g_leaksNodeSet = new HashSet<UINode*>();
	g_leaksNodeSet->add(this);
#endif
}

UINode::~UINode() {
#ifndef NDEBUG
	UINodeCounter.decrement();

	WTF::MutexLocker lock(g_leaksNodeSetMutex);
	g_leaksNodeSet->remove(this);
#endif

	m_eState = eUninited;

	KDASSERT(0 == m_children.size());
	if(m_pManager)
		m_pManager->ReapObjects(this);

	// ɾ���ڵ��ʱ��Ҫע����ID MAP���ڵ���ӵ�е���Դ������ͼƬ��Դ������ͽ�����Դ
	ClearEventListener();

	UHDeletePtr(&m_listenerMgr);
	UHDeletePtr(&m_nodeStyle);
	UHDeletePtr(&m_delayLayoutTimer);

	UHDeletePtr(&m_kqWidget);
	UHDeletePtr(&m_kqUserdata);
}

static void CheckRefIsNull(UINode* n) {
	if (!IsDebuggerPresent())
		return;

	CPageManager* manager = n->GetManager();

 	int associateCount = 0;
// 	if (manager->IsRootNode(n))
// 		associateCount++;
// 	if (manager->IsFocusNode(n))
// 		associateCount++;
// 	if (manager->IsEventHoverNode(n))
// 		associateCount++;
// 	if (manager->IsEventClickNode(n))
// 		associateCount++;
// 	if (manager->IsEventKeyNode(n))
// 		associateCount++;
// 
// 	associateCount += manager->IsFireEventNode(n);
	
	if (associateCount != n->GetRef()) {
		CStdString str(_SC("���½ڵ�����ʱ��Ȼ������:"));
		CStdString strTemp;
		strTemp.Format(_SC(" 0x%x"), n);

		str += n->GetAttrs()->m_id;
		str += strTemp;
		str += _SC("\n");
		OutputDebugString(str);
		DebugBreak();
	}
}

int UINode::ref() {
	if (eUniniting == m_eState)
		return m_ref;

	if (eUninited == m_eState) {
		KDASSERT(0);
		return m_ref;
	}

	m_ref++;
	return m_ref;
}

void UINode::deref() {
	if (eUniniting == m_eState)
		return;

	if (eUninited == m_eState) {
		KDASSERT(0);
		return;
	}
	
	m_ref--;
	KDASSERT(-1 <= m_ref); // �����ڵ㱻���ٵ�ʱ�򣬴�ʱ����Ϊ-1

	// ���û���ڵ㣬�����ü��������ӽڵ�����˵���ⲿ�������ö�û���ˣ����Խ���ɾ����
	// Ϊ���ʱ��ʹû�и��ڵ�Ҳ����ɾ������Ϊ�п����Ǹ��ڵ���ɾ�������ʱ����õ���
	if (m_ref > 0 || m_pParent)
		return;

	// �������Լ����ӽڵ��ʱ�򣬻�����һ�����ķ�Ӧ���������ټ�������ʱ����ͷ�
	// �ڵ��ñ�������ʱ���п����Ǵ�fire event���������ʱ���ɾ�����Ǵ˽ڵ���ߴ˽ڵ�ĸ��ڵ㣬�������������
	m_eState = eUniniting;

	ClearAllBindAndAdditionalData();

	for(size_t it = 0; it < m_children.size(); ++it) {
		UINode* child = (UINode*)m_children[it];
		child->ClearAllBindAndAdditionalData();
		m_pManager->ReapResources(child); // �������ӽڵ��ʱ���п����ӽڵ����ڱ��ű�ռ�ã���ʱ�������٣�

		// ����ʱ�����Ѷ�Ӧ����Դ�����ͷţ������´������Դ��ʱ����ظ�
		child->SetParent(NULL); // �����������ʱ�򣬿��ܻ�����

		CheckRefIsNull(child); // ����һ��ʱ��Ӧ������Ϊ0��
		if (0 == child->GetRef()) // ����ӽڵ������Ϊ�㣬˵�����ű�ռ�á���ʱȥderef���������ӽڵ㱻���٣��ű��ٲ����ͱ�����
			child->deref(); // ��Ȼ��ӽ�����ʱ��Ҫref�����ͷŵ�ʱ����Ҫ���Ա�֤������
	}
	m_children.clear();

	delete this;
}

class DelayDerefTask : public PageManagerDelayTask {
	WTF_MAKE_FAST_ALLOCATED;
public:
	DelayDerefTask(UINode* n) {
		m_deleteNode = n;
		m_deleteNode->ref();
	}

	virtual void Run(CPageManager* manager, Moment moment) {
		m_deleteNode->DelayDeref();
	}

	virtual void Destroy() {delete this;}

private:
	UINode* m_deleteNode;
};

void UINode::DelayDeref() {
	m_eState = eBeginToUninit;
	// ��ʱ�򣬽ڵ�ᱻ�������������Ϣ�����������á�
	// ����ռ������Ĺ���������DestroyTree�н��У���Ϊͨ����ʱ������Ϣ�ɷ������Էŵ��������
	RecursionClearAllBindAndAdditionalData(this);

	deref();
}

void UINode::RecursionClearAllBindAndAdditionalData(UINode* root) {
	// �����������ڵ�����ʱ�������UINode������������Userdata����ʱUserdata��v�����Ѿ���������
	// ����Ҫ��ǰ��һ��
	root->ClearAllBindAndAdditionalData();

	WTF::Vector<UINode*>* childs = root->GetChilds();
	if (!childs)
		return;

	for (size_t i = 0; i < childs->size(); ++i) {
		UINode* n = childs->at(i);
		RecursionClearAllBindAndAdditionalData(n);
	}
}

BOOL UINode::DestroyTree() {
	m_pManager->PostDelayTask(new DelayDerefTask(this));

	m_eState = eBeginDelayUninit;
	// ɾ�������̣��������Ľڵ�ͱ��ڵ㼰���ӽڵ�Ĺ������ټ�����һ
	m_pManager->ReapNodeTreeResources(this); // Ϊ�˷�ֹ��ĵط��������ñ��ڵ�
	// ���ﲻ������¼�����������Ϊ�������һ�����¼��ص�����á�����˷��ؾͻ�ĵ�
	if (m_pParent)
		m_pParent->RemoveChild(this);

	//KDASSERT(m_ref == 1); // ���ñ�������ռ��1�����ü���

	if (0)
		sqstd_printcallstack(m_pManager->GetScriptMgr()->GetVM());	

	return TRUE;
}

void UINode::ClearAllBindAndAdditionalData() {
	SetUserdata(NULL);
	SetWidget(NULL);
	ClearEventListener();
}

void UINode::SetFocus() {
	if(m_pManager)
		m_pManager->SetFocus(this);
}

void UINode::SetToolTip(LPCTSTR pstrText) {
	m_sToolTip = pstrText;
}

CStdString UINode::GetToolTip() const {
	return m_sToolTip;
}

void UINode::Init() {
}

CPageManager* UINode::GetManager() const {
	return m_pManager;
}

void UINode::SetManager(CPageManager* pManager) {
	m_pManager = pManager;
}

void UINode::SetParent(UINode* pParent) {	
	m_pParent = pParent;
}

UINode* UINode::GetParent() const {
	if (m_pParent)
		m_pParent->ref();
	return m_pParent;
}

UINode* UINode::GetParentTemp() const {
	return m_pParent;
}

CStdString UINode::GetName() const {
	return m_sName;
}

void UINode::SetName(LPCTSTR pstrName) {
	m_sName = pstrName;
}

LPVOID UINode::GetInterface(LPCTSTR pstrName) {
	if( _tcscmp(pstrName, _SC("Control")) == 0 ) return this;
	return NULL;
}

bool UINode::IsEqualToTheClassName(LPCTSTR className) const {
	return 0 == _tcsicmp(GetClass(), className);
}

PassRefPtr<UINode> UINode::FindControl(FINDCONTROLPROC Proc, LPVOID pData, UINT uFlags, LPVOID pProcData) {
	if (!RenderSupport::CanHittest(this))
		return NULL;

	if( (uFlags & UIFIND_HITTEST) != 0 && !RenderSupport::IsPointInNodeBoundingWithClippingArea(this, *(FloatPoint*)(pData), NULL))
		return NULL;

	// ������������ͣ���__FindControlByPointAndDispatchMsg��Ҫ�ɷ���Ϣ�������ܽ��ܽ��㣬Ҳ�����ò�ѯ�����������ж�
	if (Proc)
		Proc(this, pData, pProcData);

	for(int it = m_children.size() - 1; it >= 0; --it) {
		UINode* n = static_cast<UINode*>(m_children[it]);
		RefPtr<UINode> ret = n->FindControl(Proc, pData, uFlags, pProcData);
		if (ret.get())
			return ret;
	}

	return IsContainer() ? NULL : this; // �����G�����ߵ���һ��˵����Ȼ��λ���Լ��ķ�Χ�ڣ����ӿؼ�û��һ���ǿɼ���
}

void UINode::Invalidate() {
	InvalidateRelativeRect(BoundingRectInLocalCoordinates(), false); // BoundingRectAbsolute
}

void UINode::InvalidateRelativeRect(const IntRect& rc, bool bForce) {
	// һ���֪ͨ���ڵ㣬�����ɸ��ڵ�֪ͨmanage����������Ŀ���ǣ�����ڵ���mask����Ա�����
	OnChildInvalidateRelativeRect(rc, bForce);
}

void UINode::InvalidateAbsoluteRect(const IntRect& rc, bool bForce) {
	OnChildInvalidateAbsoluteRect(rc, bForce);
}

void UINode::UpdateLayout() {
	if (m_pManager)
		m_pManager->UpdateLayout();
}

int UINode::DispatchEventToListeners(TEventUI& event, BOOL bCapturePhase) {
	if (!m_listenerMgr)
		return 0;

	return m_listenerMgr->DispatchEventToListeners(this, event, bCapturePhase);
}

bool UINode::AddEventListener(UIEventType eventType, PassRefPtr<EventListener> eventListener) {
	if (!m_listenerMgr)
		m_listenerMgr = new ListenersMgr();
	return m_listenerMgr->AddEventListener(this, eventType, eventListener);
}

void UINode::RemoveEventListener(UIEventType eventType, EventListener* eventListener) {
	if (!m_listenerMgr)
		return;
	m_listenerMgr->RemoveEventListener(this, m_pManager, eventType, eventListener);
}

void UINode::ClearEventListener() {
	if (!m_listenerMgr)
		return;
	m_listenerMgr->ClearEventListener(m_pManager);
	UHDeletePtr(&m_listenerMgr);
}

bool UINode::HaveListenersByEventType(UIEventType Type) {
	if (!m_listenerMgr)
		return false;
	return m_listenerMgr->HaveListenersByEventType(Type);
}

int UINode::Event(TEventUI& event, BOOL bCapturePhase) {
	RefPtr<UINode> protect = this; // ��dispath�п��ܰѱ��ڵ�����

	DispatchEventToListeners(event, bCapturePhase);
	// �ɷ������ڵ�����飬ͳһ�����ⲿ���У�����Ϊ�˷�ֹ�ɷ������нű��Ѹ��ڵ�������

	return 0;
}

IntRect UINode::BoundingRectInLocalCoordinates() {
	if (!m_bBoundingDirty)
		return m_rcItem;
	m_bBoundingDirty = false;

	if (IsContainer() && eNRSOverflowHidden != m_nodeStyle->GetintAttrById(eNRStyleOverflow))
		m_rcItem = IntRect();
	else
		m_rcItem = IntRect(m_nodeStyle->X(), m_nodeStyle->Y(), m_nodeStyle->Width(), m_nodeStyle->Height());

	for(size_t it = 0; it < m_children.size(); ++it) {
		UINode* n = static_cast<UINode*>(m_children[it]);
		// �ӽڵ������Ӧ��ȡ�������꣬�ټ��ϱ任.���ֱ����BoundingRectAbsolute����������ӱ任��������
		IntRect childBounding = n->BoundingRectInLocalCoordinates(); 
		SkRect skChildBounding = SkRect::MakeFromIRect(childBounding);
		NodeStyle* style = (NodeStyle*)n->GetAttrs();
		const SkMatrix* transform = style->GetTransform();
		if (!transform->isIdentity())
			transform->mapRect(&skChildBounding);

		m_rcItem.unite(IntRect(skChildBounding));
	}

	RenderSupport::IntersectClipPathWithResources(this, m_rcItem);

	return m_rcItem;
}

IntRect UINode::BoundingRectAbsolute() {
	SkRect rc = SkRect::MakeFromIRect(BoundingRectInLocalCoordinates());

	bool bHaveFloatTransform = false;
	const SkMatrix* transform = NULL;
	UINode* parent = this;
	while (parent) {
		const SkMatrix* transform = ((NodeStyle*)parent->GetAttrs())->GetTransform();
		if (transform->getType() > SkMatrix::kTranslate_Mask)
			bHaveFloatTransform = true;

		if (!transform->isIdentity())
			transform->mapRect(&rc);

		parent = parent->GetParentTemp();
	}
	
	SkIRect dst;
	if (bHaveFloatTransform) {
		rc.round(&dst);
		return IntRect(dst);
	}
	
	return IntRect(rc);
}

void UINode::SetBoundingDirty() {
	SetBoundingDirtyImp();

	UINode* parent = GetParentTemp();
	while (parent) {
		parent->SetBoundingDirtyImp();
		parent = parent->GetParentTemp();
	}		
}

void UINode::SetBoundingDirtyImp() {
	m_bBoundingDirty = true;
}

void UINode::SetRepaintDirty() {
	m_bRepaintDirty = true;
}

void UINode::Layout(IntRect rc) {
	Invalidate();
}

void UINode::SetNeedLayout() {
	SetBoundingDirty();
	SetRepaintDirty();
	Invalidate();
}

void UINode::Paint(GraphicsContext* g, const IntRect &rcInvalid) {
	RenderCommBegin();

	for(size_t it = 0; it < m_children.size(); ++it)		
		m_children[it]->Paint(g, repaintRc);

	RenderCommEnd();
}

NodeAttrDef* UINode::GetCanRefreshAttrs(NodeAttrDef style) const {
	static NodeAttrDef defaultAttrs[] = {eNRStyleEnd};
	return defaultAttrs;
}

bool UINode::IsCanRefreshAttr(LPCTSTR pstrName) {
	NodeAttrDef style = NodeStyle::FindStyleIdByName(pstrName);
	if (eNRStyleEnd == style)
		return false;

	if (eNRStyleStyle == style)
		return true;

	NodeAttrDef* validAttrs = GetCanRefreshAttrs(style);
	for (int i = 0; eNRStyleEnd != validAttrs[i]; ++i) {
		if (style == validAttrs[i])
			return true;
	}
	
	return false;
}

void UINode::OnDrawResChange(ChildState childState) {
	if (m_pParent)
		m_pParent->OnDrawResChange(childState);
}

void UINode::OnChildSetAttr(LPCTSTR pstrName, LPCTSTR pstrValue) {
	if (m_pParent)
		m_pParent->OnChildSetAttr(pstrName, pstrValue);
}

void UINode::OnChildSetAttrBegin() {
	if (m_pParent)
		m_pParent->OnChildSetAttrBegin();
}

void UINode::OnChildSetAttrEnd() {
	if (m_pParent)
		m_pParent->OnChildSetAttrEnd();
}

// Ϊ���Ż��ٶȣ����Լ���begin��end����
void UINode::SetAttrBegin() {
	// �������Ҫ�������ꡣ��Ϊset attr���ܻ�ı���к�����任
	// ��֮���ˢ�£��������������ټ�����任������϶�����
	m_saveRepaintRectAbsolute = BoundingRectAbsolute();
	m_bRepaintDirty = false;
	m_bVisibleWhenBeginSetAttr = RenderSupport::CanRender(this);

	if (m_pParent)
		m_pParent->OnChildSetAttrBegin();
}

void UINode::SetAttrEnd() {
	m_bStartDelayLayout = false;

	if (m_bRepaintDirty) {// ������Ե���Ԫ�����ݸı䣬����Ҫˢ�½���
		InvalidateAbsoluteRect(m_saveRepaintRectAbsolute, m_bVisibleWhenBeginSetAttr); // ��ˢһ���ϱ߿�
		if (m_bBoundingDirty) // ����߿�ı䣬ˢ���±߿�
			Invalidate();
	}
	m_saveRepaintRectAbsolute.clear();
	
	if (m_pParent)
		m_pParent->OnChildSetAttrEnd();
}

bool UINode::SetAttr(LPCTSTR pstrName, LPCTSTR pstrValue) {	
	if (!m_nodeStyle->SetAttr(pstrName, eNRStyleTypeString, 0, 0, pstrValue))
		return false;
	
	if (NodeStyle::IsStyleChangeBounding(pstrName)) // �����������߿�ı䣬����Ҫˢ�±��ڵ�����и��ڵ����ı߿�
		SetBoundingDirty();

	if (IsCanRefreshAttr(pstrName)) // �����֧�ֵ����Ծ���Ҫˢ�½���
		m_bRepaintDirty = true;

	if (m_pParent)
		m_pParent->OnChildSetAttr(pstrName, pstrValue);

	return true;
}

void UINode::OnChildInvalidateAbsoluteRect(const IntRect& rc, bool bForce) {
	if (!RenderSupport::CanRender(this) && !bForce)
		return;

	UINode* parent = GetParentTemp();
	if (parent)
		parent->OnChildInvalidateAbsoluteRect(rc, bForce);
}

void UINode::OnChildInvalidateRelativeRect(const IntRect& rc, bool bForce) {
	if (!RenderSupport::CanRender(this) && !bForce)
		return;

 	IntRect clipRect(rc); // Ҫ���Ǳ�����м������
 	RenderSupport::IntersectClipPathWithResources(this, clipRect);

	SkRect skRect = (SkRect)(clipRect); // �ٿ�������任
	if (!m_nodeStyle->GetTransform()->isIdentity())
 		m_nodeStyle->GetTransform()->mapRect(&skRect);
	IntRect rcInvalid(skRect);

	UINode* parent = GetParentTemp();
	if (parent)
		parent->OnChildInvalidateRelativeRect(rcInvalid, bForce);
}

CStdString UINode::GetAttr(LPCTSTR pstrName) const {
	return m_nodeStyle->GetAttr(pstrName);
}

bool UINode::AppendChild(UINode* pControl) {
	if (0)
		sqstd_printcallstack(m_pManager->GetScriptMgr()->GetVM());	

	if (!IsContainer())
		return false;

	UINode* childParent = pControl->GetParentTemp();
	if (childParent)
		return false;
		
	for(size_t it = 0; it < m_children.size(); ++it) {
		if(static_cast<UINode*>(m_children[it]) == pControl) {
			KDASSERT(FALSE);
			return false;
		}
	}

	pControl->SetParent(this);
	m_children.append(pControl);

	SetBoundingDirty();
	Invalidate();
	return true;
}

bool UINode::RemoveChild(UINode* pControl) {
	KDASSERT(IsContainer());
	for(size_t it = 0; it < m_children.size(); ++it) {
		if(static_cast<UINode*>(m_children[it]) == pControl) {
			pControl->SetBoundingDirty();
			pControl->Invalidate();
			pControl->SetParent(NULL);
			m_children.remove(it);
			return true;
		}
	}

	return false;
}

UINode* UINode::Prev() {
	UINode* pPrev = NULL;
	UINode* pParent = GetParentTemp();
	if (!pParent)
		return NULL;

	WTF::Vector<UINode*>* childs = pParent->GetChilds();
	for(size_t it = 0; it < childs->size(); ++it) {
		if (this == childs->at(it)) {
			if (0 != it) {
				pPrev = (UINode*)childs->at(it - 1);
				pPrev->ref();
				return pPrev;
			} else
				return NULL;
		}
	}

	KDASSERT(FALSE);
	return NULL;
}

UINode* UINode::Sibling() {
	UINode* pSibling = NULL;
	UINode* pParent = GetParentTemp();
	if (!pParent)
		return NULL;

	WTF::Vector<UINode*>* childs = pParent->GetChilds();
	for(size_t it = 0; it < childs->size(); ++it) {
		if (this == childs->at(it)) {
			if (it != childs->size() - 1) {
				pSibling = (UINode*)childs->at(it + 1);
				pSibling->ref();
				return pSibling;
			} else
				return NULL;
		}
	}

	KDASSERT(FALSE);
	return NULL;
}

UINode* UINode::FirstChild() {
	UINode* firstChild = NULL;

	WTF::Vector<UINode*>* childs = GetChilds();
	if (0 >= childs->size())
		return NULL;

	firstChild = (UINode*)childs->at(0);
	firstChild->ref();
	return firstChild;
}

UINode* UINode::LastChild() {
	UINode* lastChild = NULL;

	WTF::Vector<UINode*>* childs = GetChilds();
	if (0 >= childs->size())
		return NULL;

	lastChild = (UINode*)childs->at(childs->size() - 1);
	lastChild->ref();
	return lastChild;
}

UINode* UINode::FindByClass(const SQChar* value) const {
	CStdString findVal = value;
	if (findVal.IsEmpty())
		return 0;

	findVal.MakeLower();

	bool bFindClass = false;
	if (_SC('.') == findVal[0]) {
		bFindClass = true;
		findVal = findVal.Right(findVal.GetLength() - 1);
	}
	if (findVal.IsEmpty())
		return 0;

	for(size_t i = 0; i < m_children.size(); ++i) {
		UINode* n = (UINode*)m_children[i];
		bool bFind = false;
		if (bFindClass) {
			CStdString classAttr = n->GetAttrs()->GetAttr(_SC("class"));
			classAttr.MakeLower();
			if (classAttr.IsEmpty() || findVal != classAttr) 
				continue;
			bFind = true;
		} else
			bFind = findVal == n->GetClass();

		if (bFind) {
			n->ref();
			return n;
		}
	}
	return 0;
}

void UINode::SetWidget(KqNAdditionalData* widget) { 
	if (m_kqWidget) {
		m_kqWidget->Uninit(m_pManager->IsScriptDestroying());
		delete m_kqWidget;
	}
	m_kqWidget = widget;
}

KqNAdditionalData* UINode::Widget() { 
	return m_kqWidget;
}

void UINode::SetUserdata(KqNAdditionalData* userdata) { 
	if (m_kqUserdata) {
		m_kqUserdata->Uninit(m_pManager->IsScriptDestroying());
		delete m_kqUserdata;
	}
	m_kqUserdata = userdata;
}

KqNAdditionalData* UINode::Userdata() { 
	return m_kqUserdata;
}

//////////////////////////////////////////////////////////////////////////

// ������һ���������󣬲����ж��Ƿ���Ҫֹͣ���ж�������
NodeAnim* UINode::Anim (bool stopAll) {
	ref(); // ��ն��������ʱ���п��ܻᵼ�����ü���Ϊ����������������һ
	if (stopAll)
		ForceStopAllAnimAndDestroy();

	deref();

	NodeAnim* animObj = new NodeAnim (this, m_animObjs.size()); // ����������һ

	GetManager()->AppendAnimNode(this); // ����Ҳ�������һ
	m_animObjs.append(animObj);

	return animObj;
}

void UINode::ForceStopAllAnimAndDestroy() {
	int length = m_animObjs.size();

	ref(); // �����animObj->stop(true);��䣬��������animObj�Ǳ����󣬵��¶������٣����������ü�1

	int i = 0;
	NodeAnim* animObj = 0;

	Vector<NodeAnim*> animObjs = m_animObjs;
	for (i = 0; i < length; ++i) {
		animObj = animObjs[i];
		if (0 != animObj) {
			// ʹ����releaseKQueryWhenAnimFinish������ÿֹͣһ�������Լ������ü������2������
			// �ȼ�һ��stop���2���൱��ֻ��1.���Լ��ͷ�ʣ�µ�
			animObj->ReleaseKQueryWhenAnimFinish(1);
			animObj->Stop(true); // �������ص�clearAnimWhenNoticed
		}
	}

	KDASSERT(0 == m_animObjs.size());
	deref();
}

void UINode::ClearAnimWhenNoticed (int idx) {
	if ((int)m_animObjs.size() <= idx) {
		KDASSERT(FALSE);
		return;
	}

	m_animObjs[idx] = 0;

	bool find = false;
	for (int i = 0; i < (int)m_animObjs.size(); ++i) {
		if (0 != m_animObjs[i]) 
		{ find = true; break; }
	}

	if (false == find) { // ������ж��������������������
		GetManager()->RemoveAnimNode(this);
		m_animObjs.clear();
	}
}

//////////////////////////////////////////////////////////////////////////
CStdString UINode::Dump() {
	return DoDump(0);
}

CStdString UINode::DoDump(int nDeep) {
	int childrenSize = m_children.size();

	CStdString output;
	CStdString attrStr;

	CStdString deepBlankSpace;
	for (int i = 0; i < nDeep*3; ++i)
		deepBlankSpace += L' ';

	attrStr = GetAttrs()->DumpAllAttrs();

	CStdString temp;
	temp.Format(_SC("[\"%s\", {%s},"), GetClass(), attrStr.GetString()); // ["g", {"id":"IamId"},
	temp = deepBlankSpace + temp;
	output = temp;

	if (0 != childrenSize)
		output += _SC("\n");

	OutputDebugString(output.GetString());

	if (0 == _tcsicmp(GetClass(), _SC("text")))	{
		CStdString* text;
		if (0 == Dispatch(_SC("GetTextOfTextNode"), 0, NULL, NULL, &text) && text) {
			temp = *text;
			temp.Replace(_SC('\\'), _SC('/'));
			temp.Replace(_SC("\""), _SC("\\\""));
// 			temp = _SC("\"") + temp;
// 			temp += _SC("\"");
// 			temp = _SC("\n") + deepBlankSpace + _SC("   ") + temp;
			/*temp = */temp.Format(_SC("\"%s\"\n%s   %s"), temp.GetString(), deepBlankSpace.GetString(), temp.GetString());
			
			OutputDebugString(temp.GetString());
			output += temp;
		}
	}

	for (int i = 0; i < childrenSize; ++i) {
		UINode* n = (UINode*)m_children[i];
		output += n->DoDump(nDeep + 1);
	}

	if (0 != childrenSize)
		/*temp = */temp.Format(_SC("%s]"), deepBlankSpace.GetString());
	else
		temp = _SC("],");

	CStdString thisStr;
	thisStr.Format(_SC("//%x %d\n"), this, this);
	temp += thisStr;
	OutputDebugString(temp.GetString());
	output += temp;

	return output;
}