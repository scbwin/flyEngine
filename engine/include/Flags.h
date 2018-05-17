#ifndef FLAGS_H
#define FLAGS_H

namespace fly
{
  enum MeshRenderFlag : unsigned
  {
    MR_NONE = 0,
    MR_DIFFUSE_MAP = 1,
    MR_NORMAL_MAP = 2,
    MR_ALPHA_MAP = 4,
    MR_HEIGHT_MAP = 8,
    MR_WIND = 16,
    MR_REFLECTIVE = 32
  };

  enum ShaderSetupFlags : unsigned
  {
    SS_NONE = 0,
    SS_SHADOWS = 1,
    SS_TIME = 2,
    SS_WIND = 4,
    SS_VP = 8,
    SS_LIGHTING = 16,
    SS_GAMMA = 32,
    SS_P_INVERSE = 64
  };
}

#endif // !FLAGS_H
