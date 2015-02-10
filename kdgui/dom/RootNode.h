#ifndef RootNode_h
#define RootNode_h

#include "Core/UIManager.h"
#include "style/NodeAttrDef.h"
#include "UINode.h"

enum NodeAttrDef;

class RootNode : public UINode {
	WTF_MAKE_FAST_ALLOCATED;

public:
	RootNode() {}
	virtual ~RootNode() {}

	virtual LPCTSTR GetClass() const {return _SC("root");}
	virtual UINodeEnum GetClassEnum() const {return eNE_Root;}

	virtual BOOL IsContainer() const {return TRUE;}

	// ��������Ի��Ǿ��ԣ������ڵ㶼�Ѿ��Ǿ���������
	virtual void OnChildInvalidateRelativeRect(const IntRect& rc, bool bForce) 
	{ if(m_pManager) m_pManager->Invalidate(rc); }

	virtual void OnChildInvalidateAbsoluteRect(const IntRect& rc, bool bForce)
	{ if(m_pManager) m_pManager->Invalidate(rc); }

	virtual PassRefPtr<UINode> FindControl(FINDCONTROLPROC Proc, LPVOID pData, UINT uFlags, LPVOID pProcData) {
		for(int it = m_children.size() - 1; it >= 0; --it) {
			UINode* n = static_cast<UINode*>(m_children[it]);
			RefPtr<UINode> ret = n->FindControl(Proc, pData, uFlags, pProcData);
			if (ret.get())
				return ret;
		}

		if (!pProcData)
			return NULL;

		const TEventUI* event = (const TEventUI*)pProcData;
		// ��ֹ��껬�����ڣ�������mouseleave�д�������root node
		if (UIEVENT_MOUSEENTER == event->Type) 
			return NULL;
		
		if (HaveListenersByEventType(event->Type))
			return this;
		
		return NULL;
	}

	virtual NodeAttrDef* GetCanRefreshAttrs(NodeAttrDef style) const {
		static NodeAttrDef validAttrs[] = {eNRStyleEnd};
		return validAttrs;
	}
};

#endif // RootNode_h