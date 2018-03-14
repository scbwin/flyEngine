#version 330

layout(location = 0) out vec3 color;

uniform sampler2D normalSampler;
uniform sampler2DArray gBufferSampler;
uniform sampler2D lightingSampler;
uniform float time;

uniform vec3 lightPosViewSpace;
uniform vec3 lightColor;
uniform vec3 normalViewSpace;
uniform mat3 TBN_view;

in vec2 uv;
in vec3 pos_view_space;

void main()
{
  float frequ = 128.f;
  vec2 uvw = uv * frequ;
  //vec3 normal_ts = mix(texture(normalSampler, uv * frequ + time * 0.05f).rgb, texture(normalSampler, uv * frequ + vec2(time * 0.04f, 0.f)).rgb, 0.5f);
  float t = time * 0.02f;
  vec3 normal_ts = texture(normalSampler, uvw + vec2(t, 0.f)).rgb
					+ texture(normalSampler, uvw * 0.99f - vec2(t, 0.f)).rgb
					+ texture(normalSampler, uvw * 0.992f + vec2(0.f, t)).rgb
					+ texture(normalSampler, uvw * 0.994f - vec2(0.f, t)).rgb;
  normal_ts *= 0.25f;
  vec3 n = normalize(TBN_view * (normal_ts * 2.f - 1.f));
  
  vec2 uv_screen = gl_FragCoord.xy / vec2(textureSize(gBufferSampler, 0));
  vec2 uv_screen_distorted = uv_screen + (1.f - (normal_ts.y)) / (-pos_view_space.z) * 2.f;
  if (uv_screen_distorted.x > 1.f) {
	uv_screen_distorted.x = 2.f - uv_screen_distorted.x;
  }
  if (uv_screen_distorted.y > 1.f) {
	uv_screen_distorted.y = 2.f - uv_screen_distorted.y;
  }
  vec3 refractive_color = texture(lightingSampler, uv_screen_distorted).rgb;

  vec3 light_pos_view_space = lightPosViewSpace;
  vec3 normal = normalViewSpace;
  if (!gl_FrontFacing) {
    n = -n;
    normal = -normal;
    light_pos_view_space = -light_pos_view_space;
  }
  vec3 l = normalize(light_pos_view_space - pos_view_space);

  vec3 e = normalize(-pos_view_space);
  vec3 r = normalize(reflect(-l, n));
  float spec = clamp(dot(e, r), 0.f, 1.f);
  float diff = clamp(dot(n, l), 0.f, 1.f);

  vec3 water_color = vec3(57.f, 88.f, 121.f) / 255.f;
  vec3 reflexive_color = water_color * diff + pow(spec, 8.f) * lightColor;

  float fresnel = clamp(dot(normal, e), 0.f, 1.f);

  float fade = 1.f - smoothstep(0.f, 10.f, distance(pos_view_space.z, texture(gBufferSampler, vec3(uv_screen, 0.f)).z));

  color = mix(reflexive_color, refractive_color, fresnel);
  color = mix(color, texture(lightingSampler, uv_screen).rgb, fade);
}