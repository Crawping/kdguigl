#ifndef ClipData_h
#define ClipData_h

#include <wtf/RefCounted.h>

class KdPath;

// �����ClipAttrData�������ǣ��Ǹ��Ǵ�������ȡ�����ģ���Ҫ���������װһ����ܷ��ظ�����
// ��ֱ�ӷ���m_path��ԭ���ǣ���Щ��������overflow���Ǳ�new�����ģ���������Ҫɾ��������Ĳ���Ҫ
class ClipData : public RefCounted<ClipData> {
	WTF_MAKE_FAST_ALLOCATED;
public:
	ClipData(KdPath* path, bool bDeleteIfUninit) 
		: m_path(path)
		, m_bDeleteIfUninit(bDeleteIfUninit) {}
	~ClipData();

	const KdPath* Path() {return m_path;}
	
protected:
	bool m_bDeleteIfUninit;
	KdPath* m_path;
};

#endif // ClipData_h