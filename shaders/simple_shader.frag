#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;

layout (location = 0) out vec4 outColor;

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

void main()
{


    vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    //保存每个光源镜面反射贡献的总量
    vec3 specularLight = vec3(0.0);
    //由于线性插值注意需要重新对fragNormalWorld归一化
    vec3 surfaceNormal = normalize(fragNormalWorld);

    vec3 cameraPosWorld = ubo.inverseView[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

    //多点光源
    for (int i = 0; i < ubo.numLights; i++) {
        PointLight light = ubo.pointLights[i];
        vec3 directionToLight = light.position.xyz - fragPosWorld;
        //衰减分量， 需要在归一化方向向量之前计算
        float attenuation = 1.0 / dot(directionToLight, directionToLight);
        directionToLight = normalize(directionToLight);

        float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
        vec3 intensity = light.color.xyz * light.color.w * attenuation;

        //计算叠加
        diffuseLight += intensity * cosAngIncidence;

        //specular lighting
        vec3 halfAngle = normalize(directionToLight + viewDirection);
        float blinnTerm = dot(surfaceNormal, halfAngle);
        //限制在0~1以忽略观察者和光源位于平面两侧的情况
        blinnTerm = clamp(blinnTerm, 0, 1);
        //指数越高， 高光效果越明显
        blinnTerm = pow(blinnTerm, 512.0);
	specularLight += intensity * blinnTerm;
    }

    outColor = vec4(diffuseLight * fragColor + specularLight * fragColor, 1.0);

    //平行光
    //float lightIntensity = AMBIENT + max(dot(normalWorldSpace, ubo.directionToLight), 0);
    //fragColor = lightIntensity * color;

}
