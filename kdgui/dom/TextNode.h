#ifndef TextNode_h
#define TextNode_h

#include "UINode.h"
#include "style/NodeAttrDef.h"

struct TextRun;

class TextNode : public UINode {
	WTF_MAKE_FAST_ALLOCATED;
public:
	TextNode();
	virtual ~TextNode();

	virtual LPCTSTR GetClass() const {return _SC("text");}
	virtual UINodeEnum GetClassEnum() const {return eNE_Text;}

	virtual int Dispatch(LPCTSTR func, int nArg, LPCTSTR strArg, void* arg, void* ret);

	virtual IntRect BoundingRectInLocalCoordinates();

	virtual bool SetAttr(LPCTSTR pstrName, LPCTSTR pstrValue);

	virtual NodeAttrDef* GetCanRefreshAttrs(NodeAttrDef style) const {
		static NodeAttrDef validAttrs[] = {
			eNRStyleX,
			eNRStyleY,
			eNRStyleWidth,
			eNRStyleHeight,

			eNRStyleTextAnchor,
			eNRStyleTextDecoration,
			eNRStyleFontSize,

			eNRStyleVisualNodeCommAttr,

			eNRStyleEnd
		};
		return validAttrs;
	}

	virtual void Paint(GraphicsContext* g, const IntRect &rcInvalid);

protected:
	bool CreateTextRunByStyle(const NodeStyle* style, TextRun& textRun);
	void DrawWithEllipsis(GraphicsContext* g, IntRect& bounding, TextRun& textRun);
	IntRect GetBoundingByStyle();

	CStdString m_text;
	bool m_bHadSettedTextStyle; // Ϊ���Ż�����ȫ�������������ǰ����Ҫ����CreateTextRunByStyle
	int m_descender;
	IntRect m_rcPosWithoutShadow; // ��������Ӱ��ԭʼ��С
	bool m_bNeedEllipsis; // �����text ��Ⱥ󣬷�����Ҫ�ü����ʡ�Ժ�
};

#endif // TextNode_h