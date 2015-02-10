
#include <UIlib.h>
#include "ScheduledAction.h"
#include "DOMTimer.h"

ScheduledAction::ScheduledAction(HSQUIRRELVM v, HSQOBJECT* function, int timeout, bool singleShot, ThreadTimers* threadTimers, DOMTimer* pDOMTimer)
	: m_function(*function)
	, m_timer(this, &ScheduledAction::Fire, threadTimers)
	, refCount(0)
	, m_DOMTimer(pDOMTimer)
	, m_bSingleShot(singleShot)
	, m_timerId(0)
	, m_v(v) {
	sq_addref(v, &m_function);
	Start(timeout, singleShot);
}

ScheduledAction::~ScheduledAction() {
	sq_release(m_v, &m_function);
}

void ScheduledAction::deref() {
	--refCount;
	if (0 == refCount)
		delete this;
}

void ScheduledAction::Fire(Timer<ScheduledAction>*) {
	HSQUIRRELVM v = m_v;
	HSQOBJECT functionOfCloser = m_function;
	sq_addref(v, &functionOfCloser); // ֮����Ҫ���ü�һ����Ϊ�˷�ֹ�ڽű��аѱ�����ɾ����

	SQInteger top = sq_gettop(v); // ����ԭʼ��ջ��С
	sq_pushobject(v, functionOfCloser);
	sq_pushroottable(v); // ����ĺ�������������һ��thisָ��

	ref();
	// ע�⣬�ڽű����п��ܵ���clearInterval�����±�this��delete��������call֮��������ʳ�Ա������
	sq_call(v, 1, SQFalse, SQTrue);
	
	int listenerRefConut = sq_getrefcount(v, &functionOfCloser);
	sq_release(v, &functionOfCloser);

	sq_settop(v, top); // ��ԭ��ջ

	if (m_bSingleShot) // �����ִ��һ�Σ���ɵ��Լ�
		m_DOMTimer->RemoveById(m_timerId);
	deref();
}

void ScheduledAction::Start(int timeout, bool singleShot) {
	double intervalMilliseconds = timeout/1000.0;
	if (singleShot)
		m_timer.startOneShot(intervalMilliseconds);
	else
		m_timer.startRepeating(intervalMilliseconds);
}