class EffectsResNodeMgr {
public:
	WTF::HashMap<UINode*, WTF::Vector<UINode*>* > m_resNodes; // �������Դ�ڵ�
	WTF::HashMap<UINode*, WTF::Vector<UINode*>* > m_needResNodes; // ������Դ�Ľڵ㣬��path��rect
	WTF::HashMap<UINode*, CStdString> m_resOwnersOfCanotFindRes; // ӵ�����ȴ�������Դ�ٴ���ʱ����Ҫ�ȼ�����
	CPageManager* m_pManager;

	EffectsResNodeMgr(CPageManager* pManager) 
		: m_pManager(pManager) {

	}

	void Init() {
	}

	void ReapEffectsResources(UINode* pControl) {
		m_pManager->RemoveIdMapByNode(pControl);
		RemoveEffectsResNode(pControl);
		RemoveEffectsResOwnerNode(pControl);
	}

	void ReapNodeTreeEffectsResources(UINode* pControl) {
		pControl->SetStateBeginUninit();
		pControl->ForceStopAllAnimAndDestroy(); // DEBUG TODOҪ�����Ƿ�������ö��������ص�

		ReapEffectsResources(pControl);
		WTF::Vector<UINode*>* childs = pControl->GetChilds();
		if (!childs)
			return;

		for(size_t i = 0; i < childs->size(); ++i)
			ReapNodeTreeEffectsResources((UINode*)childs->at(i));
	}

	void AddEffectsResNode(UINode* resNode) {
		WTF::HashMap<UINode*, WTF::Vector<UINode*>* >::iterator it = m_resNodes.find(resNode);
		if (it != m_resNodes.end())
			return;

		m_resNodes.set(resNode, new WTF::Vector<UINode*>());
	}

	void RemoveEffectsResNode(UINode* resNode) {
		WTF::Vector<UINode*> ownersCopy;
		WTF::Vector<UINode*>* owners = GetEffectsResOwnerNodes(resNode); // ɾ����֪ͨӵ������ˢ��

		if (owners) // �����erase������owners��������Ҫ��������
			for (size_t i = 0; i < owners->size(); ++i)
				ownersCopy.append(owners->at(i));

		WTF::HashMap<UINode*, WTF::Vector<UINode*>*>::iterator it = m_resNodes.find(resNode);
		if (it == m_resNodes.end())
			return;
		delete it->second;
		m_resNodes.remove(it);

		for (size_t i = 0; i < ownersCopy.size(); ++i) {
			UINode* owner = ownersCopy[i];
			owner->SetBoundingDirty();
			owner->Invalidate();
		}
	}

	WTF::Vector<UINode*>* GetEffectsResOwnerNodes(UINode* resNode) {
		WTF::HashMap<UINode*, WTF::Vector<UINode*>*>::iterator iter = m_resNodes.find(resNode);
		if (iter != m_resNodes.end())
			return iter->second;

		WTF::HashMap<UINode*, CStdString>::iterator it = m_resOwnersOfCanotFindRes.begin();
		for (; it != m_resOwnersOfCanotFindRes.end(); ++it) {
			if (resNode->GetAttrs()->Id() != it->second)
				continue;

			WTF::Vector<UINode*>* nodes = new WTF::Vector<UINode*>();
			m_resNodes.set(resNode, nodes);
			nodes->append(it->first);
			m_resOwnersOfCanotFindRes.remove(it);

			return nodes;
		}
		return NULL;
	}

	static bool DoRemoveEffectsResOwnerNode(UINode* ownerNode, WTF::HashMap<UINode*, WTF::Vector<UINode*>*>& resNodes) {
		WTF::HashMap<UINode*, WTF::Vector<UINode*>*>::iterator it = resNodes.begin();
		for (; it != resNodes.end(); ++it) {
			WTF::Vector<UINode*>* ownerNodes = it->second;
			for (size_t i = 0; i < ownerNodes->size(); ++i) {
				UINode* n = ownerNodes->at(i);
				if (n != ownerNode)
					continue;
				ownerNodes->erase(i);
				return true;
			}
		}

		return false;
	}

	void RemoveEffectsResOwnerNode(UINode* ownerNode) {
		bool bLoop = false;
		do {
			bLoop = DoRemoveEffectsResOwnerNode(ownerNode, m_resNodes);
		} while (bLoop);

		do {
			bLoop = false;
			WTF::HashMap<UINode*, CStdString>::iterator it = m_resOwnersOfCanotFindRes.find(ownerNode);
			if (it == m_resOwnersOfCanotFindRes.end())
				break;

			bLoop = true;
			m_resOwnersOfCanotFindRes.remove(it);
		} while (bLoop);
	}

	UINode* GetEffectsResNodeByUrl(LPCTSTR pSrc, UINode* owner) {
		UINode* resNode = m_pManager->GetNodeByID(pSrc);
		if (!resNode) { // ���һʱ�Ҳ��������´���Դ�ڵ㹹����Ϻ�϶����ҵ�
			if (m_resOwnersOfCanotFindRes.end() == m_resOwnersOfCanotFindRes.find(owner))
				m_resOwnersOfCanotFindRes.set(owner, CStdString(pSrc));
			return NULL;
		}

		// ������Դ�ڵ���û���������ȼ�¼��ӵ���߽ڵ�
		WTF::HashMap<UINode*, WTF::Vector<UINode*>*>::iterator it = m_resNodes.find(resNode);
		if (it != m_resNodes.end()) { // ����ҵ��ˣ��������Դ - ӵ����
			bool bFind = false;
			for (size_t i = 0; i < it->second->size(); ++i) {
				if (owner != it->second->at(i))
					continue;
				bFind = true;
				break;
			}
			if (!bFind)
				it->second->push_back(owner);
		} else // ���û�ҵ������¼�����Դ�ڵ�
			AddEffectsResNode(resNode);

		return resNode;
	}

	void UninitResNodes() {
		for (WTF::HashMap<UINode*, WTF::Vector<UINode*>* >::iterator it = m_resNodes.begin(); it != m_resNodes.end(); ++it) {
			if (it->second)
				delete it->second;
		}
		m_resNodes.clear();
	}
};
