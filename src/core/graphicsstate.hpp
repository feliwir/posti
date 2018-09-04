#pragma once
namespace ps
{
  enum class LineCap
  {
    Butt = 0,
    Round = 1,
    Square = 2
  };

  enum class LineJoin
  {
    Miter = 0,
    Round = 1,
    Bevel = 2
  };

  enum class ColorSpaces
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

  class GraphicsState final
  {
  public:
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

  private:
    int m_lineWidth;
    LineCap m_lineCap;
    LineJoin m_lineJoin;
    int m_miterLimit;
    bool m_strokeAdjust;
    ColorSpaces m_cspace;
    Color m_color;
  };
}
