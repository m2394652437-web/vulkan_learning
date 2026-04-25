#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
//获得片段所位于的世界方向，以便于计算片段的光源方向
layout(location = 1) out vec3 fragPosWorld;
//保存片段在世界空间中的法线
layout(location = 2) out vec3 fragNormalWorld;


struct PointLight {
  vec4 position;
  vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 ambientLightColor;
    //计算光照时候须保证输入为单位向x量
    //vec3 directionToLight;
    PointLight pointLights[10];
    int numLights;
} ubo;

//每个着色器入口点只能用一个推送块
//如果要在任意GPU上运行则最多推送128B的数据，也就是两个 mat4
layout(push_constant) uniform Push {
    mat4 modelMatrix; // projection * view * model
    mat4 normalMatrix; // model space -> world space
} push;

//用于模拟自然情况下的间接光照
const float AMBIENT = 0.02;

void main()
{

    //模型在world space 中的位置
    vec4 positionWorld =  push.modelMatrix * vec4(position, 1.0);
    //第二个参数为齐次坐标, 如果要变换方向向量则应为0

    gl_Position = ubo.projection * ubo.view * positionWorld;

    //由于对其需求，所以用mat4传递再mat3截断
    fragNormalWorld= normalize(mat3(push.normalMatrix) * normal);
    fragPosWorld = positionWorld.xyz;
    fragColor = color;

}
