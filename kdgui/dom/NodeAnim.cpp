#include <UIlib.h>
#include "NodeAnim.h"
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/CurrentTime.h>
#include <wtf/UtilHelp.h>
#include "CppTween.h"
#include "Dom/UINode.h"
#include "Core/UIManager.h"

class KdAnimCore {
public:
	KdAnimCore(float from, float to, int dur,
		NodeAnim* pHandleObj, const CStdString& context, 
		KQueryTween Tween, KQueryEasing Ease, int stepLen, float p0, float p1,
		int parentCurAnimCoreCacheIdx,
		AnimStepCallback stepCallback,
		void* stepCallbackparam1,
		void* stepCallbackparam2
		);
	~KdAnimCore();

	void Start();
	void Stop();

protected:
	float m_t;
	float m_b;
	float m_c;
	float m_d;
	float m_p0;
	float m_p1;
	NodeAnim* m_pObj;
	CStdString m_attr;
	KQueryTween m_Tween;
	KQueryEasing m_Ease;

	double m_currentTime;

	AnimStepCallback m_stepCallback;
	void* m_stepCallbackparam1;
	void* m_stepCallbackparam2;

	TweenFuncType m_pTweenFunc;
	int m_StepLen;
	bool m_active;

	int m_parentCurAnimCoreCacheIdx;

	Timer<KdAnimCore>* m_timer;

	bool _Fire();
	void Fire(Timer<KdAnimCore>*);

	void Stop(bool IsForce);
};

#ifndef NDEBUG
static RefCountedLeakCounter KAnimCounter(_SC("NodeAnim"));
#endif

NodeAnim::NodeAnim (UINode* kqueryElem, int parentCacheIdx ) {
	m_bIsDeleteing = false;
	m_bCanDestroy = true;

	m_node = kqueryElem; 
	m_pManager = m_node->GetManager();
	//m_kqueryElem->ref(); // ���ڵ����ü�����һ
	m_parentCacheIdx = parentCacheIdx; // parentCacheIdx��ʾ��UINode�б�����Ļ���λ�ã����Ϊ-1����ʾ�������Ǹ�α�ڵ�

	m_runingAnimQueueLength = 0; // ���㶯�������ж��ٸ������ÿ�����൱��һ������
	m_hadFinishAnimQueue = 0; // endAnim ʱ���ж����б���Ķ����Ƿ����
	m_curAnimCore.clear(); // ��ǰ�Ķ����б���stop��ʱ������

	//m_endCallback = 0; // ��ǰ�����Ľ����ص���ÿִ����һ��������͸���һ��

	m_releaseKQueryWhenAllAnimFinish = 1; // ���ж���������ʱ���ͷŸ��ڵ�

	m_endCallbackWhenAllAnimFinish = 0;
	m_endCallbackWhenAllAnimFinishParam1 = 0;
	m_endCallbackWhenAllAnimFinishParam2 = 0;

	m_needDestroyMyself = false;

	m_refCounted = 0;
	ref();

#ifndef NDEBUG
	KAnimCounter.increment();
#endif
}

NodeAnim::~NodeAnim() {
#ifndef NDEBUG
	KAnimCounter.decrement();
#endif
}

// $('customc').onclick = function(){
// 	easyAnim('box5').custom({
// 		backgroundColor : '#fb219a',
// 		color : '#000',
// 		borderTopColor : '#2c2c2c',
// 		borderRightColor : '#2c2c2c',
// 		borderBottomColor : '#2c2c2c',
// 		borderLeftColor : '#2c2c2c' 
// 	}, 2000, 'easeBoth' );
// };

NodeAnim* NodeAnim::Queue (AnimEndCallback callback, void* param1, void* param2) {
	if (-1 == m_parentCacheIdx) {
		if (callback)
			callback(param1, param2);
		return this;
	}

	AnimProps animProps;
	animProps.type = 1;
	animProps.endCallback = callback;
	animProps.endparam1 = param1;
	animProps.endparam2 = param2;
	m_animPropsQueue.append(animProps);
	return this;
}

NodeAnim* NodeAnim::Set(const CStdString& prop, float value) {
	if (-1 == m_parentCacheIdx)
		return this;

	AnimProps animProps;
	animProps.type = 2;
	animProps.prop = prop;
	animProps.value = value;
	m_animPropsQueue.append(animProps);
	return this;
}

NodeAnim* NodeAnim::Set(const SQChar* prop, float value) {
	if (-1 == m_parentCacheIdx)
		return this;

	AnimProps animProps;
	animProps.type = 2;
	animProps.prop = prop;
	animProps.value = value;
	m_animPropsQueue.append(animProps);
	return this;
}

NodeAnim* NodeAnim::Delay(int delayTime) {
	if (-1 == m_parentCacheIdx)
		return this;

	AnimProps animProps;
	animProps.stepLen = 10;
	animProps.easing = KQE_EASE_IN;
	animProps.tween = KQT_LINEAR;
	animProps.dur = delayTime;
	animProps.value = delayTime/(animProps.stepLen+0.0f);
	animProps.p0 = -1;
	animProps.p1 = -1;

	animProps.type = 0;
	animProps.prop = "NodeAnim-delay";

	AnimPropPair pair;
	pair.name = "NodeAnim-delay";
	pair.value = delayTime/(animProps.stepLen + 0.0f);
	animProps.props.append(pair);

	m_animPropsQueue.append(animProps);

	return this;
}

NodeAnim* NodeAnim::Custom(Vector<AnimPropPair>& props, int dur, KQueryEasing easing, KQueryTween tween) {
	if (-1 == m_parentCacheIdx)
		return this;

	return CustomWithCallback(props, dur, easing, tween, 10, -1, -1, 0, 0, 0);
}

NodeAnim* NodeAnim::CustomWithCallback(Vector<AnimPropPair>& props, int dur,
	KQueryEasing easing, KQueryTween tween, int stepLen,
	float p0, float p1, AnimStepCallback stepCallback, void* stepCallbackParam1, void* stepCallbackParam2) {
	if (-1 == m_parentCacheIdx) {
		if (stepCallback)
			stepCallback(0, 0, 0, stepCallbackParam1, stepCallbackParam2);
		return this;
	}

	AnimProps animProps;
	animProps.type = 0;
	animProps.CopyProps(props);
	animProps.dur = dur;
	animProps.easing = easing;
	animProps.tween = tween;
	animProps.stepLen = stepLen;
	animProps.p0 = p0;
	animProps.p1 = p1;
	animProps.stepCallback = stepCallback;
	animProps.stepCallbackParam1 = stepCallbackParam1;
	animProps.stepCallbackParam2 = stepCallbackParam2;

	m_animPropsQueue.append(animProps); // һ��m_animPropsQueue����൱��һ��ͬʱ���еĶ��������п�����N�����Ա仯����

	return this;
}

NodeAnim* NodeAnim::Custom(KqPropsPushHelp* props, int dur, KQueryEasing easing, KQueryTween tween)
{
	return CustomWithCallback(props, dur, easing, tween, 10, -1, -1, 0, 0, 0);
}

NodeAnim* NodeAnim::CustomWithCallback(KqPropsPushHelp* props, int dur,
	KQueryEasing easing, KQueryTween tween, 
	int stepLen, float p0, float p1, AnimStepCallback stepCallback, void* param1, void* param2) {
	if (-1 == m_parentCacheIdx) {
		props->deref();
		if (stepCallback)
			stepCallback(0, 0, 0, param1, param2);
		return this;
	}

	Vector<AnimPropPair> copyProps;

	if (0 != props->m_props.size()%2) {
		DebugBreak();
		return this;
	}

	for (int i = 0; i < (int)props->m_props.size(); i+= 2) {
		int valType =  props->m_props[i + 1].type; // ��������ֵ�������Ǹ�����

		if (1 != valType) // float
		{ DebugBreak(); continue; }

		valType =  props->m_props[i].type; // ż���������ƣ��������ַ���
		if (0 != valType) 
		{ DebugBreak(); continue; }

		AnimPropPair pair;
		pair.name = props->m_props[i].stringVal;
		pair.value = props->m_props[i + 1].floatVal;
		copyProps.append(pair);
	}

	props->deref();

	return CustomWithCallback(copyProps, dur, easing, tween, stepLen, p0, p1, stepCallback, param1, param2);
}

//////////////////////////////////////////////////////////////////////////
// m_runingAnimQueueLength ��ʾ���������е�KAnimCore�ĸ�������m_curAnimCore��ͬ��m_curAnimCore�п�����Ϊ0��
// �������Ϊ����������ֻ���m_curAnimCore��Ӧ��λ����0�����������m_curAnimCore����m_runingAnimQueueLength
// �϶���m_curAnimCore�еķ����������ͬ

static bool CheckAnimQueueLength(int nQueueLength, const Vector<KdAnimCore*>& animCores) {
	for (size_t i = 0; i < animCores.size(); ++i) {
		if (0 != animCores[i])
			nQueueLength--;
	}

	return 0 == nQueueLength;
}

static bool CheckAnimCoresIsEmpty(const Vector<KdAnimCore*>& animCores) {
	for (size_t i = 0; i < animCores.size(); ++i) {
		if (0 != animCores[i]) 
			return false;
	}

	return true;
}

UINode* NodeAnim::AttachedNode() { return m_node; }

void NodeAnim::ReleaseKQueryWhenAnimFinish(int releaseCount) {
	m_releaseKQueryWhenAllAnimFinish = releaseCount;
}

void NodeAnim::Run (Vector<AnimPropPair>& props, int dur, KQueryEasing easing, KQueryTween tween,
	int stepLen, float p0, float p1, 
	AnimStepCallback stepCallback, void* stepCallbackParam1, void* stepCallbackParam2) {
	int i = 0;
	KdAnimCore* oKAnimateObj = 0;
	//stopCurAnimCore(); // ���
	ASSERT(CheckAnimCoresIsEmpty(m_curAnimCore));

	for (i = 0; i < (int)props.size(); ++i) {
		CStdString prop;
		if (props[i].name == _SC("NodeAnim-delay")) {
			prop = _SC("0");
		} else {
			prop = m_node->GetAttr(props[i].name);
			if (prop.IsEmpty())
				continue; // ����ᵼ��m_curAnimCore.size() >= m_runingAnimQueueLength
		}

		float fProp = (float)_ttof(prop.GetString());

		oKAnimateObj = new KdAnimCore(fProp, props[i].value, dur,
			this, props[i].name, 
			tween, easing, stepLen, p0, p1, m_curAnimCore.size(), stepCallback, stepCallbackParam1, stepCallbackParam2);
		oKAnimateObj->Start();
		m_curAnimCore.append(oKAnimateObj);

		++m_runingAnimQueueLength;
	}
}

NodeAnim* NodeAnim::ref() {
	m_node->ref(); // ���ڵ����ü�����һ
	m_refCounted++;
	return this;
}

NodeAnim* NodeAnim::deref() {
	m_refCounted--;
	m_node->deref();
	if (m_refCounted > 0)
		return this;

	delete this;
	return 0;
}

void NodeAnim::Start() {
	if (-1 == m_parentCacheIdx) {
		deref();
		return;
	}

	DoStart();

	// ������startû�κζ������У��ɴ��峡���ˡ�һ��������϶ζ���ֹͣ�����µ���start�����¶Σ�����û�ж�����
	if (0 == m_runingAnimQueueLength && 0 == m_animPropsQueue.size()) 
		JuageAllAnimHaveFinishThenClearSource();
}

void NodeAnim::DoStart() {
	ASSERT(0 == m_runingAnimQueueLength);

	m_hadFinishAnimQueue = 0;

	if (0 == m_animPropsQueue.size())
		return;

	AnimProps& animProps = m_animPropsQueue[0]; // shift
	if (0 == animProps.type) {
		//m_runingAnimQueueLength = animProps.props.size();
		Run(animProps.props, animProps.dur, animProps.easing, animProps.tween, animProps.stepLen, animProps.p0, animProps.p1,
			animProps.stepCallback, animProps.stepCallbackParam1, animProps.stepCallbackParam2);
		m_animPropsQueue.remove(0);
	} else if (1 == animProps.type) {
		animProps.endCallback(animProps.endparam1, animProps.endparam2);
		m_animPropsQueue.remove(0);
		Start();
	} else if (2 == animProps.type) {
		CStdString strValue;
		strValue.Format(_SC("%f"), animProps.value);
		m_node->SetAttrBegin();
		m_node->SetAttr(animProps.prop.GetString(), strValue.GetString());
		m_node->SetAttrEnd();
		m_animPropsQueue.remove(0);
		Start();
	}
	ASSERT(CheckAnimQueueLength(m_runingAnimQueueLength, m_curAnimCore));
}

NodeAnim* NodeAnim::Stop(bool clearQueue) {
	ref();
	ASSERT(-1 != m_parentCacheIdx);
	ASSERT(clearQueue); // �ݲ�֧����ͣ���ּ�����ʼ
	m_bCanDestroy = false;

	bool bCheckIsDeleteing = true;
	if (0 == m_curAnimCore.size()) {
		bCheckIsDeleteing = false;
		m_node->ClearAnimWhenNoticed(m_parentCacheIdx);
	} else
		StopCurAnimCore();

	ASSERT(m_refCounted >= 1);
	if (m_refCounted == 1) {
		deref();
		return 0;
	}

	ASSERT(0 == m_curAnimCore.size());
	if (true == clearQueue) { // �����stopCurAnimCore��Ȼ�ᵼ��m_bIsDeleteing == true
#ifndef NDEBUG
		if (bCheckIsDeleteing)
			ASSERT(m_bIsDeleteing);
#endif
		m_animPropsQueue.clear();
		LazyDestroy(0);
		deref();
		return this;
	} 

	Start();

	deref();
	return this;
}

bool NodeAnim::PostWillDestroyAnimCoreQueueNotRepeat(KdAnimCore* animCore) {
	ASSERT(animCore);
	for (int i = 0; i < (int)m_willDestroyAnimCoreQueue.size(); ++i) {
		if (m_willDestroyAnimCoreQueue[i] == animCore)
			return false;
	}
	m_willDestroyAnimCoreQueue.append(animCore);
	NotifyLazyDestroy(0, false);
	return true;
}

// ���������ܻᵼ��m_bIsDeleteing == true
void NodeAnim::StopCurAnimCore() {
	int curAnimCoreSize = (int)m_curAnimCore.size();
	if (0 == curAnimCoreSize) // ���ﲻ�ܵ���juageAllAnimHaveFinishThenClearSource����Ϊ�����߸����ж��Ƿ���Ҫ�峡
		return;

	ref();
	bool bHadStop = false;
	for (int i = 0; i < curAnimCoreSize; ++i) {
		if (0 == m_curAnimCore[i])
			continue;

		m_curAnimCore[i]->Stop(); // ����ͣ����ʱ��ص���NodeAnim::endAnim��Ȼ�����첽�ص���delete
	}
	deref();
}

NodeAnim* NodeAnim::SetAllAnimFinishCallback(AnimEndCallback callback, void* callbackParam1, void* callbackParam2) {
	if (-1 == m_parentCacheIdx) {
		if (callback)
			callback(callbackParam1, callbackParam2);
		return this;
	}

	m_endCallbackWhenAllAnimFinish = callback;
	m_endCallbackWhenAllAnimFinishParam1 = callbackParam1;
	m_endCallbackWhenAllAnimFinishParam2 = callbackParam2;
	return this;
}

void NodeAnim::NotifyLazyDestroy(int releaseKQueryCountAtTimer, bool needDestroyMyself) {
	m_releaseKQueryAtTimerQueue.append(releaseKQueryCountAtTimer);
	m_needDestroyMyselfQueue.append(needDestroyMyself);
	// m_LazyDestroytimer.startOneShot(0);
}

void NodeAnim::LazyDestroy(Timer<NodeAnim>*) {
	int i = 0;
	for (i = 0; i < (int)m_willDestroyAnimCoreQueue.size(); ++i)
		delete m_willDestroyAnimCoreQueue[i];
	m_willDestroyAnimCoreQueue.clear();

	deref();
}

class DelayAnimFinishTask : public PageManagerDelayTask {
	WTF_MAKE_FAST_ALLOCATED;
public:
	virtual void Run(CPageManager* manager, Moment moment) {
		m_endCallbackWhenAllAnimFinish(m_endCallbackWhenAllAnimFinishParam1, m_endCallbackWhenAllAnimFinishParam2);
	}

	virtual void Destroy() {delete this;}

	AnimEndCallback m_endCallbackWhenAllAnimFinish;
	void* m_endCallbackWhenAllAnimFinishParam1;
	void* m_endCallbackWhenAllAnimFinishParam2;
};

bool NodeAnim::JuageAllAnimHaveFinishThenClearSource() {
	ASSERT(CheckAnimQueueLength(m_runingAnimQueueLength, m_curAnimCore));
	ASSERT(CheckAnimCoresIsEmpty(m_curAnimCore));
	ASSERT(0 == m_animPropsQueue.size());

	if (0 != m_animPropsQueue.size()) 
		return false;

	m_bIsDeleteing = true;

	// ���ж����Ѿ�����
	if (m_endCallbackWhenAllAnimFinish) {
		DelayAnimFinishTask* task = new DelayAnimFinishTask();
		task->m_endCallbackWhenAllAnimFinish = m_endCallbackWhenAllAnimFinish;
		task->m_endCallbackWhenAllAnimFinishParam1 = m_endCallbackWhenAllAnimFinishParam1;
		task->m_endCallbackWhenAllAnimFinishParam2 = m_endCallbackWhenAllAnimFinishParam2;
		m_node->GetManager()->PostDelayTask(task);
		m_endCallbackWhenAllAnimFinish = 0;
	}

	m_node->ClearAnimWhenNoticed(m_parentCacheIdx); // ֪ͨUINode�ڵ�

	NotifyLazyDestroy(m_releaseKQueryWhenAllAnimFinish, true);

	LazyDestroy(0); // �ӳ�������ǰ��

	return true;
}

void NodeAnim::EndAnim(bool IsForce, const CStdString& context, int parentCurAnimCoreCacheIdx) {
	ASSERT(parentCurAnimCoreCacheIdx < (int)m_curAnimCore.size());
	PostWillDestroyAnimCoreQueueNotRepeat(m_curAnimCore[parentCurAnimCoreCacheIdx]);
	m_curAnimCore[parentCurAnimCoreCacheIdx] = 0;

	m_hadFinishAnimQueue++;
	if (m_hadFinishAnimQueue != m_runingAnimQueueLength) // ���ͬһʱ����ڵĶ�����������
		return;
	
	ASSERT(CheckAnimCoresIsEmpty(m_curAnimCore));
	m_runingAnimQueueLength = 0;
	if (0 == m_animPropsQueue.size()) {// ���ж����Ѿ�����
		JuageAllAnimHaveFinishThenClearSource();
		return;
	}

	if (false == IsForce) {
		Start();
	} else { // �����ǿ����ֹ������Ҫ�ֶ��Ƴ�һ���������������start()���Զ��Ƴ�
		m_animPropsQueue.remove(0);
		if (0 == m_animPropsQueue.size()) { // ���ж����Ѿ�����
			JuageAllAnimHaveFinishThenClearSource();
			return;
		}
	}
}

// �ṩ��KdAnimCore�Ļص�
bool NodeAnim::StepAnim(
	float result,
	const CStdString* attr,
	AnimStepCallback stepCallback,
	void* stepCallbackParam1,
	void* stepCallbackParam2
	) {
	if ((!stepCallback && !attr) || (attr && *attr == _SC("NodeAnim-delay")))
		return false;

	if (stepCallback) {
		if (!attr) // ûֵ�������ʾ�������һ��step
			return stepCallback(result, 0, 0, stepCallbackParam1, stepCallbackParam2);
		return stepCallback(result, attr->GetString(), attr->GetLength(), stepCallbackParam1, stepCallbackParam2);
	} 

	if (result < 0) {
		if (*attr != _SC("x") &&
			*attr != _SC("y") &&
			*attr != _SC("translate-x") &&
			*attr != _SC("translate-y"))
			result = 0;
	}

	CStdString strResult;
	strResult.Format(_SC("%f"), result);
	m_node->SetAttrBegin();
	m_node->SetAttr(attr->GetString(), strResult.GetString());
	m_node->SetAttrEnd();

	return false;
}

//////////////////////////////////////////////////////////////////////////

const int KdAnimate_StepLen = 100; // ��ʱ��ʱ����

// t: current time����ǰʱ�䣩��
// b: beginning value����ʼֵ����
// c: change in value���仯������
// d: duration������ʱ�䡢��������
// ps��Elastic��Back��������ѡ���������涼��˵����

#ifndef NDEBUG
static RefCountedLeakCounter KdAnimCoreCounter(_SC("KdAnimCore"));
#endif

KdAnimCore::KdAnimCore(
	float from, float to, int dur,
	NodeAnim* pHandleObj, const CStdString& attr, 
	KQueryTween Tween, KQueryEasing Ease, int stepLen, float p0, float p1,
	int parentCurAnimCoreCacheIdx,
	AnimStepCallback stepCallback,
	void* stepCallbackparam1,
	void* stepCallbackparam2
	) {
#ifndef NDEBUG
	KdAnimCoreCounter.increment();
#endif
	ThreadTimers* threadTimers = pHandleObj->AttachedNode()->GetManager()->GetThreadTimers();
	KDASSERT(!!threadTimers);
	m_timer = new Timer<KdAnimCore>(this, &KdAnimCore::Fire, threadTimers);
	
	m_t = 0.0;
	m_b = from + 0.0f;
	m_c = to - from + 0.0f;
	m_d = (dur + 0.0f)/(stepLen + 0.0f);
	m_p0 = p0;
	m_p1 = p1;
	ASSERT(m_d >= 1 && m_d < 10000);
	m_pObj = pHandleObj;
	m_attr = attr;
	m_Tween = Tween;
	m_Ease = Ease;
	m_StepLen = stepLen;
	m_stepCallback = stepCallback;
	m_stepCallbackparam1 = stepCallbackparam1;
	m_stepCallbackparam2 = stepCallbackparam2;

	m_currentTime = currentTimeMS();

	m_parentCurAnimCoreCacheIdx = parentCurAnimCoreCacheIdx;

	m_pTweenFunc = KqGetTween(Tween, Ease);
	m_active = true;
}

KdAnimCore::~KdAnimCore() {
#ifndef NDEBUG
	KdAnimCoreCounter.decrement();
#endif
	UHDeletePtr(&m_timer);
}

bool KdAnimCore::_Fire() {
	bool bStop = false;
	float Result = 0.0;
	if(m_t++ < m_d) {
		Result = m_pTweenFunc(m_t, m_b, m_c, m_d, m_p0, m_p1);
		bStop = m_pObj->StepAnim(Result, &m_attr, m_stepCallback, m_stepCallbackparam1, m_stepCallbackparam2);
		if (bStop)
			Stop(true);
	} else {
		Result = m_pTweenFunc(m_d, m_b, m_c, m_d, m_p0, m_p1); // Ϊ�˷�ֹ���һ�ε�ֵ����ȷ������һ��
		bStop = m_pObj->StepAnim(Result, &m_attr, m_stepCallback, m_stepCallbackparam1, m_stepCallbackparam2);
		Stop(false);
		bStop = true;
	}

	return bStop;
}

void KdAnimCore::Fire(Timer<KdAnimCore>*) {
	if (false == m_active)
		return;

	double currentTime = currentTimeMS();
	double oldTime = m_currentTime;
	m_currentTime = currentTime; 
	if (currentTime - oldTime > 1000) { // �����ʱ����̫���أ�����������
		while (!_Fire()) { }
		return;
	}

	_Fire();
}

void KdAnimCore::Start() { 
	m_timer->startRepeating(m_StepLen/1000.0);
}

void KdAnimCore::Stop() {
	Stop(true);
}

void KdAnimCore::Stop(bool IsForce) {
	m_timer->stop();
	m_active = false;

	// ���һ��step����Ϊ�˸���step�ص����Ա��ͷ���Դ
	m_pObj->StepAnim(0, 0, m_stepCallback, m_stepCallbackparam1, m_stepCallbackparam2);
	m_pObj->EndAnim(IsForce, m_attr, m_parentCurAnimCoreCacheIdx);
}