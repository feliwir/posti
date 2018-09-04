#pragma once
namespace ps
{
	class graphicsState final
	{
	public:
		enum LineCap
		{
			butt = 0,
			round = 1,
			square = 2
		};
		enum LineJoin
		{
			miter = 0,
			round = 1,
			bevel = 2
		};
		enum ColorSpaces
		{
			//language level2
			DeviceGray,
			DeviceRGB,
			DeviceCMYK,
			CIEBasedABC,
			CIEBasedA,
			Pattern,
			Indexed,
			Separation,
			//language level 3
			CIEBasedDEF,
			CIEBasedDEFG,
			DeviceN
		};
		struct Color
		{
			int R;
			int G;
			int B;
			int hue;
			int saturation;
			int brightness;
			int cyan;
			int magenta;
			int yellow;
			int black;
		};
		struct gState
		{
			int lineWidth;
			LineCap lineCap;
			LineJoin lineJoin;
			int miterLimit;
			bool strokeAdjust;
			ColorSpace cspace;
			Color color;
		};
		gState _state;
	};
}
