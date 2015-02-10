#ifndef DocNode_h
#define DocNode_h

#include "UINode.h"
#include "style/NodeAttrDef.h"

enum NodeAttrDef;

// �ĵ��ڵ�͸��ڵ���������ڣ��ĵ��ڵ��ʾ���ڣ����ڵ��ʾ����Ԫ�صĸ�
class DocNode : public UINode {
	WTF_MAKE_FAST_ALLOCATED;

public:
	DocNode() {}
	virtual ~DocNode() {}

	virtual LPCTSTR GetClass() const {return _SC("document");}
	virtual UINodeEnum GetClassEnum() const {return eNE_Doc;}

	virtual BOOL IsContainer() const {return TRUE;}

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

#endif // DocNode_h