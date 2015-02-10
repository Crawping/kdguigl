class IdMgr {
	WTF::HashMap<DWORD, UINode*> m_idMap;

public:
	void Init() {
	}

	void AddToIdMap(const SQChar* id, UINode* n) {
		if (!id)
			return;

		CStdString Id(id);
		Id.MakeLower();
		if (0 >= Id.GetLength())
			return;

		UINT idHash = UHGetNameHash(Id.GetBuffer());
		WTF::HashMap<DWORD, UINode*>::iterator it = m_idMap.find(idHash);
		if (it != m_idMap.end() && it->second != n) { // ���id�����ˣ��Ͳ����ã�������kdgui2
			OutputDebugString(_SC("���½ڵ�id���ظ���"));
			OutputDebugString(id);
			KDASSERT(FALSE);

			// kdgui2���������߼��ǣ������ڵ���������GetNodeById��ȡ���ǿ����Ľڵ�
			return;
		}
		m_idMap.set(idHash, n);
	}

	void RemoveIdMapByNode(UINode* n) {
		bool bLoop = false;
		do {
			bLoop = false;
			WTF::HashMap<DWORD, UINode*>::iterator it;
			for (it = m_idMap.begin(); it != m_idMap.end(); ++it) {
				if (n == it->second) {
					bLoop = true;
					m_idMap.remove(it);
					break;
				}
			}
		} while (bLoop);
	}

	UINode* GetNodeByID(const CStdString& id) {
		if (id.IsEmpty())
			return NULL;

		CStdString lowID = id;
		lowID.MakeLower();
		UINT idHash = UHGetNameHash(lowID.GetString());
		WTF::HashMap<DWORD, UINode*>::iterator it = m_idMap.find(idHash);
		if (it == m_idMap.end())
			return NULL;

		it->second->ref();
		return it->second;
	}
};