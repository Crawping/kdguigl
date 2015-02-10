#ifndef NodeAttrDef_h
#define NodeAttrDef_h

// �������һ�����ԣ�Ҫ�ǵã�
// 1. NodeStyle::GetStyleTypeMap��˳��Ӷ�Ӧ�ַ���
// 2. NodeStyle::ParseXXXX ��һ������������������Ĭ��
// 3. extern NRSTDefaultVal defaultValXXXXXXX ��Ĭ��ֵ
// 4. NodeRareStyleSet(int, X) ��һ���ⲿ���Է������ӿ�
// 5. ����hash

// �������������ö��ֵ����NodeStyle::ParseEnum���϶�Ӧ��json
enum NodeAttrDef {
	eNRStyleId, // 0xd4d
	eNRStyleStyle, // 0x760f5f1

	eNRStyleX, // 0x78
	eNRStyleY, // 0x79
	eNRStyleWidth, // 0x79b47c0
	eNRStyleHeight, // 0x162aa479

	eNRStyleRx, // 0xfea
	eNRStyleRy, // 0x100b

	eNRStyleOpacity, // 0x78812219

	eNRStyleStrokeColor, // 0xf36817d8
	eNRStyleStrokeWidth, // 0xfa09d585
	eNRStyleStrokeOpacity, // 0x97971be
	eNRStyleStrokeDasharray, // 0xd0fd57a4
	eNRStyleFillColor, // 0x3d1247
	eNRStyleFillOpacity, // 0x7e42baad
	
	eNRStyleStopColor, // 0x454c1db2
	eNRStyleStopOpacity, // 0x7e44fa6c

	eNRStyleSrc, // 0x1b448

	eNRStyleVisibility, // 0xe0a5528
	eNRStylePointerEvents, //0xd0130943

	eNRStyleX1, // 0x6c9
	eNRStyleY1, // 0x6ca
	eNRStyleX2, // 0x6ea
	eNRStyleY2, // 0x6eb

	eNRStyleOffset, // 0x15fae2e7

	eNRStyleTextAnchor, // 0xcd1b562d ����ı���ȫ�����⡣��Ϊ��NodeStyle::ParseStyle����������Χ�ڵ�����
	eNRStyleTextDecoration, // 0x1578c9fa
	eNRStyleTextOverflow, // 0x1578c9fa
	eNRStyleFontSize, // 0x2ecc015f
	eNRStyleFontFamily, // 0x3887e566
	eNRStyleFontWeight, // 0x4f1e86cc
	
	eNRStyleTranslateX, // 0x6b5aad13
	eNRStyleTranslateY, // 0xf0b76254
	eNRStyleTransform, // 0x251a70dc

	eNRStyleClipPath, // 0x93e2b002
	eNRStyleMask, // 0x3ca2ac
	eNRStyleShadow, // 0xb3e26adf

	eNRStyleCx, // 0xfdb
	eNRStyleCy, // 0xffc
	eNRStyleR, // 0x72

	eNRStyleD, // 0x64

	eNRStyleOverflow, // 0x367d7414

	eNRStyleFx, // 0xfde
	eNRStyleFy, // 0xfff

	eNRStylePreserveAspectRatio, // 0x206875cb
	eNRStyleNinePatch,
	eNRStyleShapeRendering,

	eNRStyleEnd
};

//  ����ʾԪ�ع�ͬ���ԡ�����id���Իᴥ�����������������Ҳ��һ�ֿ�ˢ������
#define eNRStyleVisualNodeCommAttr \
	eNRStyleId,\
	eNRStyleVisibility,\
	eNRStyleOpacity,\
	eNRStyleClipPath,\
	eNRStyleMask,\
	eNRStyleShadow,\
	eNRStyleTranslateX,\
	eNRStyleTranslateY,\
	eNRStyleTransform

// enum NodeAttrHash {
// 	eNRHashId = 0xd4d,
// 	eNRHashStyle = 0x760f5f1,
// 
// 	eNRHashX = 0x78,
// 	eNRHashY = 0x79,
// 	eNRHashWidth = 0x79b47c0,
// 	eNRHashHeight = 0x162aa479,
// 
// 	eNRHashRx = 0xfea,
// 	eNRHashRy = 0x100b,
// 
// 	eNRHashOpacity = 0x78812219,
// 
// 	eNRHashStrokeColor = 0xf36817d8,
// 	eNRHashStrokeWidth = 0xfa09d585,
// 	eNRHashStrokeOpacity = 0x97971be,
// 	eNRHashStrokeDasharray = 0xd0fd57a4,
// 	eNRHashFillColor = 0x3d1247,
// 	eNRHashFillOpacity = 0x7e42baad,
// 
// 	eNRHashStopColor = 0x454c1db2,
// 	eNRHashStopOpacity = 0x7e44fa6c,
// 
// 	eNRHashSrc = 0x1b448,
// 
// 	eNRHashVisibility = 0xe0a5528,
// 	eNRHashPointerEvents = 0xd0130943,
// 
// 	eNRHashX1 = 0x6c9,
// 	eNRHashY1 = 0x6ca,
// 	eNRHashX2 = 0x6ea,
// 	eNRHashY2 = 0x6eb,
// 
// 	eNRHashOffset = 0x15fae2e7,
// 
// 	eNRHashTextAnchor = 0xcd1b562d,
// 	eNRHashTextDecoration = 0x1578c9fa,
// 	eNRHashFontSize = 0x2ecc015f,
// 	eNRHashFontFamily = 0x3887e566,
// 	eNRHashFontWeight = 0x4f1e86cc,
// 
// 	eNRHashTranslateX = 0x6b5aad13,
// 	eNRHashTranslateY = 0xf0b76254,
// 	eNRHashTransform = 0x251a70dc,
// 
// 	eNRHashClipPath = 0x93e2b002,
// 	eNRHashMask = 0x3ca2ac,
// 	eNRHashShadow = 0xb3e26adf,
// 
// 	eNRHashCx = 0xfdb,
// 	eNRHashCy = 0xffc,
// 	eNRHashR = 0x72,
// 
// 	eNRHashD = 0x64,
// 
// 	eNRHashOverflow = 0x367d7414,
// 
// 	eNRHashFx = 0xfde,
// 	eNRHashFy = 0xfff,
// 
// 	eNRHashPreserveAspectRatio = 0x206875cb,
// 
// 	eNRHashEnd
// };

#endif // NodeAttrDef_h