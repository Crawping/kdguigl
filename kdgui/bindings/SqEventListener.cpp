
#include <UIlib.h>
#include <wtf/UtilHelp.h>
#include <wtf/RefCountedLeakCounter.h>

#include "SqEventListener.h"
#include "script/scripthelp/SquirrelBindingsUtils.h"
#include "dom/UINode.h"

#ifndef NDEBUG
static RefCountedLeakCounter SqEventListenerCounter(_SC("SqEventListener"));
#endif

SqEventListener::SqEventListener(HSQUIRRELVM v, HSQOBJECT listenerObj, HSQOBJECT listenerData, bool bUseCapture) 
	: EventListener(SqEventListenerType, bUseCapture) {
	m_listenerObj = listenerObj;
	m_listenerData = listenerData;
	m_v = v;

#ifndef NDEBUG
	SqEventListenerCounter.increment();
#endif
}

SqEventListener::~SqEventListener() {
	KDASSERT(OT_CLOSURE == m_listenerObj._type || OT_NULL == m_listenerObj._type);
	if (OT_CLOSURE == m_listenerObj._type && !m_bIsScriptDestroying) {
		KDASSERT (0 != m_v);

		sq_release(m_v, &m_listenerObj);
		m_listenerObj._type = OT_NULL;

		sq_release(m_v, &m_listenerData);      
	}

	m_fireCount = -1;
	m_refCount = -1;

#ifndef NDEBUG
	SqEventListenerCounter.decrement();
#endif
}

bool SqEventListener::Equal(const EventListener& other) const {
	if (m_type != other.GetType())
		return false;

	const SqEventListener* pOther = (const SqEventListener*)&other;	
	if (m_listenerObj._type == pOther->m_listenerObj._type &&
		m_listenerObj._unVal.pClosure == pOther->m_listenerObj._unVal.pClosure)
		return true;
	
	return false;
}

static SQInteger _eventToSqReleaseHook(SQUserPointer p, SQInteger size)
{
	SqPushEventStruct* data = (SqPushEventStruct*)p;
	*data->ref = 0;
	return 1;
}

static SqPushEventStruct* PushEventToSq(HSQUIRRELVM v, TEventUI* event, SQRELEASEHOOK hook)
{
	SqPushEventStruct* data = new SqPushEventStruct();
	data->event = event;
	KDASSERT(SbuCreateNativeClassInstance(v, _SC("KqEvt"), data, hook));

	return data;
}

class ProtectFireEvent { // Ϊ�˷�ֹ���ɷ��¼���ʱ��ɾ�����ڵ�
public:
	ProtectFireEvent(UINode* pTarget, UINode* pCurrentTarget) {
		CPageManager* manage = NULL;
		m_pTarget = pTarget;
		m_pCurrentTarget = pCurrentTarget;
		if (pTarget) {
			manage = pTarget->GetManager();
			//manage->PushFireEventNode(pTarget);
		} 
		
		if (pCurrentTarget) {
			manage = pCurrentTarget->GetManager();
			//manage->PushFireEventNode(pCurrentTarget);
		}

		if (!manage)
			return;
	}

	~ProtectFireEvent() {
		CPageManager* manage = NULL;
		if (m_pTarget) {
			manage = m_pTarget->GetManager();
			//manage->EraseFireEventNode(m_pTarget);
		} 
		
		if (m_pCurrentTarget) {
			manage = m_pCurrentTarget->GetManager();
			//manage->EraseFireEventNode(m_pCurrentTarget);
		}
	}

private:
	UINode* m_pTarget;
	UINode* m_pCurrentTarget;
};

void SqEventListener::FireEvent(TEventUI* evt) {
	m_fireCount++; // �п��ܻ����뵽clear listener��
	ref(); // ֮����Ҫ���ü�һ����Ϊ�˷�ֹ�ڽű��аѱ�����ɾ����
	ProtectFireEvent protect(evt->pTarget, evt->pCurrentTarget);

	// ǿ��ʹ�ñ�����������������scriptExecutionContextЯ���ģ�������Ϊ���ܷ�����attach����
	HSQUIRRELVM v = m_v;

	sq_addref(v, &m_listenerObj); // ֮����Ҫ���ü�һ����Ϊ�˷�ֹ�ڽű��аѱ�����ɾ����
	sq_addref(v, &m_listenerData);

	SQInteger top = sq_gettop(v); // ����ԭʼ��ջ��С
	sq_pushobject(v, m_listenerObj);
	sq_pushroottable(v); // ����ĺ�������������һ��thisָ��

	int refNum = 1;
	SqPushEventStruct* pushEvtStruct = PushEventToSq(v, evt, _eventToSqReleaseHook);
	pushEvtStruct->ref = &refNum;

	sq_pushobject(v, m_listenerData);

	if(SQ_FAILED(sq_call(v, 3, SQFalse, SQTrue)))
		KDASSERT(FALSE);
	KDASSERT (0 == refNum); // ����ű�û�ͷ��ˣ�˵���������ˣ�����ǿ���ͷ�
						    // ��ʱ�����������Ϊ���˺����ķ�Χ�����뱣֤��evt��Ч��
	delete pushEvtStruct;

	sq_release(v, &m_listenerData);

	int listenerRefConut = sq_getrefcount(v, &m_listenerObj);
	sq_release(v, &m_listenerObj);
	if (1 == listenerRefConut)  // ���Ϊ1��˵���ű��������removeEventListener
		m_listenerObj._type = OT_NULL;

	sq_settop(v, top); // ��ԭ��ջ

	KDASSERT(m_fireCount >= 1);
	m_fireCount--;

	deref(); // ��Ȼ�ܵ��ýű����ǹ������϶�׼�������
}
