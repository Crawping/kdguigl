#include <UIlib.h>
#include "GNode.h"
#include <wtf/UtilHelp.h>
#include "style/NodeStyle.h"
#include "rendering/ClipData.h"
#include "rendering/RenderSupport.h"

#include "graphics/KdPath.h"
#include "graphics/GraphicsContext.h"

GNode::GNode() : m_fastImageClipRect(NULL) {}

GNode::~GNode() {
	UHDeletePtr(&m_fastImageClipRect);
}

NodeAttrDef* GNode::GetCanRefreshAttrs(NodeAttrDef style) const {
	if (eNRStyleX <= style && style <= eNRStyleHeight && m_nodeStyle->IsStyleSetted(eNRStyleOverflow)) {
		static NodeAttrDef validAttrs0[] = {
			eNRStyleX,
			eNRStyleY,
			eNRStyleWidth,
			eNRStyleHeight,
			eNRStyleVisualNodeCommAttr,
			eNRStyleEnd
		};
		return validAttrs0; // �����������ü����ԣ�����Ҫ��Ӧx��y��������
	}

	static NodeAttrDef validAttrs1[] = {
		eNRStyleVisualNodeCommAttr,
		eNRStyleEnd
	};
	return validAttrs1;
}

bool GNode::CanFastDrawImage() { // ĳЩ����£�ͼƬ���Կ��ٻ��ƣ�����Ҫ�߼�����
	if (1 != m_children.size() || eNE_Image != m_children[0]->GetClassEnum())
		return false;

	if (m_nodeStyle->IsStyleSetted(eNRStyleShadow))
		return false;

	UINode* maskNode = RenderSupport::GetMaskNodeFromCurrentNode(this);
	if (maskNode) {
		maskNode->deref();
		return false;
	}

	if (m_nodeStyle->IsStyleSetted(eNRStyleOverflow)) {
		UHDeletePtr(&m_fastImageClipRect);
		m_fastImageClipRect = new IntRect(m_nodeStyle->X(), m_nodeStyle->Y(), m_nodeStyle->Width(), m_nodeStyle->Height());
		return true;
	}

	if (m_nodeStyle->IsStyleSetted(eNRStyleClipPath)) {
		PassRefPtr<ClipData> clipData = RenderSupport::GetClipPathFromNode(this);
		const KdPath* clipPath = clipData->Path();
		if (!clipPath || clipPath->isEmpty()) // ��������ˣ�����������Ч��������ͨ���̣�����ͼƬ��ʵҲ���ᱻ����
			return false;

		if (!clipPath->platformPath()->isRect(NULL, NULL))
			return false;

		UHDeletePtr(&m_fastImageClipRect);
		m_fastImageClipRect = new IntRect(clipPath->boundingRect());
		return true;
	}

	return false;
}

void GNode::PaintFastImage(GraphicsContext* g, const IntRect &rcInvalid) {
	IntRect rcPaint(rcInvalid);
	if (!RenderSupport::RenderBegin(this, rcPaint))
		return;

	GraphicsContextStateSaver stateSaver(*g);
	g->concatCTM(*m_nodeStyle->GetTransform());

	float opacity = m_nodeStyle->Opacity();
	if (opacity < 1)
		g->beginTransparencyLayer(opacity);

	m_children[0]->Paint(g, rcPaint);

	if (opacity < 1)
		g->endTransparencyLayer();
}

void GNode::Paint(GraphicsContext* g, const IntRect &rcInvalid) {
	if (CanFastDrawImage ())
		PaintFastImage(g, rcInvalid);
	else
		UINode::Paint(g, rcInvalid);
}
