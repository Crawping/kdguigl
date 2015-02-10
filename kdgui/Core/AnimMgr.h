
class AnimMgr {
	Vector<UINode*> m_animatingNodeQueue; // ���ڽ��ж����Ľڵ㣬�����ڹر�ʱ��ͳһɾ������

public:
	void Init() {
	}

	void AppendAnimNode(UINode* node) {
		size_t size = m_animatingNodeQueue.size();
		for (size_t i = 0; i < size; ++i) {
			if (node == m_animatingNodeQueue[i])
				return;
		}

		node->ref();
		m_animatingNodeQueue.append(node);
	}

	void RemoveAnimNode(UINode* node) {
		size_t size = m_animatingNodeQueue.size();
		for (size_t i = 0; i < size; ++i) {
			if (node == m_animatingNodeQueue[i]) {
				m_animatingNodeQueue.remove(i, 1);
				node->deref();
				return;
			}
		}
	}

	void ForceStopAllAnim() {
		// ��Ϊ��ǿ��ֹͣ��ʱ������removeAnimNode�������У�������Ҫ����һ��
		while (0 != m_animatingNodeQueue.size()) {
			Vector<UINode*> animatingNodeQueueDummy = m_animatingNodeQueue;
			size_t size = animatingNodeQueueDummy.size();
			for (size_t i = 0; i < size; ++i) // ��ֹͣ��ʱ���п����û������������Ӷ��������������Ҫһ��ѭ��
				animatingNodeQueueDummy[i]->ForceStopAllAnimAndDestroy();
		}

		KDASSERT(0 == m_animatingNodeQueue.size());
	}
};